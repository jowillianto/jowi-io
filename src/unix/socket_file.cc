module;
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <expected>
#include <fcntl.h>
#include <string.h>
#include <string>
#include <utility>
export module moderna.io:socket_file;
import :file;
import :local_file;
import :error;
import :file_descriptor;

namespace moderna::io {
  /*
    tcp_connection
    this represents a tcp connection between the client and the server. 
  */
  export template <is_file_descriptor desc_t> struct tcp_connection {
    tcp_connection(desc_t fd, struct sockaddr_in addr) noexcept :
      __fd{std::move(fd)}, __addr{std::move(addr)} {}
    tcp_connection(tcp_connection &&) noexcept = default;
    tcp_connection(const tcp_connection &) = default;
    tcp_connection &operator=(tcp_connection &&) noexcept = default;
    tcp_connection &operator=(const tcp_connection &) = default;

    is_readable<std::string> auto get_reader() const noexcept {
      return file_reader{__fd.borrow()};
    }
    is_writable<std::string_view> auto get_writer() const noexcept {
      return file_writer{__fd.borrow()};
    }

    /*
      Returns the IP of the other end of the connection
    */
    std::string ip() const {
      return inet_ntoa(__addr.sin_addr);
    }

    /*
      Returns the port of the other end of the connection
    */
    int port() const {
      return htons(__addr.sin_port);
    }

    static std::expected<tcp_connection, fs_error> create(
      desc_t fd, struct sockaddr_in addr, bool non_blocking
    ) {
      if (non_blocking) {
        int flags = fcntl(fd.fd(), F_GETFL);
        int err_no = errno;
        if (flags == -1) {
          return std::unexpected{fs_error{err_no, strerror(err_no)}};
        }
        int flag_set_res = fcntl(fd.fd(), F_SETFL, flags | O_NONBLOCK);
        err_no = errno;
        if (flag_set_res == -1) {
          return std::unexpected{fs_error{err_no, strerror(err_no)}};
        }
      }
      return tcp_connection{std::move(fd), std::move(addr)};
    }

  private:
    desc_t __fd;
    struct sockaddr_in __addr;
  };
  /*
    tcp_listener
    This represents a tcp listener that listens for incoming connections. This struct 
    satisfies the file_opener concept since it opens a connection. 
  */
  export template <is_file_descriptor desc_t> struct tcp_listener {
    tcp_listener(desc_t fd, struct sockaddr_in addr, bool non_blocking) noexcept :
      __fd{std::move(fd)}, __addr{std::move(addr)} {}
    tcp_listener(tcp_listener &&) noexcept = default;
    tcp_listener(const tcp_listener &) = default;
    tcp_listener &operator=(tcp_listener &&) noexcept = default;
    tcp_listener &operator=(const tcp_listener &) = default;

    std::expected<tcp_connection<unix_fd>, fs_error> open() const noexcept {
      return accept();
    }
    std::expected<tcp_connection<unix_fd>, fs_error> accept() const noexcept {
      struct sockaddr_in addr;
      socklen_t addr_size = sizeof(addr);
      int fd = ::accept(__fd.fd(), reinterpret_cast<struct sockaddr *>(&addr), &addr_size);
      int err_no = errno;
      if (fd == -1) {
        return std::unexpected{fs_error{err_no, strerror(err_no)}};
      }
      return tcp_connection<unix_fd>::create(unix_fd{fd}, addr, __non_blocking);
    }

    /*
      Returns the ip of the current connection
    */
    std::string ip() const {
      return inet_ntoa(__addr.sin_addr);
    }

    /*
      Returns the ip of the other end of the connection.
    */
    int port() const {
      return ntohs(__addr.sin_port);
    }

  private:
    desc_t __fd;
    bool __non_blocking;
    struct sockaddr_in __addr;
  };

  /*
    tcp_host
    creates a listener
  */
  export struct tcp_host {
    tcp_host(int port, int backlog = 10, bool non_blocking = false) :
      __port{port}, __backlog{backlog}, __non_blocking{non_blocking} {}
    std::expected<tcp_listener<unix_fd>, fs_error> open() {
      return __create().and_then([this](auto &&fd) { return __bind(std::move(fd)); }
      ).and_then([this](auto &&p) { return __listen(std::move(p.first), std::move(p.second)); });
    }

    /*
      Returns the desired port.
    */
    int port() const noexcept {
      return __port;
    }

  private:
    int __port;
    int __backlog;
    bool __non_blocking;

    std::expected<unix_fd, fs_error> __create() const noexcept {
      int fd = ::socket(AF_INET, SOCK_STREAM, 0);
      int err_no = errno;
      if (fd == -1) {
        return std::unexpected{fs_error{err_no, strerror(err_no)}};
      }
      return unix_fd{fd};
    }
    std::expected<std::pair<unix_fd, struct sockaddr_in>, fs_error> __bind(unix_fd fd
    ) const noexcept {
      struct sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = INADDR_ANY;
      addr.sin_port = htons(__port);
      int res = ::bind(fd.fd(), reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
      int err_no = errno;
      if (res == -1) {
        return std::unexpected{fs_error{err_no, strerror(err_no)}};
      }
      return std::pair{std::move(fd), std::move(addr)};
    }
    std::expected<tcp_listener<unix_fd>, fs_error> __listen(unix_fd fd, struct sockaddr_in addr)
      const noexcept {
      int res = ::listen(fd.fd(), __backlog);
      int err_no = errno;
      if (res == -1) {
        return std::unexpected{fs_error{err_no, strerror(err_no)}};
      }
      return tcp_listener{std::move(fd), std::move(addr), __non_blocking};
    }
  };

  /*
    tcp_connector
    creates a tcp_connection. I.e. connects to a tcp_server. 
  */
  export struct tcp_connector {
    tcp_connector(std::string ip, int port, bool non_blocking = false) :
      __ip{std::move(ip)}, __port{port}, __non_blocking{non_blocking} {}

    std::expected<tcp_connection<unix_fd>, fs_error> open() const noexcept {
      return create().and_then([this](auto &&fd) { return connect(std::move(fd)); });
    }

    std::expected<unix_fd, fs_error> create() const noexcept {
      int fd = ::socket(AF_INET, SOCK_STREAM, 0);
      int err_no = errno;
      if (fd == -1) {
        return std::unexpected{fs_error{err_no, strerror(err_no)}};
      }
      return unix_fd{fd};
    }
    std::expected<tcp_connection<unix_fd>, fs_error> connect(unix_fd fd) const noexcept {
      struct sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_port = htons(__port);
      int cv_res = inet_pton(AF_INET, __ip.c_str(), &addr.sin_addr);
      if (cv_res == 0 || cv_res == -1) {
        return std::unexpected{fs_error{errno, strerror(errno)}};
      }
      int res = ::connect(fd.fd(), reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
      int err_no = errno;
      if (res == -1) {
        return std::unexpected{fs_error{err_no, strerror(err_no)}};
      }
      return tcp_connection<unix_fd>::create(std::move(fd), addr, __non_blocking);
    }

    int port() const noexcept {
      return __port;
    }

    std::string_view ip() const noexcept {
      return __ip;
    }

  private:
    std::string __ip;
    int __port;
    bool __non_blocking;
  };
}