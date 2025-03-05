module;
#include <errno.h>
#include <expected>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <utility>
export module moderna.io:pipe;
import :file;
import :file_descriptor;
import :error;
import :local_file;

namespace moderna::io {
  /*
    pipe
    This represents a pipe between two file descriptors.
  */
  export template <is_file_descriptor write_desc_t, is_file_descriptor read_desc_t> struct pipe {
    file_reader<read_desc_t> reader;
    file_writer<write_desc_t> writer;

    void close_reader() {
      reader.close();
    }
    void close_writer() {
      writer.close();
    }
    void close() {
      close_reader();
      close_writer();
    }
  };
  export struct pipe_creator {
    pipe_creator(bool non_blocking = false) : __non_blocking{non_blocking} {}
    std::expected<pipe<unix_fd, unix_fd>, fs_error> open() const noexcept {
      int pipefd[2];
      int res = ::pipe(pipefd);
      int err_no = errno;
      if (err_no == -1) {
        return std::unexpected{fs_error{err_no, strerror(err_no)}};
      }
      auto reader = file_reader{unix_fd{pipefd[0]}};
      auto writer = file_writer{unix_fd{pipefd[1]}};
      if (__non_blocking) {
        return __mark_fd_as_non_blocking(reader.fd())
          .and_then([&]() { return __mark_fd_as_non_blocking(writer.fd()); })
          .transform([&]() {
            return pipe{std::move(reader), std::move(writer)};
          });
      }
      return pipe{std::move(reader), std::move(writer)};
    }

  private:
    bool __non_blocking;

    int __get_flags() {
      if (__non_blocking) {
        return O_NONBLOCK;
      }
      return 0;
    }

    std::expected<void, fs_error> __mark_fd_as_non_blocking(const is_file_descriptor auto &fd
    ) const {
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
      return {};
    }
  };
}