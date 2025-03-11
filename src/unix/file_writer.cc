module;
#include <sys/file.h>
#include <sys/poll.h>
#include <cerrno>
#include <expected>
#include <string_view>
#include <unistd.h>
export module moderna.io:file_writer;
import :is_borroweable_file_descriptor;
import :is_file_descriptor;
import :borrowed_file_descriptor;
import :error;

namespace moderna::io {
  export template <is_borroweable_file_descriptor fd_t> struct file_writer {
    file_writer(fd_t fd) noexcept : __fd{std::move(fd)} {}
    file_writer(file_writer &&) noexcept = default;
    file_writer(const file_writer &) = default;
    file_writer &operator=(file_writer &&) noexcept = default;
    file_writer &operator=(const file_writer &) = default;
    std::expected<void, fs_error> write(std::string_view data) const noexcept {
      size_t total_written_bytes = 0;
      while (total_written_bytes < data.length()) {
        int written_bytes = ::write(
          __fd.fd(),
          static_cast<const void *>(data.begin() + total_written_bytes),
          data.length() - total_written_bytes
        );
        int err_no = errno;
        if (written_bytes == -1) {
          return std::unexpected{fs_error{err_no, strerror(err_no)}};
        }
        total_written_bytes += written_bytes;
      }
      return {};
    }
    std::expected<bool, fs_error> is_writable() const noexcept {
      struct pollfd poll_fd = {.fd = __fd.fd(), .events = POLLOUT};
      int res = ::poll(&poll_fd, 1, 0);
      int err_no = errno;
      if (res == -1) {
        return std::unexpected{fs_error{err_no, strerror(err_no)}};
      }
      return res != 0;
    }
    borrowed_file_descriptor<native_handle_type<fd_t>> fd() const noexcept {
      return __fd.borrow();
    }
    file_writer<borrowed_file_descriptor<native_handle_type<fd_t>>> borrow() const noexcept {
      return file_writer{fd()};
    }

  private:
    fd_t __fd;
  };
}