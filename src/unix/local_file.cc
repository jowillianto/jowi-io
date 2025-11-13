module;
#include <sys/fcntl.h>
#include <bitset>
#include <chrono>
#include <expected>
#include <filesystem>
#include <string_view>
#include <unistd.h>
export module jowi.io:local_file;
import jowi.asio;
import :fd_type;
import :error;
import :file;
import :buffer;
import :sys_call;

/**
 * @file unix/LocalFile.cc
 * @brief Thin wrapper around POSIX file descriptors providing higher-level file utilities.
 */

namespace jowi::io {
  namespace fs = std::filesystem;

  /**
   * @brief RAII-style local file abstraction built on top of `FileType`.
   */
  export struct LocalFile {
  private:
    FileDescriptor __f;
    bool __eof;
    LocalFile(FileDescriptor f) noexcept : __f{std::move(f)}, __eof{false} {}
    friend struct OpenOptions;
    static LocalFile from(FileDescriptor f) noexcept {
      return LocalFile{std::move(f)};
    }

  public:
    /**
     * @brief Writes bytes to the file.
     * @param v Bytes to append or overwrite depending on open mode.
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
     * @brief Reads bytes into the provided buffer.
     * @param buf Writable buffer populated with file contents.
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
     * @brief Indicates whether the end-of-file condition has been reached.
     * @return True when EOF has been observed.
     */
    bool eof() const noexcept {
      return __eof;
    }
    /**
     * @brief Performs a seek relative to the chosen origin.
     * @param s Seek origin.
     * @param offset Offset from the origin in bytes.
     * @return New file position or IO error.
     */
    std::expected<off_t, IoError> seek(SeekMode s, off_t offset) noexcept {
      return sys_seek(__f, s, offset);
    }
    /**
     * @brief Seeks relative to the file beginning.
     * @param offset Offset from the beginning.
     * @return New file position or IO error.
     */
    std::expected<off_t, IoError> seek_beg(off_t offset) noexcept {
      return seek(SeekMode::START, offset);
    }
    /**
     * @brief Seeks relative to the current file position.
     * @param offset Offset from the current position.
     * @return New file position or IO error.
     */
    std::expected<off_t, IoError> seek_cur(off_t offset) noexcept {
      return seek(SeekMode::CURRENT, offset);
    }
    /**
     * @brief Seeks relative to the file end.
     * @param offset Negative or zero offset from the end.
     * @return New file position or IO error.
     */
    std::expected<off_t, IoError> seek_end(off_t offset) noexcept {
      return seek(SeekMode::END, offset);
    }
    /**
     * @brief Computes the current file size while restoring the original cursor position.
     * @return File size in bytes or IO error.
     */
    std::expected<size_t, IoError> size() noexcept {
      return seek(SeekMode::CURRENT, 0).and_then([&](off_t cur_pos) {
        return seek(SeekMode::END, 0).and_then([&](size_t fsize) {
          return seek(SeekMode::START, cur_pos).transform([&](auto &&) { return fsize; });
        });
      });
    }
    /**
     * @brief Truncates the file to the supplied length.
     * @param size Desired file size.
     * @return New file position or IO error.
     */
    std::expected<off_t, IoError> truncate(off_t size) noexcept {
      return sys_truncate(__f, size);
    }
    /**
     * @brief Flushes in-memory changes to disk.
     * @return Success or IO error.
     */
    std::expected<void, IoError> sync() noexcept {
      return sys_sync(__f);
    }
    /**
     * @brief Returns a borrowed file handle.
     * @return Non-owning file handle for the underlying descriptor.
     */
    auto native_handle() const noexcept {
      return __f.get_or(-1);
    }
  };

  /**
   * @brief Fluent interface for configuring file open flags.
   */
  export struct OpenOptions {
  private:
    std::bitset<32> __opt;

  public:
    /**
     * @brief Initializes with no flags set.
     */
    OpenOptions() noexcept : __opt{0} {}
    /**
     * @brief Requests read-only access.
     * @return Reference to these options for chaining.
     */
    OpenOptions &read() noexcept {
      __opt.set(0, false);
      __opt.set(1, false);
      __opt |= O_RDONLY;
      return *this;
    }
    /**
     * @brief Requests write-only access.
     * @return Reference to these options for chaining.
     */
    OpenOptions &write() noexcept {
      __opt.set(0, false);
      __opt.set(1, false);
      __opt |= O_WRONLY;
      return *this;
    }
    /**
     * @brief Requests read-write access.
     * @return Reference to these options for chaining.
     */
    OpenOptions &read_write() noexcept {
      __opt.set(0, false);
      __opt.set(1, false);
      __opt |= O_RDWR;
      return *this;
    }
    /**
     * @brief Applies the truncate flag.
     * @return Reference to these options for chaining.
     */
    OpenOptions &truncate() noexcept {
      __opt |= O_TRUNC;
      return *this;
    }
    /**
     * @brief Applies append semantics.
     * @return Reference to these options for chaining.
     */
    OpenOptions &append() noexcept {
      __opt |= O_APPEND;
      return *this;
    }
    /**
     * @brief Requests file creation when it does not exist.
     * @return Reference to these options for chaining.
     */
    OpenOptions &create() noexcept {
      __opt |= O_CREAT;
      return *this;
    }
    /**
     * @brief Opens the file described by the configuration.
     * @param p Filesystem path to open.
     * @return Local file object or IO error.
     */
    std::expected<LocalFile, IoError> open(const fs::path &p) const noexcept {
      return sys_call(::open, p.c_str(), static_cast<int>(__opt.to_ulong()))
        .transform(FileDescriptor::manage_default)
        .transform(LocalFile::from);
    }
  };
}
