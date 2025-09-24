module;
#include <sys/fcntl.h>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <expected>
#include <string_view>
#include <unistd.h>
#include <utility>
export module jowi.io:pipe;
import jowi.generic;
import :fd_type;
import :error;
import :sys_util;
import :sys_file;

/**
 * @file unix/pipe.cc
 * @brief Pipe abstractions that present reader and writer endpoints with polling helpers.
 */

namespace jowi::io {
  export struct reader_pipe;
  export struct writer_pipe;
  export std::expected<std::pair<reader_pipe, writer_pipe>, io_error> open_pipe(
    bool non_blocking = true
  ) noexcept;

  export struct reader_pipe {
  private:
    file_type __f;
    reader_pipe(file_type f) : __f{std::move(f)} {}
    friend std::expected<std::pair<reader_pipe, writer_pipe>, io_error> open_pipe(bool) noexcept;

  public:
    /**
     * @brief Reads bytes from the pipe into the provided buffer.
     * @param buf Writable buffer receiving bytes.
     * @return Success or IO error.
     */
    std::expected<void, io_error> read(is_writable_buffer auto &buf) noexcept {
      return sys_read(__f.fd(), buf);
    }

    /**
     * @brief Checks if the pipe has data available within the timeout window.
     * @param timeout Duration to wait before timing out.
     * @return True when readable, false on timeout, or IO error.
     */
    std::expected<bool, io_error> is_readable(
      std::chrono::milliseconds timeout = std::chrono::milliseconds{0}
    ) const noexcept {
      return sys_file_poller::read_poller().timeout(timeout)(__f.fd());
    }

    /**
     * @brief Returns a borrowed handle for the underlying descriptor.
     * @return Non-owning file handle.
     */
    file_handle<int> handle() const {
      return __f.borrow();
    }
  };
  export struct writer_pipe {
  private:
    file_type __f;
    writer_pipe(file_type f) : __f{std::move(f)} {}
    friend std::expected<std::pair<reader_pipe, writer_pipe>, io_error> open_pipe(bool) noexcept;

  public:
    /**
     * @brief Writes bytes to the pipe.
     * @param v View describing the bytes to send.
     * @return Number of bytes written or IO error.
     */
    std::expected<size_t, io_error> write(std::string_view v) noexcept {
      return sys_write(__f.fd(), v);
    }
    /**
     * @brief Checks if the pipe can accept data within the timeout window.
     * @param timeout Duration to wait before timing out.
     * @return True when writable, false on timeout, or IO error.
     */
    std::expected<bool, io_error> is_writable(
      std::chrono::milliseconds timeout = std::chrono::milliseconds{0}
    ) const noexcept {
      return sys_file_poller::write_poller().timeout(timeout)(__f.fd());
    }

    /**
     * @brief Returns a borrowed handle for the underlying descriptor.
     * @return Non-owning file handle.
     */
    file_handle<int> handle() const {
      return __f.borrow();
    }
  };

  /**
   * @brief Creates a reader and writer pipe pair.
   * @param non_blocking When true, applies non-blocking and close-on-exec flags.
   * @return Pair of pipe endpoints or IO error.
   */
  export std::expected<std::pair<reader_pipe, writer_pipe>, io_error> open_pipe(
    bool non_blocking
  ) noexcept {
    std::array<int, 2> pipe_fd;
    return sys_call(pipe, pipe_fd.data()).and_then([&](auto &&) {
      file_type r_fd{pipe_fd[0]};
      file_type w_fd{pipe_fd[1]};
      return sys_fcntl_or_flag{}.flags_or(O_NONBLOCK | O_CLOEXEC)(r_fd.fd()).and_then([&]() {
        return sys_fcntl_or_flag{}.flags_or(O_NONBLOCK | O_CLOEXEC)(w_fd.fd()).transform([&]() {
          return std::pair{reader_pipe{std::move(r_fd)}, writer_pipe{std::move(w_fd)}};
        });
      });
    });
  }
}
