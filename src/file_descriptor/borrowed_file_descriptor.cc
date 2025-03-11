module;
#include <concepts>
export module moderna.io:borrowed_file_descriptor;
import :is_file_descriptor;

namespace moderna::io {
  export template <class fd_t> struct borrowed_file_descriptor {
    using native_handle_t = fd_t;
    borrowed_file_descriptor(fd_t fd) noexcept : __fd{fd} {}
    borrowed_file_descriptor(const is_file_descriptor auto &fd) noexcept : __fd{fd.fd()} {}
    borrowed_file_descriptor(const borrowed_file_descriptor &) noexcept = default;
    borrowed_file_descriptor(borrowed_file_descriptor &&) noexcept = default;
    borrowed_file_descriptor &operator=(const borrowed_file_descriptor &) noexcept = default;
    borrowed_file_descriptor &operator=(borrowed_file_descriptor &&) noexcept = default;
    borrowed_file_descriptor &operator=(is_file_descriptor auto &fd) noexcept {
      __fd = fd.fd();
      return *this;
    }
    fd_t fd() const noexcept {
      return __fd;
    }
    borrowed_file_descriptor<fd_t> borrow() const noexcept {
      return borrowed_file_descriptor{__fd};
    }

  private:
    fd_t __fd;
  };
}