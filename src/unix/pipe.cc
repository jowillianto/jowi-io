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
import :borrowed_file_descriptor;
import :error;
import :file_reader;
import :file_writer;
import :is_file_descriptor;
import :file_opener;

namespace moderna::io {
  /*
    pipe
    This represents a pipe between two file descriptors.
  */
  export template <is_file_descriptor write_desc_t, is_file_descriptor read_desc_t> struct pipe {
    using borrowed_reader_fd = borrowed_file_descriptor<native_handle_type<write_desc_t>>;
    using borrowed_writer_fd = borrowed_file_descriptor<native_handle_type<read_desc_t>>;
    writable_file<write_desc_t> writer;
    readable_file<read_desc_t> reader;

    borrowed_writer_fd write_fd() const noexcept {
      return writer.borrow();
    }

    borrowed_reader_fd reader_fd() const noexcept {
      return reader.borrow();
    }

    pipe<borrowed_writer_fd, borrowed_reader_fd> borrow() const noexcept {
      return pipe<borrowed_writer_fd, borrowed_reader_fd>{writer.borrow(), reader.borrow()};
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
      auto read_f = readable_file{unix_fd{pipefd[0]}};
      auto write_f = writable_file{unix_fd{pipefd[1]}};
      if (__non_blocking) {
        return __mark_fd_as_non_blocking(read_f.fd())
          .and_then([&]() { return __mark_fd_as_non_blocking(write_f.fd()); })
          .transform([&]() {
            return pipe{std::move(write_f), std::move(read_f)};
          });
      }
      return pipe{std::move(write_f), std::move(read_f)};
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