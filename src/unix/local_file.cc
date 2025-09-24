module;
#include <sys/fcntl.h>
#include <bitset>
#include <expected>
#include <filesystem>
#include <unistd.h>
export module jowi.io:local_file;
import jowi.generic;
import :fd_type;
import :error;
import :sys_file;
import :is_file;
import :buffer;

/**
 * @file unix/local_file.cc
 * @brief Thin wrapper around POSIX file descriptors providing higher-level file utilities.
 */

namespace jowi::io {
  namespace fs = std::filesystem;

  /**
   * @brief RAII-style local file abstraction built on top of `file_type`.
   */
  export struct local_file {
  private:
    file_type __f;
    bool __eof;
    local_file(file_type f) noexcept : __f{std::move(f)}, __eof{false} {}
    friend struct open_options;

  public:
    /**
     * @brief Writes bytes to the file.
     * @param v Bytes to append or overwrite depending on open mode.
     * @return Number of bytes written or IO error.
     */
    std::expected<size_t, io_error> write(std::string_view v) noexcept {
      return sys_write(__f.fd(), v);
    }
    /**
     * @brief Reads bytes into the provided buffer.
     * @param buf Writable buffer populated with file contents.
     * @return Success or IO error.
     */
    std::expected<void, io_error> read(is_writable_buffer auto &buf) noexcept {
      return sys_read(__f.fd(), buf);
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
    std::expected<off_t, io_error> seek(seek_mode s, off_t offset) noexcept {
      return sys_seek(__f.fd(), s, offset);
    }
    /**
     * @brief Seeks relative to the file beginning.
     * @param offset Offset from the beginning.
     * @return New file position or IO error.
     */
    std::expected<off_t, io_error> seek_beg(off_t offset) noexcept {
      return seek(seek_mode::START, offset);
    }
    /**
     * @brief Seeks relative to the current file position.
     * @param offset Offset from the current position.
     * @return New file position or IO error.
     */
    std::expected<off_t, io_error> seek_cur(off_t offset) noexcept {
      return seek(seek_mode::CURRENT, offset);
    }
    /**
     * @brief Seeks relative to the file end.
     * @param offset Negative or zero offset from the end.
     * @return New file position or IO error.
     */
    std::expected<off_t, io_error> seek_end(off_t offset) noexcept {
      return seek(seek_mode::END, offset);
    }
    /**
     * @brief Computes the current file size while restoring the original cursor position.
     * @return File size in bytes or IO error.
     */
    std::expected<size_t, io_error> size() noexcept {
      return seek(seek_mode::CURRENT, 0).and_then([&](off_t cur_pos) {
        return seek(seek_mode::END, 0).and_then([&](size_t fsize) {
          return seek(seek_mode::START, cur_pos).transform([&](auto &&) { return fsize; });
        });
      });
    }
    /**
     * @brief Truncates the file to the supplied length.
     * @param size Desired file size.
     * @return New file position or IO error.
     */
    std::expected<off_t, io_error> truncate(off_t size) noexcept {
      return sys_truncate(__f.fd(), size);
    }
    /**
     * @brief Flushes in-memory changes to disk.
     * @return Success or IO error.
     */
    std::expected<void, io_error> sync() noexcept {
      return sys_sync(__f.fd());
    }
    /**
     * @brief Returns a borrowed file handle.
     * @return Non-owning file handle for the underlying descriptor.
     */
    file_handle<int> handle() const {
      return __f.borrow();
    }
  };

  /**
   * @brief Fluent interface for configuring file open flags.
   */
  export struct open_options {
  private:
    std::bitset<32> __opt;

  public:
    /**
     * @brief Initializes with no flags set.
     */
    open_options() noexcept : __opt{0} {}
    /**
     * @brief Requests read-only access.
     * @return Reference to these options for chaining.
     */
    open_options &read() noexcept {
      __opt.set(0, false);
      __opt.set(1, false);
      __opt |= O_RDONLY;
      return *this;
    }
    /**
     * @brief Requests write-only access.
     * @return Reference to these options for chaining.
     */
    open_options &write() noexcept {
      __opt.set(0, false);
      __opt.set(1, false);
      __opt |= O_WRONLY;
      return *this;
    }
    /**
     * @brief Requests read-write access.
     * @return Reference to these options for chaining.
     */
    open_options &read_write() noexcept {
      __opt.set(0, false);
      __opt.set(1, false);
      __opt |= O_RDWR;
      return *this;
    }
    /**
     * @brief Applies the truncate flag.
     * @return Reference to these options for chaining.
     */
    open_options &truncate() noexcept {
      __opt |= O_TRUNC;
      return *this;
    }
    /**
     * @brief Applies append semantics.
     * @return Reference to these options for chaining.
     */
    open_options &append() noexcept {
      __opt |= O_APPEND;
      return *this;
    }
    /**
     * @brief Requests file creation when it does not exist.
     * @return Reference to these options for chaining.
     */
    open_options &create() noexcept {
      __opt |= O_CREAT;
      return *this;
    }
    /**
     * @brief Opens the file described by the configuration.
     * @param p Filesystem path to open.
     * @return Local file object or IO error.
     */
    std::expected<local_file, io_error> open(const fs::path &p) const noexcept {
      return sys_call(::open, p.c_str(), static_cast<int>(__opt.to_ulong()))
        .transform([](auto &&fd) { return local_file{file_type{fd}}; });
    }
  };
}
