module;
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <concepts>
#include <expected>
#include <string_view>
export module jowi.io:sys_net;
import jowi.generic;
import :error;
import :fd_type;
import :sys_util;
import :sys_file;

/**
 * @file unix/sys_net.cc
 * @brief POSIX networking abstractions built on top of generic file descriptors.
 */

namespace jowi::io {
  /**
   * @brief Supported socket protocols.
   */
  export enum struct sock_protocol { TCP, UDP };
  /**
   * @brief Mapping from protocol enum to system socket type constants.
   */
  std::array sock_protocol_to_sys{SOCK_STREAM, SOCK_DGRAM};
  /**
   * @brief Concept satisfied by network address types consumable by POSIX socket calls.
   */
  export template <class net_address_type>
  concept is_net_address = requires(net_address_type addr, const net_address_type caddr) {
    { caddr.sys_domain() } -> std::same_as<sa_family_t>;
    { caddr.sys_addr() } -> std::same_as<std::pair<const sockaddr *, socklen_t>>;
    { addr.sys_addr() } -> std::same_as<std::pair<sockaddr *, socklen_t>>;
    { net_address_type::empty() } -> std::same_as<net_address_type>;
  };

  /**
   * @brief IPv4 socket address helper offering fluent configuration.
   */
  export struct ipv4_address {
  private:
    sockaddr_in __addr;

    ipv4_address() noexcept : __addr{} {
      __addr.sin_family = AF_INET;
      __addr.sin_addr.s_addr = INADDR_ANY;
    }

  public:
    /**
     * @brief Binds the address to all interfaces.
     * @return Reference to this address for chaining.
     */
    ipv4_address &all_interface() noexcept {
      __addr.sin_addr.s_addr = INADDR_ANY;
      return *this;
    }
    /**
     * @brief Sets the address to the loopback interface.
     * @return Reference to this address for chaining.
     */
    ipv4_address &localhost() noexcept {
      __addr.sin_addr.s_addr = INADDR_LOOPBACK;
      return *this;
    }
    /**
     * @brief Parses and stores a dotted-quad IPv4 address.
     * @param addr Textual IPv4 address.
     * @return Success or IO error if conversion fails.
     */
    std::expected<void, io_error> addr(std::string_view addr) noexcept {
      return sys_call_void(inet_aton, addr.data(), &__addr.sin_addr);
    }
    /**
     * @brief Configures the port number.
     * @param port Port in host byte order.
     * @return Reference to this address for chaining.
     */
    ipv4_address &port(int port) noexcept {
      __addr.sin_port = port;
      return *this;
    }
    /**
     * @brief Returns the address family constant used for socket creation.
     * @return AF_INET constant.
     */
    sa_family_t sys_domain() const noexcept {
      return AF_INET;
    }
    /**
     * @brief Returns a const pointer/length pair suitable for socket APIs.
     * @return Pair containing const sockaddr pointer and length.
     */
    std::pair<const sockaddr *, socklen_t> sys_addr() const noexcept {
      return std::pair{reinterpret_cast<const sockaddr *>(&__addr), sizeof(__addr)};
    }
    /**
     * @brief Returns a mutable pointer/length pair suitable for socket APIs.
     * @return Pair containing sockaddr pointer and length.
     */
    std::pair<sockaddr *, socklen_t> sys_addr() noexcept {
      return std::pair{reinterpret_cast<sockaddr *>(&__addr), sizeof(__addr)};
    }

    /**
     * @brief Creates an empty IPv4 address instance.
     * @return Default-initialized IPv4 address.
     */
    static ipv4_address empty() noexcept {
      return ipv4_address{};
    }
  };

  export struct ipv6_address {};
  /**
   * @brief UNIX domain socket address helper.
   */
  export struct local_address {
  private:
    sockaddr_un __addr;
    local_address() noexcept : __addr{} {
      __addr.sun_family = AF_LOCAL;
    }

  public:
    /**
     * @brief Sets the filesystem path for the local socket.
     * @param addr Filesystem path limited to 107 characters.
     * @return Reference to this address for chaining.
     */
    local_address &addr(const generic::fixed_string<107> &addr) noexcept {
      std::ranges::copy(addr, __addr.sun_path);
      return *this;
    }
    /**
     * @brief Returns the address family constant used for socket creation.
     * @return AF_LOCAL constant.
     */
    sa_family_t sys_domain() const noexcept {
      return AF_LOCAL;
    }
    /**
     * @brief Returns a const pointer/length pair suitable for socket APIs.
     * @return Pair containing const sockaddr pointer and length.
     */
    std::pair<const sockaddr *, socklen_t> sys_addr() const noexcept {
      return std::pair{reinterpret_cast<const sockaddr *>(&__addr), sizeof(__addr)};
    }
    /**
     * @brief Returns a mutable pointer/length pair suitable for socket APIs.
     * @return Pair containing sockaddr pointer and length.
     */
    std::pair<sockaddr *, socklen_t> sys_addr() noexcept {
      return std::pair{reinterpret_cast<sockaddr *>(&__addr), sizeof(__addr)};
    }
    /**
     * @brief Creates an empty local address instance.
     * @return Default-initialized local address.
     */
    static local_address empty() noexcept {
      return local_address{};
    }
    /**
     * @brief Factory for constructing a local address bound to the supplied path.
     * @param addr Filesystem path limited to 107 characters.
     * @return Local address initialized with the given path.
     */
    static local_address new_at(const generic::fixed_string<107> &addr) noexcept {
      return local_address{}.addr(addr);
    }
  };

  /**
   * @brief Read/write socket wrapper offering polling assistance.
   * @tparam addr_type Network address type satisfying `is_net_address`.
   */
  export template <is_net_address addr_type> struct sock_rw {
  private:
    file_type __f;
    addr_type __addr;
    sock_protocol __protocol;

  public:
    /**
     * @brief Constructs a read/write socket wrapper.
     * @param f File descriptor representing the socket.
     * @param addr Address associated with the socket endpoint.
     * @param protocol Protocol describing how the socket was created.
     */
    sock_rw(file_type f, addr_type addr, sock_protocol protocol) noexcept :
      __f{std::move(f)}, __addr{std::move(addr)}, __protocol{protocol} {}

    /**
     * @brief Accessor for the bound address.
     * @return Reference to the stored address.
     */
    const addr_type &addr() const noexcept {
      return __addr;
    }

    /**
     * @brief Reads bytes into the supplied buffer.
     * @param buf Writable buffer receiving data.
     * @return Success or IO error from `read`.
     */
    std::expected<void, io_error> read(is_writable_buffer auto &buf) noexcept {
      return sys_read(__f.fd(), buf);
    }

    /**
     * @brief Writes bytes to the socket.
     * @param v View describing the bytes to send.
     * @return Number of bytes written or IO error.
     */
    constexpr auto write(std::string_view v) noexcept {
      return sys_write(__f.fd(), v);
    }

    /**
     * @brief Checks if the socket is writable within the timeout window.
     * @param timeout Duration to wait before timing out.
     * @return True when writable, false on timeout, or IO error.
     */
    std::expected<bool, io_error> is_writable(
      std::chrono::milliseconds timeout = std::chrono::milliseconds{0}
    ) const noexcept {
      return sys_file_poller::write_poller().timeout(timeout)(__f.fd());
    }
    /**
     * @brief Checks if the socket is readable within the timeout window.
     * @param timeout Duration to wait before timing out.
     * @return True when readable, false on timeout, or IO error.
     */
    std::expected<bool, io_error> is_readable(
      std::chrono::milliseconds timeout = std::chrono::milliseconds{0}
    ) const noexcept {
      return sys_file_poller::read_poller().timeout(timeout)(__f.fd());
    }

    /**
     * @brief Returns a borrowed handle for the wrapped descriptor.
     * @return Non-owning file handle.
     */
    file_handle<int> handle() const {
      return __f.borrow();
    }
  };

  /**
   * @brief Socket wrapper managing lifetime prior to establishing a connection.
   * @tparam addr_type Network address type satisfying `is_net_address`.
   */
  export enum struct sock_state { FREE, BIND, LISTEN };
  export template <is_net_address addr_type, sock_state state> struct sock_free {
  private:
    file_type __f;
    addr_type __addr;
    sock_protocol __protocol;
    bool __non_blocking;

    friend struct sock_free<addr_type, sock_state::FREE>;
    friend struct sock_free<addr_type, sock_state::BIND>;
    friend struct sock_free<addr_type, sock_state::LISTEN>;

    template <sock_state o_state>
    sock_free(sock_free<addr_type, o_state> &&s) :
      __f{std::move(s.__f)}, __addr{s.__addr}, __protocol{s.__protocol},
      __non_blocking{s.__non_blocking} {}

  public:
    /**
     * @brief Constructs a movable socket wrapper.
     * @param f File descriptor representing the socket.
     * @param addr Address associated with the socket endpoint.
     * @param protocol Protocol describing how the socket was created.
     */
    sock_free(file_type f, addr_type addr, sock_protocol protocol) :
      __f{std::move(f)}, __addr{std::move(addr)}, __protocol{protocol}, __non_blocking{false} {}

    /**
     * @brief Requests non-blocking behavior for accepted or connected sockets.
     * @return Reference to this socket wrapper for chaining.
     */
    sock_free &non_blocking() noexcept {
      __non_blocking = true;
      return *this;
    }
    /**
     * @brief Accessor for the stored address.
     * @return Reference to the address.
     */
    const addr_type &addr() const noexcept {
      return __addr;
    }

    /**
     * @brief Returns a borrowed handle for the wrapped descriptor.
     * @return Non-owning file handle.
     */
    file_handle<int> handle() const {
      return __f.borrow();
    }
    /**
     * @brief Binds the socket to its configured address.
     * @return Bound socket wrapper or IO error.
     */
    std::expected<sock_free<addr_type, sock_state::BIND>, io_error> bind() && noexcept
      requires(state == sock_state::FREE)
    {
      auto [addr, addr_size] = const_cast<const addr_type &>(__addr).sys_addr();
      return sys_call_void(::bind, __f.fd(), addr, addr_size).transform([&]() {
        return sock_free<addr_type, sock_state::BIND>(std::move(*this));
      });
    }
    /**
     * @brief Places the socket into listening state.
     * @param backlog Maximum number of pending connections.
     * @return Socket wrapper ready to accept connections or IO error.
     */
    std::expected<sock_free<addr_type, sock_state::LISTEN>, io_error> listen(
      int backlog
    ) && noexcept requires(state == sock_state::BIND)
    {
      return sys_call_void(::listen, __f.fd(), backlog).transform([&]() {
        return sock_free<addr_type, sock_state::LISTEN>{std::move(*this)};
      });
    }
    /**
     * @brief Convenience helper combining `bind` and `listen`.
     * @param backlog Maximum number of pending connections.
     * @return Socket wrapper ready to accept connections or IO error.
     */
    std::expected<sock_free<addr_type, sock_state::LISTEN>, io_error> bind_listen(
      int backlog
    ) && noexcept requires(state == sock_state::FREE)
    {
      return std::move(*this).bind().and_then([&](auto &&sock) {
        return std::move(sock).listen(backlog);
      });
    }
    /**
     * @brief Connects the socket to its configured address.
     * @return Read/write socket wrapper or IO error.
     */
    std::expected<sock_rw<addr_type>, io_error> connect() && noexcept
      requires(state == sock_state::FREE)
    {
      auto [addr, addr_size] = const_cast<const addr_type &>(__addr).sys_addr();
      return sys_call_void(::connect, __f.fd(), addr, addr_size).transform([&]() {
        return sock_rw{std::move(__f), __addr, __protocol};
      });
    }
    /**
     * @brief Accepts an incoming connection.
     * @return Read/write socket representing the accepted connection or IO error.
     */
    std::expected<sock_rw<addr_type>, io_error> accept() noexcept
      requires(state == sock_state::LISTEN)
    {
      auto recv_addr = addr_type::empty();
      auto [addr, addr_size] = recv_addr.sys_addr();
      return sys_call(::accept, __f.fd(), addr, &addr_size).transform([&](auto fd) {
        return sock_rw{file_type{fd}, recv_addr, __protocol};
      });
    }

    /**
     * @brief Checks if the socket has an standing accept connection
     * @param timeout Duration to wait before timing out.
     * @return True when readable, false on timeout, or IO error.
     */
    std::expected<bool, io_error> is_readable(
      std::chrono::milliseconds timeout = std::chrono::milliseconds{0}
    ) const noexcept {
      return sys_file_poller::read_poller().timeout(timeout)(__f.fd());
    }
  };

  /**
   * @brief Fluent builder for configuring and creating sockets.
   * @tparam addr_type Network address type satisfying `is_net_address`.
   */
  export template <is_net_address addr_type> struct sock_options {
  private:
    addr_type __addr;
    sock_protocol __protocol;

  public:
    /**
     * @brief Initializes socket options with a target address.
     * @param addr Address used when creating the socket.
     */
    sock_options(addr_type addr) noexcept :
      __addr{std::move(addr)}, __protocol{sock_protocol::TCP} {}

    /**
     * @brief Switches the protocol to UDP.
     * @return Reference to this options object for chaining.
     */
    sock_options &udp() noexcept {
      __protocol = sock_protocol::UDP;
      return *this;
    }

    /**
     * @brief Switches the protocol to TCP.
     * @return Reference to this options object for chaining.
     */
    sock_options &tcp() noexcept {
      __protocol = sock_protocol::TCP;
      return *this;
    }

    /**
     * @brief Creates a socket using the configured address and protocol.
     * @return Movable socket wrapper or IO error.
     */
    std::expected<sock_free<addr_type, sock_state::FREE>, io_error> create() const noexcept {
      return sys_call(
               socket, __addr.sys_domain(), sock_protocol_to_sys[static_cast<int>(__protocol)], 0
      )
        .transform([&](int fd) {
          return sock_free<addr_type, sock_state::FREE>{fd, __addr, __protocol};
        });
    }
  };

  template struct sock_options<ipv4_address>;
  template struct sock_options<local_address>;
  template struct sock_rw<ipv4_address>;
  template struct sock_rw<local_address>;
  template struct sock_free<ipv4_address, sock_state::FREE>;
  template struct sock_free<ipv4_address, sock_state::BIND>;
  template struct sock_free<ipv4_address, sock_state::LISTEN>;
  template struct sock_free<local_address, sock_state::FREE>;
  template struct sock_free<local_address, sock_state::BIND>;
  template struct sock_free<local_address, sock_state::LISTEN>;

  export using ipv4_sock_options = sock_options<ipv4_address>;
  export using local_sock_options = sock_options<local_address>;
  export using ipv4_sock_rw = sock_options<ipv4_address>;
  export using local_sock_rw = sock_options<local_address>;
  export using ipv4_sock_free = sock_free<ipv4_address, sock_state::FREE>;
  export using ipv4_bind_sock_free = sock_free<ipv4_address, sock_state::BIND>;
  export using ipv4_listen_sock_free = sock_free<ipv4_address, sock_state::LISTEN>;
  export using local_sock_free = sock_free<local_address, sock_state::FREE>;
  export using local_bind_sock_free = sock_free<local_address, sock_state::BIND>;
  export using local_listena_sock_free = sock_free<local_address, sock_state::LISTEN>;
}
