module;
#include <sys/poll.h>
#include <cerrno>
#include <cstring>
#include <expected>
#include <unistd.h>
export module moderna.io:file_poller;
import :error;
import :is_file;
namespace moderna::io {
  export struct file_poller {
    using result_type = bool;
    short events;
    int timeout;

    std::expected<result_type, fs_error> operator()(const is_basic_file auto &file) const noexcept {
      struct pollfd conf{get_native_handle(file), events, 0};
      int res = ::poll(&conf, 1, timeout);
      int err_no = errno;
      if (res == -1) {
        return std::unexpected{fs_error::make(err_no, strerror(err_no))};
      } else if (res == 0) {
        return false;
      }
      return true;
    }

    static file_poller write_poller(int timeout = 0) {
      return file_poller{POLLWRITE, timeout};
    }
    static file_poller read_poller(int timeout = 0) {
      return file_poller{POLLIN, timeout};
    }
  };
}