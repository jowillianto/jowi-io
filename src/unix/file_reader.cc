module;
#include <sys/file.h>
#include <sys/poll.h>
#include <array>
#include <cerrno>
#include <expected>
#include <optional>
#include <string>
#include <unistd.h>
#include <utility>
export module moderna.io:file_reader;
import :is_file_descriptor;
import :is_borroweable_file_descriptor;
import :borrowed_file_descriptor;
import :error;

namespace moderna::io {
  export template <is_borroweable_file_descriptor fd_t> struct file_reader {
    file_reader(fd_t fd) noexcept : __fd{std::move(fd)} {}
    file_reader(file_reader &&) noexcept = default;
    file_reader(const file_reader &) = default;
    file_reader &operator=(file_reader &&) noexcept = default;
    file_reader &operator=(const file_reader &) = default;

    /*
      Read Functions
    */

    /*
      Read one character from the file, this can either return a character or nothing if the file
      have reached the end.
    */
    std::expected<std::optional<char>, fs_error> read_one() const noexcept {
      char buf = 0;
      while (true) {
        int read_bytes = ::read(__fd.fd(), static_cast<void *>(&buf), 1);
        int err_no = errno;
        if (read_bytes == -1) {
          if (err_no != EAGAIN || err_no != EWOULDBLOCK) {
            return std::unexpected{fs_error{err_no, strerror(err_no)}};
          }
          // Try again.
        } else if (read_bytes == 0) {
          return std::optional<char>{std::nullopt};
        } else {
          return std::optional<char>{buf};
        }
      }
    }
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

    borrowed_file_descriptor<native_handle_type<fd_t>> fd() const noexcept {
      return __fd.borrow();
    }
    file_reader<borrowed_file_descriptor<native_handle_type<fd_t>>> borrow() const noexcept {
      return file_reader{fd()};
    }

  private:
    fd_t __fd;
  };
}