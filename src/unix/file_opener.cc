module;
#include <sys/file.h>
#include <expected>
#include <filesystem>
#include <unistd.h>
export module moderna.io:file_opener;
import :file_descriptor;
import :open_mode;
import :error;
import :rw_file;

namespace moderna::io {
  struct file_closer {
    void operator()(int fd) const noexcept {
      close(fd);
    }
  };
  using unix_fd = file_descriptor<int, file_closer>;

  export template <open_mode mode = open_mode::read> struct file_opener {
    file_opener(std::filesystem::path path) noexcept : __path{std::move(path)} {}
    file_opener(
      std::filesystem::path path, bool auto_create, int create_permission_bit = 0666
    ) noexcept :
      __path{std::move(path)},
      __auto_create{auto_create}, __permission_bit{create_permission_bit} {}

    file_opener &automatically_create() noexcept {
      __auto_create = true;
      return *this;
    }
    file_opener &with_permission(int permission_bit) noexcept {
      __permission_bit = permission_bit;
      return *this;
    }
    std::expected<rw_file<unix_fd, mode>, fs_error> open() const noexcept {
      int fd = ::open(__path.c_str(), __get_open_flags(), __permission_bit);
      int err_no = errno;
      if (fd == -1) {
        return std::unexpected{fs_error{err_no, strerror(err_no)}};
      }
      return rw_file<unix_fd, mode>{unix_fd{fd}};
    }

  private:
    std::filesystem::path __path;
    bool __auto_create = true;
    int __permission_bit = 0666;

    constexpr int __get_open_flags() const noexcept {
      if constexpr (mode == open_mode::read) {
        return O_RDONLY;
      } else if constexpr (mode == open_mode::write_truncate) {
        if (__auto_create) {
          return O_WRONLY | O_CREAT | O_TRUNC;
        } else {
          return O_WRONLY | O_TRUNC;
        }
      } else if constexpr (mode == open_mode::write_append) {
        if (__auto_create) {
          return O_WRONLY | O_CREAT | O_APPEND;
        } else {
          return O_WRONLY | O_APPEND;
        }
      } else {
        return O_RDWR;
      }
    }
  };
}