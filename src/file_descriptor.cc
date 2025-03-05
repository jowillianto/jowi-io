module;
#include <concepts>
#include <functional>
export module moderna.io:file_descriptor;

namespace moderna::io {
  export template <std::integral fd_t> struct borrowed_file_descriptor;

  export template <class T>
  concept is_file_descriptor = requires(T fd) {
    std::integral<typename T::native_handle_t>;
    { fd.fd() } -> std::same_as<typename T::native_handle_t>;
    { fd.borrow() } -> std::same_as<borrowed_file_descriptor<typename T::native_handle_t>>;
  };

  /*
    This borrows a file descriptor and does not really own the file descriptor
  */
  template <std::integral fd_t> struct borrowed_file_descriptor {
    using native_handle_t = fd_t;
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
      return *this;
    }

  private:
    fd_t __fd;
  };
  /*
    This controls the lifetime of a file descriptor. The file descriptor class automatically
    closes a file descriptor when it goes out of scope.
 */
  export template <std::integral fd_t, std::invocable<fd_t> closer_t, fd_t invalid_value = -1>
  struct file_descriptor {
    using native_handle_t = fd_t;
    file_descriptor(native_handle_t fd, closer_t closer) noexcept :
      __fd{fd}, __closer{std::move(closer)} {}
    file_descriptor(native_handle_t fd) noexcept
      requires(std::is_default_constructible_v<closer_t>)
      : __fd{fd}, __closer{} {}
    file_descriptor(file_descriptor &&o) noexcept : __fd{o.__fd}, __closer{o.__closer} {
      o.__fd = invalid_value;
    }
    file_descriptor(const file_descriptor &) = delete;
    file_descriptor &operator=(file_descriptor &&o) noexcept {
      __fd = o.__fd;
      __closer = o.__closer;
      o.__fd = invalid_value;
      return *this;
    }
    file_descriptor &operator=(const file_descriptor &) = delete;
    native_handle_t fd() const noexcept {
      return __fd;
    }
    borrowed_file_descriptor<native_handle_t> borrow() const noexcept {
      return borrowed_file_descriptor<native_handle_t>{*this};
    }
    ~file_descriptor() noexcept {
      if (__fd != invalid_value) {
        __closer(__fd);
      }
    }

  private:
    closer_t __closer;
    native_handle_t __fd;
  };
}