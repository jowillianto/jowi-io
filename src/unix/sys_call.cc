module;
#include <sys/poll.h>
#include <sys/socket.h>
#include <cerrno>
#include <expected>
#include <fcntl.h>
#include <optional>
#include <string_view>
#include <unistd.h>
export module jowi.io:sys_call;
import :error;
import :fd_type;
import :file;
import :buffer;

namespace jowi::io {
  /**
   * @brief invokes a sys call function and returns IoError when the return value is less than 0.
   * Call errno to get the error.
   * @tparam F Callable type accepting the provided `Args`.
   * @tparam Args Argument types forwarded to the callable.
   * @param f Callable representing the syscall.
   * @param args Arguments forwarded to the callable.
   * @return Expected with the syscall result or `IoError` when the call fails.
   */
  template <class F, class... Args> requires(std::invocable<F, Args...>)
  std::expected<std::invoke_result_t<F, Args...>, IoError> sys_call(
    F &&f, Args &&...args
  ) noexcept {
    auto res = std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    int err_no = errno;
    if (res == -1) {
      return std::unexpected{IoError::str_error(err_no)};
    }
    return res;
  }
  /**
   * @brief sys_call that discards the return value.
   * @tparam F Callable type accepting the provided `Args`.
   * @tparam Args Argument types forwarded to the callable.
   * @param f Callable representing the syscall.
   * @param args Arguments forwarded to the callable.
   * @return Empty expected on success or `IoError` on failure.
   */
  template <class F, class... Args> requires(std::invocable<F, Args...>)
  std::expected<void, IoError> sys_call_void(F &&f, Args &&...args) noexcept {
    return sys_call(std::forward<F>(f), std::forward<Args>(args)...).transform([](auto &&) {});
  }
  /**
   * @brief reads bytes into a writable buffer
   * @param fd native file descriptor
   * @param buf writable buffer receiving the data
   */
  export std::expected<void, IoError> sys_read(
    const FileDescriptor &fd, WritableBuffer auto &buf
  ) noexcept {
    return sys_call(read, fd.get_or(-1), buf.write_beg(), buf.writable_size())
      .transform(BufferWriteMarker{buf});
  }

  export template <WritableBuffer buf_type> struct SysReadPoller {
  private:
    const FileDescriptor &__fd;
    buf_type &__buf;

  public:
    using ValueType = std::expected<void, IoError>;
    SysReadPoller(const FileDescriptor &fd, buf_type &buf) : __fd{fd}, __buf{buf} {}
    std::optional<ValueType> poll() noexcept {
      auto res = sys_read(__fd, __buf);
      if (!res && (res.error().err_code() == EWOULDBLOCK || res.error().err_code() == EAGAIN)) {
        return std::nullopt;
      }
      return res;
    }
  };

  /**
   * @brief Writes data to the supplied descriptor.
   * @param fd Native file descriptor.
   * @param v String view containing the bytes to write.
   * @return Number of bytes written or IO error.
   */
  std::expected<uint64_t, IoError> sys_write(
    const FileDescriptor &fd, std::string_view v
  ) noexcept {
    return sys_call(write, fd.get_or(-1), v.data(), v.length());
  };

  export struct SysWritePoller {
  private:
    const FileDescriptor &__fd;
    std::string_view __v;

  public:
    using ValueType = std::expected<size_t, IoError>;
    SysWritePoller(const FileDescriptor &fd, std::string_view v) : __fd{fd}, __v{v} {}

    std::optional<ValueType> poll() noexcept {
      auto res = sys_write(__fd, __v);
      if (!res && (res.error().err_code() == EWOULDBLOCK || res.error().err_code() == EAGAIN)) {
        return std::nullopt;
      }
      return res;
    }
  };

  /**
   * Most of the time, only write and read system call can block, other system call will not block.
   * Hence, we are free to do with them as we wish.
   */
  /**
   * @brief Performs a seek operation using the chosen origin.
   * @param fd Native file descriptor.
   * @param s Seek origin mode.
   * @param offset Offset applied relative to the origin.
   * @return New file position or IO error.
   */
  std::expected<off_t, IoError> sys_seek(
    const FileDescriptor &fd, SeekMode s, off_t offset
  ) noexcept {
    int seek_bit = 0;
    switch (s) {
      case SeekMode::START:
        seek_bit = SEEK_SET;
        break;
      case SeekMode::CURRENT:
        seek_bit = SEEK_CUR;
        break;
      case SeekMode::END:
        seek_bit = SEEK_END;
        break;
    }
    return sys_call(lseek, fd.get_or(-1), offset, seek_bit);
  };

  /**
   * @brief Truncates or extends a file to the specified size.
   * @param fd Native file descriptor.
   * @param size New file size in bytes.
   * @return Result of the truncate operation or IO error.
   */
  std::expected<off_t, IoError> sys_truncate(const FileDescriptor &fd, off_t size) noexcept {
    return sys_call(ftruncate, fd.get_or(-1), size);
  }

  /**
   * @brief Synchronizes file contents to stable storage.
   * @param fd Native file descriptor.
   * @return Success or IO error.
   */
  std::expected<void, IoError> sys_sync(const FileDescriptor &fd) noexcept {
    return sys_call_void(fsync, fd.get_or(-1));
  };

  /**
   * fcntl
   */
  std::expected<int, IoError> sys_fcntl(const FileDescriptor &fd, int op, int args) noexcept {
    return sys_call(fcntl, fd.get_or(-1), op, args);
  }

  /**
   * fcntl set flags
   */
  std::expected<int, IoError> sys_fcntl_set_f_or(const FileDescriptor &fd, int flags) noexcept {
    return sys_fcntl(fd, F_GETFD, 0).and_then([&](int cur_flags) {
      return sys_fcntl(fd, F_SETFD, cur_flags | flags);
    });
  }

  std::expected<FileDescriptor, IoError> sys_fcntl_nonblock(FileDescriptor fd) noexcept {
    return sys_fcntl_set_f_or(fd, O_CLOEXEC | O_NONBLOCK).transform([&](auto) {
      return std::move(fd);
    });
  }

  std::expected<void, IoError> sys_fcntl_nonblock_void(const FileDescriptor &fd) noexcept {
    return sys_fcntl_set_f_or(fd, O_CLOEXEC | O_NONBLOCK).transform([&](auto) {});
  }

  /**
   * poll
   */
  std::expected<bool, IoError> sys_poll_in(const FileDescriptor &fd) noexcept {
    struct pollfd conf{fd.get_or(-1), POLLIN, 0};
    return sys_call(poll, &conf, 1, 0).transform([](int n_events) { return n_events == 1; });
  }
  std::expected<bool, IoError> sys_poll_out(const FileDescriptor &fd) noexcept {
    struct pollfd conf{fd.get_or(-1), POLLOUT, 0};
    return sys_call(poll, &conf, 1, 0).transform([](int n_events) { return n_events == 1; });
  }

  /**
   * Pipe
   */
  std::expected<std::pair<FileDescriptor, FileDescriptor>, IoError> sys_pipe(bool non_blocking) {
    std::array<int, 2> pipe_fd;
    return sys_call(pipe, pipe_fd.data()).and_then([&](auto &&) {
      FileDescriptor r_fd = FileDescriptor::manage_default(pipe_fd[0]);
      FileDescriptor w_fd = FileDescriptor::manage_default(pipe_fd[1]);
      if (non_blocking) {
        return sys_fcntl_set_f_or(r_fd, O_NONBLOCK | O_CLOEXEC).and_then([&](auto) {
          return sys_fcntl_set_f_or(w_fd, O_NONBLOCK | O_CLOEXEC).transform([&](auto) {
            return std::pair{std::move(r_fd), std::move(w_fd)};
          });
        });
      } else {
        return std::expected<std::pair<FileDescriptor, FileDescriptor>, IoError>{
          std::pair{std::move(r_fd), std::move(w_fd)}
        };
      }
    });
  }
}
