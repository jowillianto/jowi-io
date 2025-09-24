module;
#include <sys/poll.h>
#include <chrono>
#include <expected>
#include <fcntl.h>
#include <string_view>
#include <unistd.h>
export module jowi.io:sys_file;
import jowi.generic;
import :error;
import :sys_util;
import :is_file;
import :buffer;

/**
 * @file unix/sys_file.cc
 * @brief POSIX file descriptor helpers including polling and buffered IO.
 */

namespace jowi::io {
  /**
   * @brief Performs a seek operation using the chosen origin.
   * @param fd Native file descriptor.
   * @param s Seek origin mode.
   * @param offset Offset applied relative to the origin.
   * @return New file position or IO error.
   */
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
  /**
   * @brief Truncates or extends a file to the specified size.
   * @param fd Native file descriptor.
   * @param size New file size in bytes.
   * @return Result of the truncate operation or IO error.
   */
  std::expected<off_t, io_error> sys_truncate(int fd, off_t size) noexcept {
    return sys_call(ftruncate, fd, size);
  }
  /**
   * @brief Writes data to the supplied descriptor.
   * @param fd Native file descriptor.
   * @param v String view containing the bytes to write.
   * @return Number of bytes written or IO error.
   */
  std::expected<size_t, io_error> sys_write(int fd, std::string_view v) noexcept {
    return sys_call(write, fd, v.data(), v.length());
  };
  /**
   * @brief Synchronizes file contents to stable storage.
   * @param fd Native file descriptor.
   * @return Success or IO error.
   */
  std::expected<void, io_error> sys_sync(int fd) noexcept {
    return sys_call_void(fsync, fd);
  };
  /**
   * @brief Reads bytes into a writable buffer.
   * @param fd Native file descriptor.
   * @param buf Writable buffer receiving the data.
   * @return Success or IO error.
   */
  std::expected<void, io_error> sys_read(int fd, is_writable_buffer auto &buf) noexcept {
    return sys_call(read, fd, buf.write_beg(), buf.writable_size()).transform([&](int read_size) {
      buf.finish_write(read_size);
    });
  }
  /**
   * @brief Helper wrapping `poll` for readability and writability checks.
   */
  struct sys_file_poller {
  private:
    short __evts;
    std::chrono::milliseconds __timeout;

  public:
    sys_file_poller(
      short evts = 0, std::chrono::milliseconds timeout = std::chrono::milliseconds{0}
    ) : __evts{evts}, __timeout{timeout} {}
    /**
     * @brief Overrides the poll timeout.
     * @param tout Duration to wait.
     * @return Reference to this poller for chaining.
     */
    sys_file_poller &timeout(std::chrono::milliseconds tout) noexcept {
      __timeout = tout;
      return *this;
    }
    /**
     * @brief Adds a readiness check for readability.
     * @return Reference to this poller for chaining.
     */
    sys_file_poller &poll_in() noexcept {
      __evts = __evts | POLLIN;
      return *this;
    }
    /**
     * @brief Adds a readiness check for writability.
     * @return Reference to this poller for chaining.
     */
    sys_file_poller &poll_out() noexcept {
      __evts = __evts | POLLOUT;
      return *this;
    }
    /**
     * @brief Performs the poll operation.
     * @param fd Descriptor to monitor.
     * @return True when the descriptor is ready, false on timeout, or IO error.
     */
    std::expected<bool, io_error> operator()(int fd) noexcept {
      struct pollfd conf{fd, __evts, 0};
      return sys_call(poll, &conf, 1, __timeout.count()).transform([](int res) {
        return res != 0;
      });
    }

    /**
     * @brief Factory producing a poller configured for writability checks.
     * @param timeout Duration to wait before timing out.
     * @return Poller configured for write readiness.
     */
    static sys_file_poller write_poller(
      std::chrono::milliseconds timeout = std::chrono::milliseconds{0}
    ) noexcept {
      return sys_file_poller{}.timeout(timeout).poll_out();
    }
    /**
     * @brief Factory producing a poller configured for readability checks.
     * @param timeout Duration to wait before timing out.
     * @return Poller configured for read readiness.
     */
    static sys_file_poller read_poller(
      std::chrono::milliseconds timeout = std::chrono::milliseconds{0}
    ) noexcept {
      return sys_file_poller{}.timeout(timeout).poll_in();
    }
  };
  /**
   * @brief Wrapper for invoking `fcntl` with explicit operation and argument.
   */
  struct sys_fcntl {
  private:
    int __op;
    int __args;

  public:
    sys_fcntl(int op = F_GETFD, int args = 0) noexcept : __op{op}, __args{args} {}
    /**
     * @brief Sets the fcntl operation code.
     * @param op Operation passed to `fcntl`.
     * @return Reference to this wrapper for chaining.
     */
    sys_fcntl &op(int op) noexcept {
      __op = op;
      return *this;
    }
    /**
     * @brief Sets the third argument forwarded to `fcntl`.
     * @param args Argument passed to `fcntl`.
     * @return Reference to this wrapper for chaining.
     */
    sys_fcntl &args(int args) noexcept {
      __args = args;
      return *this;
    }
    /**
     * @brief Invokes `fcntl` with the configured parameters.
     * @param fd Descriptor to operate on.
     * @return Result of the `fcntl` call or IO error.
     */
    std::expected<int, io_error> operator()(int fd) const noexcept {
      return sys_call(fcntl, fd, __op, __args);
    }
  };
  /**
   * @brief Helper that ORs file descriptor flags retrieved via `fcntl`.
   */
  struct sys_fcntl_or_flag {
    int __or_flags;

  public:
    sys_fcntl_or_flag(int flags = 0) noexcept : __or_flags{flags} {}
    /**
     * @brief Configures additional flags to be ORed onto the descriptor.
     * @param flags Flags to apply.
     * @return Reference to this helper for chaining.
     */
    sys_fcntl_or_flag &flags_or(int flags) noexcept {
      __or_flags = flags;
      return *this;
    }
    /**
     * @brief Applies the configured flags to the supplied descriptor.
     * @param fd Descriptor to update.
     * @return Success or IO error.
     */
    std::expected<void, io_error> operator()(int fd) const noexcept {
      return sys_fcntl{F_GETFD}(fd).and_then([&](int flags) {
        return sys_fcntl{F_GETFD, flags | __or_flags}(fd).transform([](auto &&) {});
      });
    }
  };
}
