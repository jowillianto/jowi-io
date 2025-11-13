module;
#include <sys/socket.h>
#include <cerrno>
#include <chrono>
#include <expected>
#include <optional>
#include <string_view>
export module jowi.io:net_socket;
import jowi.asio;
import :error;
import :sys_call;
import :net_address;
import :buffer;

namespace jowi::io {

  struct TcpSocketSendPoller {
    const FileDescriptor &f;
    std::string_view payload;

    using ValueType = std::expected<size_t, IoError>;

    std::optional<ValueType> poll() const noexcept {
      std::optional<ValueType> res = sys_call(
        ::send,
        f.get_or(-1),
        static_cast<const void *>(payload.data()),
        payload.length(),
        MSG_DONTWAIT
      );
      if (!res->has_value() &&
          (res->error().err_code() == EAGAIN || res->error().err_code() == EWOULDBLOCK)) {
        res.reset();
        return res;
      }
      return res;
    }
  };

  template <WritableBuffer Buffer> struct TcpSocketRecvPoller {
    const FileDescriptor &f;
    Buffer &buf;

    using ValueType = std::expected<void, IoError>;

    std::optional<ValueType> poll() const noexcept {
      std::optional<ValueType> res =
        sys_call(::recv, f.get_or(-1), buf.write_beg(), buf.writable_size(), MSG_DONTWAIT);
      if (!res->has_value() &&
          (res->error().err_code() == EAGAIN || res->error().err_code() == EWOULDBLOCK)) {
        res.reset();
        return res;
      }
      return res;
    }
  };

  /*
   * Definition
   */
  export template <NetAddress Addr> struct TcpSocket {
  private:
    Addr __addr;
    FileDescriptor __f;

  public:
    TcpSocket(Addr addr, FileDescriptor f) : __addr{addr}, __f{std::move(f)} {}

    std::expected<size_t, IoError> send(
      std::string_view v, bool non_blocking = true
    ) const noexcept {
      int flags = non_blocking ? MSG_DONTWAIT : 0;
      return sys_call(
        ::send, __f.get_or(-1), static_cast<const void *>(v.data()), v.length(), flags
      );
    }
    std::expected<void, IoError> recv(
      WritableBuffer auto &buf, bool non_blocking = true
    ) const noexcept {
      int flags = non_blocking ? MSG_DONTWAIT : 0;
      return sys_call(::recv, __f.get_or(-1), buf.write_beg(), buf.writable_size(), flags)
        .transform(BufferWriteMarker{buf});
    }

    const Addr &addr() const noexcept {
      return __addr;
    }
    auto native_handle() const noexcept {
      return __f.get_or(-1);
    }

    /*
     * asynchronous execution
     */
    asio::InfiniteAwaiter<TcpSocketSendPoller> asend(std::string_view v) const noexcept {
      return {__f, v};
    }
    template <class clock_type = std::chrono::steady_clock>
    asio::TimedAwaiter<TcpSocketSendPoller> asend(
      std::string_view v, std::chrono::milliseconds timeout
    ) const noexcept {
      return {timeout, __f, v};
    }
    template <WritableBuffer Buffer>
    asio::InfiniteAwaiter<TcpSocketRecvPoller<Buffer>> arecv(Buffer &buf) const noexcept {
      return {__f, buf};
    }
    template <WritableBuffer Buffer, class clock_type = std::chrono::steady_clock>
    asio::TimedAwaiter<TcpSocketRecvPoller<Buffer>, clock_type> arecv(
      Buffer &buf, std::chrono::milliseconds timeout
    ) const noexcept {
      return {timeout, __f, buf};
    }
  };

  template <NetAddress Addr> struct TcpAcceptPoller {
    const FileDescriptor &f;

    using ValueType = std::expected<TcpSocket<Addr>, IoError>;

    std::optional<ValueType> poll() const noexcept {
      auto addr = Addr::empty();
      auto [raw_addr, len] = addr.sys_addr();
      std::optional<ValueType> res =
        sys_call(::accept, f.get_or(-1), raw_addr, &len)
          .transform(FileDescriptor::manage_default)
          .transform([&](FileDescriptor f) { return TcpSocket{addr, std::move(f)}; });
      if (!res->has_value() &&
          (res->error().err_code() == EAGAIN || res->error().err_code() == EWOULDBLOCK)) {
        res.reset();
        return res;
      }
      return res;
    }
  };

  export template <NetAddress Addr> struct TcpListener {
  private:
    Addr __addr;
    FileDescriptor __f;

    TcpListener(Addr addr, FileDescriptor f) : __addr{addr}, __f{std::move(f)} {}

  public:
    std::optional<std::expected<TcpSocket<Addr>, IoError>> accept() const noexcept {
      return TcpAcceptPoller<Addr>{__f}.poll();
    }
    asio::InfiniteAwaiter<TcpAcceptPoller<Addr>> aaccept() const noexcept {
      return {__f};
    }
    asio::TimedAwaiter<TcpAcceptPoller<Addr>> aaccept(
      std::chrono::milliseconds timeout
    ) const noexcept {
      return {timeout, __f};
    }

    const Addr &addr() const noexcept {
      return __addr;
    }
    auto native_handle() const noexcept {
      return __f.get_or(-1);
    }

    static std::expected<TcpListener, IoError> listen(const Addr &addr, int backlog) {
      auto [raw_addr, len] = addr.sys_addr();
      return sys_call(socket, Addr::addr_family(), SOCK_STREAM, 0)
        .transform(FileDescriptor::manage_default)
        .and_then(sys_fcntl_nonblock)
        .and_then([&](FileDescriptor f) {
          return sys_call_void(bind, f.get_or(-1), raw_addr, len)
            .and_then([&]() { return sys_call_void(::listen, f.get_or(-1), backlog); })
            .transform(FileDescriptorMover{std::move(f)});
        })
        .transform([&](FileDescriptor f) { return TcpListener{addr, std::move(f)}; });
    }
  };

  export template <NetAddress Addr>
  std::expected<TcpListener<Addr>, IoError> create_tcp_listener(const Addr &addr, int backlog) {
    return TcpListener<std::decay_t<decltype(addr)>>::listen(addr, backlog);
  }

  export template <NetAddress Addr>
  std::expected<TcpSocket<Addr>, IoError> tcp_connect(const Addr &addr) {
    auto [raw_addr, len] = addr.sys_addr();
    return sys_call(socket, Addr::addr_family(), SOCK_STREAM, 0)
      .transform(FileDescriptor::manage_default)
      .and_then([&](FileDescriptor f) {
        return sys_call_void(connect, f.get_or(-1), raw_addr, len)
          .transform(FileDescriptorMover{std::move(f)});
      })
      .transform([&](FileDescriptor f) { return TcpSocket{addr, std::move(f)}; });
  }
}