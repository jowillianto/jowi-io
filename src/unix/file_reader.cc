module;
#include <sys/poll.h>
#include <algorithm>
#include <expected>
#include <optional>
#include <ranges>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>
export module moderna.io:file_reader;
import :file_poller;
import :is_file_descriptor;
import :borrowed_file_descriptor;
import :error;
import :adapter;
import :file;

namespace moderna::io {
  export struct char_reader {
    std::expected<std::optional<char>, fs_error> read(is_file_descriptor auto fd) const noexcept {
      char x;
      return cp_adapter::read(fd.fd(), static_cast<void *>(&x), 1)
        .transform([&](int c) -> std::optional<char> {
          if (c == 0) {
            return std::optional<char>{std::nullopt};
          } else
            return std::optional<char>{x};
        })
        .transform_error(cp_adapter::make_error);
    }
  };
  export struct str_reader {
    str_reader(size_t size) noexcept : __read_size{size} {}
    std::expected<std::string, fs_error> read(is_file_descriptor auto fd) const {
      std::string buf(__read_size, '\0');
      return cp_adapter::read(fd.fd(), static_cast<void *>(buf.data()), __read_size)
        .transform([&](int read_count) -> std::string {
          buf.resize(read_count);
          return std::move(buf);
        })
        .transform_error(cp_adapter::make_error);
    }

  private:
    size_t __read_size;
  };

  export struct str_eof_reader {
    std::expected<std::string, fs_error> read(is_file_descriptor auto fd) const {
      std::array<char, 4096> temp_buf;
      std::string buf;
      using expected_t = std::expected<std::string, fs_error>;
      while (true) {
        auto res = cp_adapter::read(fd.fd(), static_cast<void *>(temp_buf.data()), 4096);
        if (res.has_value() && res.value() == 0) {
          break;
        } else if (res.has_value()) {
          buf.append_range(std::ranges::subrange{temp_buf.begin(), temp_buf.begin() + res.value()});
        } else {
          if (res.error() == EAGAIN) {
            break;
          } else {
            return std::unexpected{cp_adapter::make_error(std::move(res.error()))};
          }
        }
      }
      return std::move(buf);
    }
  };

  export struct block_until_delimiter_reader {
    block_until_delimiter_reader(char delim = '\n') : __delim{delim} {}
    std::expected<std::string, fs_error> read(is_file_descriptor auto fd) const {
      std::string data;
      while (true) {
        char buf;
        auto res = cp_adapter::read(fd.fd(), static_cast<void *>(&buf), 1);
        if (res.has_value() && res.value() == 0) {
          break;
        } else if (res.has_value() && buf == __delim) {
          break;
        } else if (res.has_value()) {
          data.push_back(buf);
          continue;
        } else if (res.error() == EAGAIN) {
          continue;
        } else {
          return std::unexpected{cp_adapter::make_error(std::move(res.error()))};
        }
      }
      return std::move(data);
    }

  private:
    char __delim;
  };

  export struct delimited_reader {
    delimited_reader(char delim = '\n') : __delim{delim} {}
    std::expected<std::vector<std::string>, fs_error> read(is_file_descriptor auto fd) const {
      return str_eof_reader{}.read(std::move(fd)).transform([&](auto &&lines) {
        std::vector<std::string> sep_lines;
        auto sep_lines_it = std::back_inserter(sep_lines);
        sep_lines.reserve(std::ranges::count(lines, __delim));
        auto lines_sep_tf = std::ranges::transform_view{
          std::ranges::split_view{std::move(lines), __delim},
          [](auto &&line) {
            return std::string{line.begin(), line.end()};
          }
        };
        std::ranges::copy(lines_sep_tf, sep_lines_it);
        return std::move(sep_lines);
      });
    }

  private:
    char __delim;
  };

  export struct csv_reader {
    csv_reader(char delim = ',') : __delim{delim} {}
    std::expected<std::vector<std::vector<std::string>>, fs_error> read(is_file_descriptor auto fd
    ) const {
      return delimited_reader{'\n'}.read(std::move(fd)).transform([&](auto &&lines) {
        std::vector<std::vector<std::string>> sep_lines;
        auto sep_lines_it = std::back_inserter(sep_lines);
        sep_lines.reserve(lines.size());
        std::ranges::copy(
          std::ranges::transform_view{
            std::move(lines),
            [&](auto &&line) {
              std::vector<std::string> values;
              auto it = std::back_inserter(values);
              auto sep_values = std::ranges::transform_view{
                std::ranges::split_view{line, __delim},
                [](auto &&sep_value) {
                  return std::string{sep_value.begin(), sep_value.end()};
                }
              };
              std::ranges::copy(sep_values, it);
              return std::move(values);
            }
          },
          sep_lines_it
        );
        return std::move(sep_lines);
      });
    }

  private:
    char __delim;
  };

  export template <is_file_descriptor fd_t> struct readable_file {
    using borrowed_fd = borrowed_file_descriptor<native_handle_type<fd_t>>;
    readable_file(fd_t fd) : __fd{std::move(fd)} {}

    template <is_readable<borrowed_fd> reader_t>
    decltype(std::declval<reader_t>().read(std::declval<borrowed_fd>())) read(const reader_t &reader
    ) const {
      return reader.read(fd());
    }

    std::expected<bool, fs_error> is_readable(int timeout = 0) const {
      return file_poller::make_read_poller(timeout).poll_binary(fd());
    }

    std::expected<std::string, fs_error> read(size_t nbytes) const {
      return read(str_reader{nbytes});
    }
    std::expected<std::string, fs_error> read() const {
      return read(str_eof_reader{});
    }
    std::expected<char, fs_error> read_one() const {
      return read(char_reader{});
    }

    readable_file<borrowed_fd> borrow() const noexcept {
      return readable_file{fd()};
    }
    borrowed_fd fd() const noexcept {
      return __fd.borrow();
    }

  private:
    fd_t __fd;
  };
}