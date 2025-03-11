module;
#include <sys/stat.h>
#include <dirent.h>
#include <expected>
#include <filesystem>
export module moderna.io:dir;
import :file;
import :error;

namespace moderna::io {
  export struct directory_closer {
    void operator()(DIR *dir) const noexcept {
      closedir(dir);
    }
  };
  using directory_ptr_t = std::unique_ptr<DIR, directory_closer>;
  export struct directory {
    directory(directory_ptr_t dir) noexcept : __dir{std::move(dir)} {}
    directory(directory &&) noexcept = default;
    directory &operator=(directory &&) noexcept = default;

  private:
    directory_ptr_t __dir;
  };
  export struct dir_opener {
    dir_opener(std::filesystem::path path) noexcept : __path{std::move(path)} {}
    std::expected<directory, fs_error> open() const noexcept {
      return __open_dir()
        .or_else([&](int err_no) {
          using expected_t = std::expected<DIR *, int>;
          if (err_no != ENOENT || !__auto_create) {
            return expected_t{std::unexpected{err_no}};
          }
          int res = mkdir(__path.c_str(), __permission_bit);
          err_no = errno;
          if (res == -1) {
            return expected_t{std::unexpected{err_no}};
          }
          return __open_dir();
        })
        .transform_error([](int err_no) {
          return fs_error{err_no, strerror(err_no)};
        })
        .transform([](DIR *dir) { return directory{directory_ptr_t{dir}}; });
    }
    dir_opener &automatically_create() noexcept {
      __auto_create = true;
      return *this;
    }
    dir_opener &with_permission(int permission_bit) noexcept {
      __permission_bit = permission_bit;
      return *this;
    }

  private:
    std::filesystem::path __path;
    bool __auto_create = true;
    int __permission_bit = 0755;

    std::expected<DIR *, int> __open_dir() const noexcept {
      DIR *dir = opendir(__path.c_str());
      int err_no = errno;
      if (dir == nullptr) {
        return std::unexpected{err_no};
      }
      return dir;
    }
  };
}