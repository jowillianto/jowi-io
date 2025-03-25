module;
#include <sys/poll.h>
#include <cerrno>
#include <expected>
#include <optional>
#include <unistd.h>
export module moderna.io:file_poller;
import :error;
import :is_file_descriptor;
import :adapter;
namespace moderna::io {
  export struct file_poller {
    file_poller(short events, int timeout) : __events{events}, __timeout{timeout} {}

    std::expected<std::optional<short>, fs_error> poll(is_file_descriptor auto fd) {
      struct pollfd conf {
        fd.fd(), __events, 0
      };
      int res = ::poll(&conf, 1, __timeout);
      int err_no = errno;
      if (res == -1) {
        return std::unexpected{cp_adapter::make_error(err_no)};
      } else if (res == 0) {
        return std::nullopt;
      }
      return conf.revents;
    }

    std::expected<bool, fs_error> poll_binary(is_file_descriptor auto fd) {
      return poll(std::move(fd)).transform([](auto &&v) { return v.has_value(); });
    }

    static file_poller make_write_poller(int timeout = 0) {
      return file_poller{POLLWRITE, timeout};
    }
    static file_poller make_read_poller(int timeout = 0) {
      return file_poller{POLLIN, timeout};
    }

  private:
    short __events;
    int __timeout;
  };
}