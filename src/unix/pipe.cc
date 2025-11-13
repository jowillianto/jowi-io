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
import jowi.asio;
import :fd_type;
import :error;
import :sys_call;

/**
 * @file unix/pipe.cc
 * @brief Pipe abstractions that present reader and writer endpoints with polling helpers.
 */

namespace jowi::io {
  export struct ReaderPipe;
  export struct WriterPipe;
  export std::expected<std::pair<ReaderPipe, WriterPipe>, IoError> open_pipe(
    bool non_blocking = true
  ) noexcept;

  export struct ReaderPipe {
  private:
    FileDescriptor __f;
    ReaderPipe(FileDescriptor f) : __f{std::move(f)} {}
    friend std::expected<std::pair<ReaderPipe, WriterPipe>, IoError> open_pipe(bool) noexcept;

  public:
    /**
     * @brief Reads bytes from the pipe into the provided buffer.
     * @param buf Writable buffer receiving bytes.
     * @return Success or IO error.
     */
    std::expected<void, IoError> read(WritableBuffer auto &buf) noexcept {
      return sys_read(__f, buf);
    }
    template <WritableBuffer buf_type>
    asio::InfiniteAwaiter<SysReadPoller<buf_type>> aread(buf_type &buf) noexcept {
      return {__f, buf};
    }

    template <WritableBuffer buf_type>
    asio::TimedAwaiter<SysReadPoller<buf_type>> aread(
      buf_type &buf, std::chrono::milliseconds dur
    ) noexcept {
      return {dur, __f, buf};
    }
    /**
     * @brief Checks if the pipe has data available within the timeout window.
     * @param timeout Duration to wait before timing out.
     * @return True when readable, false on timeout, or IO error.
     */
    std::expected<bool, IoError> is_readable() const noexcept {
      return sys_poll_in(__f);
    }
    auto native_handle() const noexcept {
      return __f.get_or(-1);
    }
  };
  export struct WriterPipe {
  private:
    FileDescriptor __f;
    WriterPipe(FileDescriptor f) : __f{std::move(f)} {}
    friend std::expected<std::pair<ReaderPipe, WriterPipe>, IoError> open_pipe(bool) noexcept;

  public:
    /**
     * @brief Writes bytes to the pipe.
     * @param v View describing the bytes to send.
     * @return Number of bytes written or IO error.
     */
    std::expected<size_t, IoError> write(std::string_view v) noexcept {
      return sys_write(__f, v);
    }
    asio::InfiniteAwaiter<SysWritePoller> awrite(std::string_view v) noexcept {
      return {__f, v};
    }
    asio::TimedAwaiter<SysWritePoller> awrite(
      std::string_view v, std::chrono::milliseconds dur
    ) noexcept {
      return {dur, __f, v};
    }
    /**
     * @brief Checks if the pipe can accept data within the timeout window.
     * @param timeout Duration to wait before timing out.
     * @return True when writable, false on timeout, or IO error.
     */
    std::expected<bool, IoError> is_writable() const noexcept {
      return sys_poll_out(__f);
    }

    auto native_handle() const noexcept {
      return __f.get_or(-1);
    }
  };

  /**
   * @brief Creates a reader and writer pipe pair.
   * @param non_blocking When true, applies non-blocking and close-on-exec flags.
   * @return Pair of pipe endpoints or IO error.
   */
  export std::expected<std::pair<ReaderPipe, WriterPipe>, IoError> open_pipe(
    bool non_blocking
  ) noexcept {
    return sys_pipe(non_blocking).transform([](auto p) {
      return std::pair{ReaderPipe{std::move(p.first)}, WriterPipe{std::move(p.second)}};
    });
  }
}
