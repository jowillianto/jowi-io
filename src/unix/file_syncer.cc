module;
#include <cerrno>
#include <cstring>
#include <expected>
export module moderna.io:file_syncer;
import :is_borroweable_file_descriptor;
import :borrowed_file_descriptor;
import :is_file_descriptor;
import :seek_mode;
import :error;

namespace moderna::io {
  export template <is_borroweable_file_descriptor fd_t> struct file_syncer {
    file_syncer(fd_t fd) : __fd{std::move(fd)} {}
    file_syncer(file_syncer &&) noexcept = default;
    file_syncer(const file_syncer &) = default;
    file_syncer &operator=(file_syncer &&) noexcept = default;
    file_syncer &operator=(const file_syncer &) = default;
    std::expected<void, fs_error> sync(bool flush_metadata = false) const noexcept {
      if (flush_metadata) {
        int res = fdatasync(__fd.fd());
        int err_no = errno;
        if (res == -1) {
          return std::unexpected{fs_error{err_no, strerror(err_no)}};
        }
      } else {
        int res = fsync(__fd.fd());
        int err_no = errno;
        if (res == -1) {
          return std::unexpected{fs_error{err_no, strerror(err_no)}};
        }
      }
    }
    borrowed_file_descriptor<native_handle_type<fd_t>> fd() const noexcept {
      return __fd.borrow();
    }
    file_syncer<borrowed_file_descriptor<native_handle_type<fd_t>>> borrow() const noexcept {
      return file_syncer{fd()};
    }

  private:
    fd_t __fd;
  };
}