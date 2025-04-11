module;
#include <expected>
#include <string>
#include <string_view>
#include <unistd.h>
#include <vector>
export module moderna.io:file_writer;
import :is_file_descriptor;
import :borrowed_file_descriptor;
import :is_borroweable_file_descriptor;
import :adapter;
import :error;
import :file;
import :file_poller;

namespace moderna::io {
  export struct char_writer {
    std::expected<void, fs_error> write(is_file_descriptor auto fd, char x) const noexcept {
      return cp_adapter::write(fd.fd(), &x, 1)
        .transform([](auto x) -> void {})
        .transform_error(cp_adapter::make_error);
    }
  };

  export struct str_writer {
    std::expected<void, fs_error> write(is_file_descriptor auto fd, std::string_view d)
      const noexcept {
      return cp_adapter::write(fd.fd(), static_cast<const void *>(d.data()), d.length())
        .transform([](auto x) -> void {})
        .transform_error(cp_adapter::make_error);
    }
  };

  export class line_writer {
  private:
    char __delim;

  public:
    line_writer(char delim = '\n') : __delim{delim} {}
    std::expected<void, fs_error> write(is_borroweable_file_descriptor auto fd, std::string_view v)
      const {
      return str_writer{}.write(fd.borrow(), v).and_then([&]() {
        return char_writer{}.write(fd.borrow(), '\n');
      });
    }
  };

  export struct csv_writer {
    csv_writer(char delim = ',') : __delim{delim} {}
    std::expected<void, fs_error> write(
      is_borroweable_file_descriptor auto fd, const std::vector<std::vector<std::string>> &v
    ) const {
      for (size_t i = 0; i < v.size(); i += 1) {
        for (size_t j = 0; j < v[i].size(); j += 1) {
          auto res = str_writer{}.write(fd.borrow(), v[i][j]);
          if (j != v[i].size() - 1) {
            res =
              std::move(res).and_then([&]() { return char_writer{}.write(fd.borrow(), __delim); });
          }
          if (!res.has_value()) {
            return std::unexpected{std::move(res.error())};
          }
        }
        if (i != v.size() - 1) {
          auto res = char_writer{}.write(fd.borrow(), '\n');
          if (!res.has_value()) {
            return std::unexpected{std::move(res.error())};
          }
        }
      }
      return {};
    }

  private:
    char __delim;
  };

  export template <is_file_descriptor fd_t> struct writable_file {
    using borrowed_fd = borrowed_file_descriptor<native_handle_type<fd_t>>;
    writable_file(fd_t fd) : __fd{std::move(fd)} {}

    template <class write_t, is_writable<borrowed_fd, write_t> writer_t>
    auto write(write_t &&w, const writer_t &writer) const {
      return writer.write(fd(), std::forward<write_t>(w));
    }

    std::expected<void, fs_error> write(std::string_view d) const {
      return write(d, str_writer{});
    }
    std::expected<void, fs_error> write_one(char x) const {
      return write(x, char_writer{});
    }
    std::expected<bool, fs_error> is_writable(int timeout = 0) const {
      return file_poller::make_write_poller(timeout).poll_binary(fd());
    }

    writable_file<borrowed_fd> borrow() const noexcept {
      return writable_file{fd()};
    }
    borrowed_fd fd() const noexcept {
      return __fd.borrow();
    }

  private:
    fd_t __fd;
  };
}