module;
#include <sys/poll.h>
#include <chrono>
#include <expected>
#include <fcntl.h>
#include <string_view>
#include <unistd.h>
export module moderna.io:sys_file;
import moderna.generic;
import :error;
import :sys_util;
import :is_file;

namespace moderna::io {
  std::expected<off_t, io_error> sys_seek(int fd, seek_mode s, off_t offset) noexcept {
    int seek_bit = 0;
    switch (s) {
      case seek_mode::START:
        seek_bit = SEEK_SET;
        break;
      case seek_mode::CURRENT:
        seek_bit = SEEK_CUR;
        break;
      case seek_mode::END:
        seek_bit = SEEK_END;
        break;
    }
    return sys_call(lseek, fd, offset, seek_bit);
  };
  std::expected<off_t, io_error> sys_truncate(int fd, off_t size) noexcept {
    return sys_call(ftruncate, fd, size);
  }
  std::expected<size_t, io_error> sys_write(int fd, std::string_view v) noexcept {
    return sys_call(write, fd, v.data(), v.length());
  };
  std::expected<void, io_error> sys_sync(int fd) noexcept {
    return sys_call_void(fsync, fd);
  };
  template <size_t N>
  std::expected<void, io_error> sys_read(int fd, generic::fixed_string<N> &buf) noexcept {
    return sys_call(read, fd, buf.begin(), N).transform([&](int read_size) {
      buf.unsafe_set_length(read_size);
    });
  }
  struct sys_file_poller {
  private:
    short __evts;
    std::chrono::milliseconds __timeout;

  public:
    sys_file_poller(
      short evts = 0, std::chrono::milliseconds timeout = std::chrono::milliseconds{0}
    ) : __evts{evts}, __timeout{timeout} {}
    sys_file_poller &timeout(std::chrono::milliseconds tout) noexcept {
      __timeout = tout;
      return *this;
    }
    sys_file_poller &poll_in() noexcept {
      __evts = __evts | POLLIN;
      return *this;
    }
    sys_file_poller &poll_out() noexcept {
      __evts = __evts | POLLOUT;
      return *this;
    }
    std::expected<bool, io_error> operator()(int fd) noexcept {
      struct pollfd conf{fd, __evts, 0};
      return sys_call(poll, &conf, 1, __timeout.count()).transform([](int res) {
        return res != 0;
      });
    }

    static sys_file_poller write_poller(
      std::chrono::milliseconds timeout = std::chrono::milliseconds{0}
    ) noexcept {
      return sys_file_poller{}.timeout(timeout).poll_out();
    }
    static sys_file_poller read_poller(
      std::chrono::milliseconds timeout = std::chrono::milliseconds{0}
    ) noexcept {
      return sys_file_poller{}.timeout(timeout).poll_in();
    }
  };
  struct sys_fcntl {
  private:
    int __op;
    int __args;

  public:
    sys_fcntl(int op = F_GETFD, int args = 0) noexcept : __op{op}, __args{args} {}
    sys_fcntl &op(int op) noexcept {
      __op = op;
      return *this;
    }
    sys_fcntl &args(int args) noexcept {
      __args = args;
      return *this;
    }
    std::expected<int, io_error> operator()(int fd) const noexcept {
      return sys_call(fcntl, fd, __op, __args);
    }
  };
  struct sys_fcntl_or_flag {
    int __or_flags;

  public:
    sys_fcntl_or_flag(int flags = 0) noexcept : __or_flags{flags} {}
    sys_fcntl_or_flag &flags_or(int flags) noexcept {
      __or_flags = flags;
      return *this;
    }
    std::expected<void, io_error> operator()(int fd) const noexcept {
      return sys_fcntl{F_GETFD}(fd).and_then([&](int flags) {
        return sys_fcntl{F_GETFD, flags | __or_flags}(fd).transform([](auto &&) {});
      });
    }
  };
}