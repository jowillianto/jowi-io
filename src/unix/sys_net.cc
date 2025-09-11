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

namespace jowi::io {
  export enum struct sock_protocol { TCP, UDP };
  std::array sock_protocol_to_sys{SOCK_STREAM, SOCK_DGRAM};
  export template <class net_address_type>
  concept is_net_address = requires(net_address_type addr, const net_address_type caddr) {
    { caddr.sys_domain() } -> std::same_as<sa_family_t>;
    { caddr.sys_addr() } -> std::same_as<std::pair<const sockaddr *, socklen_t>>;
    { addr.sys_addr() } -> std::same_as<std::pair<sockaddr *, socklen_t>>;
    { net_address_type::empty() } -> std::same_as<net_address_type>;
  };

  /**
    Abstracts the creation of an ipv4 address.
  */
  export struct ipv4_address {
  private:
    sockaddr_in __addr;

    ipv4_address() noexcept : __addr{} {
      __addr.sin_family = AF_INET;
      __addr.sin_addr.s_addr = INADDR_ANY;
    }

  public:
    ipv4_address &all_interface() noexcept {
      __addr.sin_addr.s_addr = INADDR_ANY;
      return *this;
    }
    ipv4_address &localhost() noexcept {
      __addr.sin_addr.s_addr = INADDR_LOOPBACK;
      return *this;
    }
    std::expected<void, io_error> addr(std::string_view addr) noexcept {
      return sys_call_void(inet_aton, addr.data(), &__addr.sin_addr);
    }
    ipv4_address &port(int port) noexcept {
      __addr.sin_port = port;
      return *this;
    }
    sa_family_t sys_domain() const noexcept {
      return AF_INET;
    }
    std::pair<const sockaddr *, socklen_t> sys_addr() const noexcept {
      return std::pair{reinterpret_cast<const sockaddr *>(&__addr), sizeof(__addr)};
    }
    std::pair<sockaddr *, socklen_t> sys_addr() noexcept {
      return std::pair{reinterpret_cast<sockaddr *>(&__addr), sizeof(__addr)};
    }

    static ipv4_address empty() noexcept {
      return ipv4_address{};
    }
  };

  export struct ipv6_address {};
  export struct local_address {
  private:
    sockaddr_un __addr;
    local_address() noexcept : __addr{} {
      __addr.sun_family = AF_LOCAL;
    }

  public:
    local_address &addr(const generic::fixed_string<107> &addr) noexcept {
      std::ranges::copy(addr, __addr.sun_path);
      return *this;
    }
    sa_family_t sys_domain() const noexcept {
      return AF_LOCAL;
    }
    std::pair<const sockaddr *, socklen_t> sys_addr() const noexcept {
      return std::pair{reinterpret_cast<const sockaddr *>(&__addr), sizeof(__addr)};
    }
    std::pair<sockaddr *, socklen_t> sys_addr() noexcept {
      return std::pair{reinterpret_cast<sockaddr *>(&__addr), sizeof(__addr)};
    }
    static local_address empty() noexcept {
      return local_address{};
    }
    static local_address new_at(const generic::fixed_string<107> &addr) noexcept {
      return local_address{}.addr(addr);
    }

    /*
      Address Getters
    */
  };

  export template <is_net_address addr_type> struct sock_rw {
  private:
    file_type __f;
    addr_type __addr;
    sock_protocol __protocol;

  public:
    sock_rw(file_type f, addr_type addr, sock_protocol protocol) noexcept :
      __f{std::move(f)}, __addr{std::move(addr)}, __protocol{protocol} {}

    const addr_type &addr() const noexcept {
      return __addr;
    }

    template <size_t N> constexpr auto read(generic::fixed_string<N> &buf) noexcept {
      return sys_read<N>(__f.fd(), buf);
    }

    constexpr auto write(std::string_view v) noexcept {
      return sys_write(__f.fd(), v);
    }

    std::expected<bool, io_error> is_writable(
      std::chrono::milliseconds timeout = std::chrono::milliseconds{0}
    ) const noexcept {
      return sys_file_poller::write_poller().timeout(timeout)(__f.fd());
    }
    std::expected<bool, io_error> is_readable(
      std::chrono::milliseconds timeout = std::chrono::milliseconds{0}
    ) const noexcept {
      return sys_file_poller::read_poller().timeout(timeout)(__f.fd());
    }
  };

  export template <is_net_address addr_type> struct sock_free {
  private:
    file_type __f;
    addr_type __addr;
    sock_protocol __protocol;
    bool __non_blocking;

  public:
    sock_free(file_type f, addr_type addr, sock_protocol protocol) :
      __f{std::move(f)}, __addr{std::move(addr)}, __protocol{protocol}, __non_blocking{false} {}

    /*
      This will set all socket created through accept or connect() to be non blocking
    */
    sock_free &non_blocking() noexcept {
      __non_blocking = true;
      return *this;
    }
    const addr_type &addr() const noexcept {
      return __addr;
    }
    std::expected<sock_free, io_error> bind() && noexcept {
      auto [addr, addr_size] = const_cast<const addr_type &>(__addr).sys_addr();
      return sys_call_void(::bind, __f.fd(), addr, addr_size).transform([&]() {
        return std::move(*this);
      });
    }
    std::expected<sock_free, io_error> listen(int backlog) && noexcept {
      return sys_call_void(::listen, __f.fd(), backlog).transform([&]() {
        return std::move(*this);
      });
    }
    std::expected<sock_free, io_error> bind_listen(int backlog) && noexcept {
      return std::move(*this).bind().and_then([&](auto &&sock) {
        return std::move(sock).listen(backlog);
      });
    }
    std::expected<sock_rw<addr_type>, io_error> connect() && noexcept {
      auto [addr, addr_size] = const_cast<const addr_type &>(__addr).sys_addr();
      return sys_call_void(::connect, __f.fd(), addr, addr_size).transform([&]() {
        return sock_rw{std::move(__f), __addr, __protocol};
      });
    }
    std::expected<sock_rw<addr_type>, io_error> accept() noexcept {
      auto recv_addr = addr_type::empty();
      auto [addr, addr_size] = recv_addr.sys_addr();
      return sys_call(::accept, __f.fd(), addr, &addr_size).transform([&](auto fd) {
        return sock_rw{file_type{fd}, recv_addr, __protocol};
      });
    }
  };

  export template <is_net_address addr_type> struct sock_options {
  private:
    addr_type __addr;
    sock_protocol __protocol;

  public:
    sock_options(addr_type addr) noexcept :
      __addr{std::move(addr)}, __protocol{sock_protocol::TCP} {}

    sock_options &udp() noexcept {
      __protocol = sock_protocol::UDP;
      return *this;
    }

    sock_options &tcp() noexcept {
      __protocol = sock_protocol::TCP;
      return *this;
    }

    std::expected<sock_free<addr_type>, io_error> create() const noexcept {
      return sys_call(
               socket, __addr.sys_domain(), sock_protocol_to_sys[static_cast<int>(__protocol)], 0
      )
        .transform([&](int fd) { return sock_free{fd, __addr, __protocol}; });
    }
  };
  /*
    Start from address declaration
  */
  // export struct ipv4_addr {
  //   int port;
  //   generic::static_string<15> addr;

  //   std::expected<struct sockaddr_in, io_error> sys_addr() const noexcept {
  //     struct sockaddr_in s_addr;
  //     s_addr.sin_family = AF_INET;
  //     s_addr.sin_port = port;
  //     s_addr.sin_addr.s_addr = addr;
  //     return s_addr;
  //   }
  // };

  // export struct ipv6_addr {
  //   int port;
  //   uint32_t flow_info;
  //   std::array<char, 16> addr;

  //   struct sockaddr_in6 sys_addr() const noexcept {
  //     struct sockaddr_in6 s_addr;
  //     s_addr.sin6_family = AF_INET6;
  //     s_addr.sin6_port = port;
  //     s_addr.sin6_flowinfo = flow_info;
  //   }
  // };

  // export struct local_addr {
  //   generic::static_string<107> addr;

  //   struct sockaddr_un sys_addr() const noexcept {
  //     struct sockaddr_un s_addr;
  //     s_addr.sun_family = AF_UNIX;
  //     std::ranges::copy_n(addr, 108, s_addr.sun_path);
  //   }
  // };

  // export enum struct net_addr_family { IPV4 = 0 };
  // constexpr static std::array addr_family_to_sys = {AF_INET};

  // export enum struct net_protocol { TCP = 0, UDP };
  // constexpr static std::array protocol_to_sys = {SOCK_STREAM, SOCK_DGRAM};

  // export struct sock_rw {
  // private:
  //   file_type __f;
  //   net_addr_family __dom;
  //   net_protocol __protocol;
  //   int __port;

  // public:
  //   sock_rw(file_type f, net_addr_family dom, net_protocol protocol, int port) noexcept :
  //     __f{std::move(f)}, __dom{dom}, __protocol{protocol}, __port{port} {}

  //   auto write(std::string_view v) noexcept {
  //     return sys_write(__f.fd(), v);
  //   }

  //   template <size_t N> std::expected<generic::static_string<N>, io_error> read_n() noexcept {
  //     return sys_read<N>{}();
  //   }

  //   std::expected<bool, io_error> is_readable(
  //     std::chrono::milliseconds timeout = std::chrono::milliseconds{0}
  //   ) noexcept {
  //     return sys_file_poller::read_poller(timeout)(__f.fd());
  //   }

  //   std::expected<bool, io_error> is_writable(
  //     std::chrono::milliseconds timeout = std::chrono::milliseconds{0}
  //   ) noexcept {
  //     return sys_file_poller::write_poller(timeout)(__f.fd());
  //   }

  //   int port() const noexcept {
  //     return __port;
  //   }
  //   net_protocol protocol() const noexcept {
  //     return __protocol;
  //   }
  //   net_addr_family domain() const noexcept {
  //     return __dom;
  //   }
  // };

  // export struct sock_listener {
  // private:
  //   file_type __f;
  //   net_addr_family __dom;
  //   net_protocol __protocol;
  //   int __port;
  //   int __backlog;

  // public:
  //   sock_listener(
  //     file_type f, net_addr_family dom, net_protocol protocol, int port, int backlog
  //   ) noexcept :
  //     __f{std::move(f)}, __dom{std::move(dom)}, __protocol{std::move(protocol)}, __port{port},
  //     __backlog{backlog} {}
  //   int port() const noexcept {
  //     return __port;
  //   }
  //   int backlog() const noexcept {
  //     return __backlog;
  //   }
  //   net_protocol protocol() const noexcept {
  //     return __protocol;
  //   }
  //   net_addr_family domain() const noexcept {
  //     return __dom;
  //   }
  //   std::expected<sock_rw, io_error> accept() noexcept {
  //     struct sockaddr_in addr;
  //     socklen_t addr_len = sizeof(addr);
  //     return sys_call(::accept, __f.fd(), reinterpret_cast<struct sockaddr *>(&addr), &addr_len)
  //       .transform([&](int fd) { return file_type{fd}; })
  //       .and_then([&](auto &&f) {
  //         return sys_fcntl_or_flag{O_CLOEXEC | O_NONBLOCK}(f.fd()).transform([&]() {
  //           return sock_rw{std::move(f), __dom, __protocol, addr.sin_port};
  //         });
  //       });
  //   }
  // };

  // export struct sock {
  // private:
  //   file_type __f;
  //   net_addr_family __dom;
  //   net_protocol __protocol;

  //   struct sockaddr_in __base_addr(int port) const noexcept {
  //     struct sockaddr_storage addr;
  //     addr.ss_family = addr_family_to_sys[static_cast<int>(__dom)];
  //     addr.= port;
  //     return addr;
  //   }

  //   struct sockaddr_in __addr(int port) const noexcept {
  //     auto addr = __base_addr(port);
  //     addr.sin_addr.s_addr = INADDR_ANY;
  //     return addr;
  //   }

  //   std::expected<struct sockaddr_in, io_error> __addr(
  //     std::string_view ip, int port
  //   ) const noexcept {
  //     auto addr = __base_addr(port);
  //     return sys_call_void(inet_aton, ip.begin(), &addr.sin_addr).transform([&]() { return addr;
  //     });
  //   }

  // public:
  //   sock(file_type f, net_addr_family domain, net_protocol protocol) noexcept :
  //     __f{std::move(f)}, __dom{domain}, __protocol(protocol) {}

  //   net_protocol protocol() const noexcept {
  //     return __protocol;
  //   }
  //   net_addr_family domain() const noexcept {
  //     return __dom;
  //   }

  //   std::expected<void, io_error> listen_at(std::string_view addr, int port, int backlog)
  //   noexcept {
  //     return __addr(addr, port)
  //       .and_then([&](auto addr) {
  //         return sys_call_void(
  //           bind, __f.fd(), reinterpret_cast<const struct sockaddr *>(&addr), sizeof(addr)
  //         );
  //       })
  //       .and_then([&]() { return sys_call_void(::listen, __f.fd(), backlog); });
  //   }
  //   std::expected<sock_rw, io_error> connect(std::string_view addr, int port) noexcept {
  //     return __addr(addr, port).and_then([&](auto addr) {
  //       return sys_call_void(
  //                ::connect, __f.fd(), reinterpret_cast<const struct sockaddr *>(&addr),
  //                sizeof(addr)
  //       )
  //         .transform([&]() { return sock_rw{std::move(__f), __dom, __protocol, port}; });
  //     });
  //   }
  //   std::expected<void, io_error> listen(int port, int backlog) noexcept {
  //     struct sockaddr_in addr = __addr(port);
  //     return sys_call_void(
  //              bind, __f.fd(), reinterpret_cast<const struct sockaddr *>(&addr), sizeof(addr)
  //     )
  //       .and_then([&]() { return sys_call_void(::listen, __f.fd(), backlog); });
  //   }
  // };

  // export struct sock_options {
  // private:
  //   net_addr_family __dom;
  //   net_protocol __protocol;

  // public:
  //   sock_options() noexcept : __dom{net_addr_family::LOCAL}, __protocol{net_protocol::TCP} {}
  //   sock_options &domain(net_addr_family dom) noexcept {
  //     __dom = dom;
  //     return *this;
  //   }
  //   sock_options &protocol(net_protocol prot) noexcept {
  //     __protocol = prot;
  //     return *this;
  //   }
  //   sock_options &tcp() noexcept {
  //     return protocol(net_protocol::TCP);
  //   }
  //   sock_options &udp() noexcept {
  //     return protocol(net_protocol::UDP);
  //   }

  //   std::expected<sock, io_error> open() {
  //     return sys_call(
  //              socket,
  //              addr_family_to_sys[static_cast<int>(__dom)],
  //              protocol_to_sys[static_cast<int>(__protocol)],
  //              0
  //     )
  //       .transform([&](auto &&f) { return file_type{f}; })
  //       .and_then([&](auto &&f) {
  //         return sys_fcntl_or_flag{O_NONBLOCK | O_CLOEXEC}(f.fd()).transform([&]() {
  //           return sock{std::move(f), __dom, __protocol};
  //         });
  //       });
  //   }
  // };
}