module;
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <expected>
#include <filesystem>
#include <unistd.h>
export module moderna.io:file_opener;
import :file_descriptor;
import :open_mode;
import :error;
import :generic_file;
import :fd_type;
import :file_unblocker;

namespace moderna::io {
  namespace fs = std::filesystem;

  constexpr int get_open_flags(open_mode mode, bool auto_create) noexcept {
    if (mode == open_mode::read) {
      return O_RDONLY;
    } else if (mode == open_mode::write_truncate) {
      if (auto_create) {
        return O_WRONLY | O_CREAT | O_TRUNC;
      } else {
        return O_WRONLY | O_TRUNC;
      }
    } else if (mode == open_mode::write_append) {
      if (auto_create) {
        return O_WRONLY | O_CREAT | O_APPEND;
      } else {
        return O_WRONLY | O_APPEND;
      }
    } else {
      return O_RDWR;
    }
  }

  /*
    Opens a local file.
  */
  export std::expected<generic_file<file_type, true, true, true, true, true>, fs_error> open_file(
    const fs::path &p, open_mode mode, bool auto_create = true
  ) {
    int open_flags = get_open_flags(mode, auto_create);
    int fd = open(p.c_str(), get_open_flags(mode, auto_create), 0666);
    int err_no = errno;
    if (fd == -1) {
      return std::unexpected{fs_error::make(err_no, strerror(err_no))};
    }
    return generic_file<file_type, true, true, true, true, true>{file_type{fd}};
  }

  /*
    Opens a local file
  */
  export template <open_mode mode> auto open_file(const fs::path &p, bool auto_create = true) {
    constexpr bool is_readable =
      mode != open_mode::write_append || mode != open_mode::write_truncate;
    constexpr bool is_writable = mode != open_mode::read;
    return open_file(p, mode, auto_create).transform([](auto &&file) {
      return generic_file<file_type, is_readable, is_writable, true, true, true>{std::move(file).fd(
      )};
    });
  }

  /*
    Opens a pipe
  */
  export std::expected<pipe_file<file_type>, fs_error> open_pipe(
    bool non_blocking = true
  ) noexcept {
    std::array<int, 2> pipe_fd;
    int res = pipe(pipe_fd.data());
    int err_no = errno;
    if (err_no == -1) {
      return std::unexpected{fs_error::make(err_no, strerror(err_no))};
    }
    auto pipe =
      pipe_file<file_type>{.reader{file_type{pipe_fd[0]}}, .writer{file_type{pipe_fd[1]}}};
    if (!non_blocking) {
      return std::move(pipe);
    }
    return pipe.reader.apply_operation(file_unblocker{}).and_then([&]() {
      return pipe.writer.apply_operation(file_unblocker{}).transform([&]() {
        return std::move(pipe);
      });
    });
  }

  export std::expected<tcp_host<file_type>, fs_error> listen(
    int port, int backlog = 50, bool non_blocking = true
  ) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    int err_no = errno;
    if (fd == -1) {
      return std::unexpected{fs_error::make(err_no, strerror(err_no))};
    }
    file_type controlled_fd{fd};
    // Bind Socket
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    int bind_res = ::bind(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
    err_no = errno;
    if (bind_res == -1) {
      return std::unexpected{fs_error::make(err_no, strerror(err_no))};
    }
    int listen_res = ::listen(fd, backlog);
    err_no = errno;
    if (listen_res == -1) {
      return std::unexpected{fs_error::make(err_no, strerror(err_no))};
    }
    auto host =
      tcp_host<file_type>{.file{std::move(controlled_fd)}, .host{inet_ntoa(addr.sin_addr), port}};
    if (!non_blocking) {
      return std::move(host);
    }
    return host.file.apply_operation(file_unblocker{}).transform([&]() { return std::move(host); });
  }

  export std::expected<tcp_connection<file_type>, fs_error> connect(
    std::string_view ip, int port, bool non_blocking = true
  ) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    int err_no = errno;
    if (fd == -1) {
      return std::unexpected{fs_error::make(err_no, strerror(err_no))};
    }
    file_type controlled_fd{fd};
    // Bind Socket
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    int connect_res = ::connect(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
    err_no = errno;
    if (connect_res == -1) {
      return std::unexpected{fs_error::make(err_no, strerror(err_no))};
    }
    auto connection = tcp_connection<file_type>{
      .file{std::move(controlled_fd)},
      .host{.ip{inet_ntoa(addr.sin_addr)}, .port = addr.sin_port},
      .client{.ip{std::string{ip}}, .port = port}
    };
    if (!non_blocking) {
      return std::move(connection);
    }
    return connection.file.apply_operation(file_unblocker{}).transform([&]() {
      return std::move(connection);
    });
  }

  export std::expected<tcp_connection<file_type>, fs_error> accept(
    const tcp_host<file_type> &host
  ) {
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(addr);
    int accept_fd = ::accept(
      get_native_handle(host.file), reinterpret_cast<struct sockaddr *>(&addr), &addr_size
    );
    int err_no = errno;
    if (accept_fd == -1) {
      return std::unexpected{fs_error::make(err_no, strerror(err_no))};
    }
    return tcp_connection<file_type>{
      .file{file_type{accept_fd}},
      .host{host.host},
      .client{.ip{inet_ntoa(addr.sin_addr)}, .port = addr.sin_port}
    };
  }
}