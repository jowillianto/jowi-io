module;
#include <cerrno>
#include <expected>
#include <optional>
#include <unistd.h>
export module moderna.io:adapter;
import :seek_mode;
import :error;

namespace moderna::io {
  struct cp_adapter {
    static std::expected<int, int> read(int fd, void *cont, size_t nbytes) {
      int res = ::read(fd, cont, nbytes);
      int err_no = errno;
      if (res == -1) {
        return std::unexpected{err_no};
      } else {
        return res;
      }
    }
    static std::expected<off_t, int> seek(int fd, off_t offset, seek_mode mode) {
      off_t res = lseek(fd, offset, __get_seek_flags(mode));
      int err_no = errno;
      if (res == -1) {
        return std::unexpected{err_no};
      } else {
        return res;
      }
    }
    static std::expected<int, int> write(int fd, const void *cont, size_t nbytes) {
      int res = ::write(fd, cont, nbytes);
      int err_no = errno;
      if (res == -1) {
        return std::unexpected{err_no};
      } else {
        return res;
      }
    }
    static fs_error make_error(int err) {
      return fs_error{err, strerror(err)};
    }
    static int __get_seek_flags(seek_mode mode) {
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