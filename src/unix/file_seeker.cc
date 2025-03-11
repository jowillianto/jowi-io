module;
#include <sys/file.h>
#include <cerrno>
#include <cstring>
#include <expected>
#include <unistd.h>
export module moderna.io:file_seeker;
import :is_borroweable_file_descriptor;
import :borrowed_file_descriptor;
import :is_file_descriptor;
import :seek_mode;
import :error;

namespace moderna::io {
  export template <is_borroweable_file_descriptor fd_t> struct file_seeker {
    file_seeker(fd_t fd) noexcept : __fd{std::move(fd)} {}
    file_seeker(file_seeker &&) noexcept = default;
    file_seeker(const file_seeker &) = default;
    file_seeker &operator=(file_seeker &&) noexcept = default;
    file_seeker &operator=(const file_seeker &) = default;

    // Functions
    std::expected<off_t, fs_error> seek(size_t offset, seek_mode mode = seek_mode::start)
      const noexcept {
      off_t cur_off = lseek(__fd.fd(), offset, __get_seek_flags(mode));
      int err_no = errno;
      if (cur_off == -1) {
        return std::unexpected{fs_error{err_no, strerror(err_no)}};
      }
      return cur_off;
    }
    std::expected<off_t, fs_error> tell() const noexcept {
      return seek(0, seek_mode::current);
    }
    std::expected<off_t, fs_error> seek_begin() const noexcept {
      return seek(0, seek_mode::start);
    }
    std::expected<off_t, fs_error> seek_end() const noexcept {
      return seek(0, seek_mode::end);
    }

    borrowed_file_descriptor<native_handle_type<fd_t>> fd() const noexcept {
      return __fd.borrow();
    }
    file_seeker<borrowed_file_descriptor<native_handle_type<fd_t>>> borrow() const noexcept {
      return file_seeker{fd()};
    }

  private:
    fd_t __fd;
    int __get_seek_flags(seek_mode mode) const {
      if (mode == seek_mode::start) {
        return SEEK_SET;
      } else if (mode == seek_mode::current) {
        return SEEK_CUR;
      } else {
        return SEEK_END;
      }
    }
  };
}