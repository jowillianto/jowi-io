module;
#include <sys/fcntl.h>
#include <cerrno>
#include <cstring>
#include <expected>
#include <fcntl.h>
export module moderna.io:file_unblocker;
import :error;
import :is_file;

namespace moderna::io {
  export struct file_unblocker {
    using result_type = void;
    bool non_blocking = true;
    bool close_on_exec = false;
    constexpr int get_flags(int current_flags) const noexcept {
      if (non_blocking) {
        current_flags |= O_NONBLOCK;
      } else {
        current_flags &= (~O_NONBLOCK);
      }
      if (close_on_exec) {
        current_flags |= O_CLOEXEC;
      }
      return current_flags;
    }
    std::expected<void, fs_error> operator()(const is_basic_file auto &file) const noexcept {
      int current_flags = fcntl(get_native_handle(file), F_GETFD, 0);
      int err_no = errno;
      if (current_flags == -1) {
        return std::unexpected{fs_error::make(err_no, strerror(err_no))};
      }
      int set_res = fcntl(get_native_handle(file), F_SETFD, get_flags(current_flags));
      err_no = errno;
      if (set_res == -1) {
        return std::unexpected{fs_error::make(err_no, strerror(err_no))};
      }
      return {};
    }
  };
}