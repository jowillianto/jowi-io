module;
#include <sys/file.h>
#include <array>
#include <concepts>
#include <expected>
#include <filesystem>
#include <poll.h>
#include <ranges>
#include <unistd.h>
export module moderna.io:local_file;
import :file;
import :file_descriptor;
import :open_mode;
import :seek_mode;
import :error;

namespace moderna::io {
  export template <is_file_descriptor desc_t> struct file_reader {
    file_reader(desc_t fd) noexcept : __fd{std::move(fd)} {}
    file_reader(file_reader &&) noexcept = default;
    file_reader(const file_reader &) = default;
    file_reader &operator=(file_reader &&) noexcept = default;
    file_reader &operator=(const file_reader &) = default;

    template <size_t buffer_size = 4096> std::expected<std::string, fs_error> read() const {
      std::array<char, buffer_size> buffer;
      std::string data;
      while (true) {
        int read_bytes = ::read(__fd.fd(), static_cast<void *>(buffer.data()), buffer_size);
        int err_no = errno;
        if (read_bytes == -1) {
          if (err_no != EAGAIN || err_no == EWOULDBLOCK) {
            return data;
          }
          return std::unexpected{fs_error{err_no, strerror(err_no)}};
        } else if (read_bytes == 0) {
          break;
        }
        data.append_range(std::ranges::subrange{buffer.begin(), buffer.begin() + read_bytes});
      }
      return data;
    }
    template <size_t buffer_size = 4096>
    std::expected<std::string, fs_error> read(size_t bytes_to_read) const {
      std::array<char, buffer_size> buffer;
      std::string data;
      size_t unread_bytes = bytes_to_read;
      while (unread_bytes != 0) {
        int bytes_read = ::read(__fd.fd(), static_cast<void *>(buffer.data()), unread_bytes);
        int err_no = errno;
        if (bytes_read == -1) {
          if (err_no != EAGAIN || err_no == EWOULDBLOCK) {
            return data;
          }
          return std::unexpected{fs_error{err_no, strerror(err_no)}};
        } else if (bytes_read == 0) {
          break;
        }
        data.append_range(std::ranges::subrange{buffer.begin(), buffer.begin() + bytes_read});
        unread_bytes -= bytes_read;
      }
      return data;
    }
    template <size_t read_size>
    std::expected<std::array<char, read_size + 1>, fs_error> read() const {
      std::array<char, read_size> buffer;
      size_t read_bytes = 0;
      while (read_bytes != read_size) {
        int bytes_read = ::read(
          __fd.fd(), static_cast<void *>(buffer.begin() + read_bytes), read_size - read_bytes
        );
        int err_no = errno;
        if (bytes_read == -1) {
          if (err_no != EAGAIN || err_no == EWOULDBLOCK) {
            buffer[read_bytes] = '\0';
          }
          return std::unexpected{fs_error{err_no, strerror(err_no)}};
        } else if (bytes_read == 0) {
          break;
        }
        read_bytes += bytes_read;
      }
      buffer[read_bytes] = '\0';
      return buffer;
    }
    std::expected<bool, fs_error> is_readable() const noexcept {
      struct pollfd poll_fd = {.fd = __fd.fd(), .events = POLLIN};
      int res = ::poll(&poll_fd, 1, 0);
      int err_no = errno;
      if (res == -1) {
        return std::unexpected{fs_error{err_no, strerror(err_no)}};
      }
      return res != 0;
    }
    const desc_t &fd() const & noexcept {
      return __fd;
    }
    desc_t &fd() & noexcept {
      return __fd;
    }
    file_reader<borrowed_file_descriptor<typename desc_t::native_handle_t>> borrow(
    ) const noexcept {
      return file_reader{__fd.borrow()};
    }
    void close() noexcept {
      desc_t fd = std::move(__fd);
    }

  private:
    desc_t __fd;
  };
  export template <is_file_descriptor desc_t> struct file_writer {
    file_writer(desc_t fd) noexcept : __fd{std::move(fd)} {}
    file_writer(file_writer &&) noexcept = default;
    file_writer(const file_writer &) = default;
    file_writer &operator=(file_writer &&) noexcept = default;
    file_writer &operator=(const file_writer &) = default;
    std::expected<void, fs_error> write(std::string_view data) const noexcept {
      size_t total_written_bytes = 0;
      while (total_written_bytes < data.length()) {
        int written_bytes = ::write(
          __fd.fd(),
          static_cast<const void *>(data.begin() + total_written_bytes),
          data.length() - total_written_bytes
        );
        int err_no = errno;
        if (written_bytes == -1) {
          return std::unexpected{fs_error{err_no, strerror(err_no)}};
        }
        total_written_bytes += written_bytes;
      }
      return {};
    }
    std::expected<bool, fs_error> is_writable() const noexcept {
      struct pollfd poll_fd = {.fd = __fd.fd(), .events = POLLOUT};
      int res = ::poll(&poll_fd, 1, 0);
      int err_no = errno;
      if (res == -1) {
        return std::unexpected{fs_error{err_no, strerror(err_no)}};
      }
      return res != 0;
    }
    const desc_t &fd() const & {
      return __fd;
    }
    desc_t &fd() & {
      return __fd;
    }
    file_writer<borrowed_file_descriptor<typename desc_t::native_handle_t>> borrow() const {
      return file_writer{__fd.borrow()};
    }
    void close() noexcept {
      desc_t fd = std::move(__fd);
    }

  private:
    desc_t __fd;
  };
  export template <is_file_descriptor desc_t> struct file_seeker {
    file_seeker(desc_t fd) noexcept : __fd{std::move(fd)} {}
    file_seeker(file_seeker &&) noexcept = default;
    file_seeker(const file_seeker &) = default;
    file_seeker &operator=(file_seeker &&) noexcept = default;
    file_seeker &operator=(const file_seeker &) = default;
    std::expected<off_t, fs_error> seek(size_t offset, seek_mode mode = seek_mode::start)
      const noexcept {
      off_t cur_off = lseek(__fd.fd(), offset, __get_seek_flags(mode));
      int err_no = errno;
      if (cur_off == -1) {
        return std::unexpected{fs_error{err_no, strerror(err_no)}};
      }
      return cur_off;
    }
    std::expected<off_t, fs_error> tell() const noexcept {
      return seek(0, seek_mode::current);
    }
    std::expected<off_t, fs_error> seek_begin() const noexcept {
      return seek(0, seek_mode::start);
    }
    std::expected<off_t, fs_error> seek_end() const noexcept {
      return seek(0, seek_mode::end);
    }

    const desc_t &fd() const & noexcept {
      return __fd;
    }
    desc_t &fd() & noexcept {
      return __fd;
    }
    file_seeker<borrowed_file_descriptor<typename desc_t::native_handle_t>> borrow(
    ) const noexcept {
      return file_seeker{__fd.borrow()};
    }
    void close() noexcept {
      desc_t fd = std::move(__fd);
    }

  private:
    desc_t __fd;
    int __get_seek_flags(seek_mode mode) const {
      if (mode == seek_mode::start) {
        return SEEK_SET;
      } else if (mode == seek_mode::current) {
        return SEEK_CUR;
      } else {
        return SEEK_END;
      }
    }
  };
  export template <is_file_descriptor desc_t> struct file_syncer {
    file_syncer(desc_t fd) : __fd{std::move(fd)} {}
    file_syncer(file_syncer &&) noexcept = default;
    file_syncer(const file_syncer &) = default;
    file_syncer &operator=(file_syncer &&) noexcept = default;
    file_syncer &operator=(const file_syncer &) = default;
    std::expected<void, fs_error> sync(bool flush_metadata = false) const noexcept {
      if (flush_metadata) {
        int res = fdatasync(__fd.fd());
        int err_no = errno;
        if (res == -1) {
          return std::unexpected{fs_error{err_no, strerror(err_no)}};
        }
      } else {
        int res = fsync(__fd.fd());
        int err_no = errno;
        if (res == -1) {
          return std::unexpected{fs_error{err_no, strerror(err_no)}};
        }
      }
    }
    const desc_t &fd() const & noexcept {
      return __fd;
    }
    desc_t &fd() & noexcept {
      return __fd;
    }
    file_syncer<borrowed_file_descriptor<typename desc_t::native_handle_t>> borrow(
    ) const noexcept {
      return file_syncer{__fd.borrow()};
    }
    void close() noexcept {
      desc_t fd = std::move(__fd);
    }

  private:
    desc_t __fd;
  };
  export template <is_file_descriptor desc_t, open_mode mode> struct rw_file {
    rw_file(desc_t fd) : __fd{std::move(fd)} {}
    rw_file(rw_file &&) noexcept = default;
    rw_file(const rw_file &) = default;
    rw_file &operator=(rw_file &&) noexcept = default;
    rw_file &operator=(const rw_file &) = default;

    is_readable<std::string> auto get_reader() const & noexcept
      requires(mode != open_mode::write_append && mode != open_mode::write_truncate)
    {
      return file_reader{__fd.borrow()};
    }
    is_writable<std::string_view> auto get_writer() const & noexcept
      requires(mode != open_mode::read)
    {
      return file_writer{__fd.borrow()};
    }
    is_seekable auto get_seeker() const & noexcept {
      return file_seeker{__fd.borrow()};
    }
    is_syncable auto get_syncer() const & noexcept {
      return file_syncer{__fd.borrow()};
    }
    is_readable<std::string> auto get_reader() && noexcept
      requires(mode != open_mode::write_append && mode != open_mode::write_truncate)
    {
      return file_reader{std::move(__fd)};
    }
    is_writable<std::string_view> auto get_writer() && noexcept
      requires(mode != open_mode::read)
    {
      return file_writer{std::move(__fd)};
    }
    is_seekable auto get_seeker() && noexcept {
      return file_seeker{std::move(__fd)};
    }
    is_syncable auto get_syncer() && noexcept {
      return file_syncer{std::move(__fd)};
    }

    desc_t &fd() & noexcept {
      return __fd;
    }
    const desc_t &fd() const & noexcept {
      return __fd;
    }
    void close() noexcept {
      desc_t fd = std::move(__fd);
    }

  private:
    desc_t __fd;
  };

  struct file_closer {
    void operator()(int fd) const noexcept {
      ::close(fd);
    }
  };

  /*
    This is the type of the file descriptor used in unix based system.
  */
  using unix_fd = file_descriptor<int, file_closer>;

  /*
    Generic file opener, this will open  a file in the given path in a specific mode with no
    compile time restrictions on its usage.
  */

  export template <open_mode mode = open_mode::read> struct file_opener {
    file_opener(std::filesystem::path path)
      requires(mode == open_mode::read)
      : __path{std::move(path)} {}
    file_opener(std::filesystem::path path, bool auto_create = true, int permission_bit = 0666)
      requires(mode == open_mode::write_truncate || mode == open_mode::write_append)
      : __path{std::move(path)}, __auto_create{auto_create} {}
    const std::filesystem::path &path() const noexcept {
      return __path;
    }
    std::expected<rw_file<unix_fd, mode>, fs_error> open() const noexcept {
      int fd = ::open(__path.c_str(), __get_flags(), __permission);
      int err_no = errno;
      if (fd == -1) {
        return std::unexpected{fs_error{err_no, strerror(err_no)}};
      }
      return rw_file<unix_fd, mode>{unix_fd{fd}};
    }

  private:
    std::filesystem::path __path;
    bool __auto_create;
    int __permission;

    constexpr int __get_flags() const noexcept {
      if constexpr (mode == open_mode::read) {
        return O_RDONLY;
      } else if constexpr (mode == open_mode::write_truncate) {
        if (__auto_create) {
          return O_WRONLY | O_CREAT | O_TRUNC;
        } else {
          return O_WRONLY | O_TRUNC;
        }
      } else if constexpr (mode == open_mode::write_append) {
        if (__auto_create) {
          return O_WRONLY | O_CREAT | O_APPEND;
        } else {
          return O_WRONLY | O_APPEND;
        }
      } else {
        return O_RDWR;
      }
    }
  };

}