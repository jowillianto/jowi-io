module;
#include <concepts>
export module jowi.io:file_descriptor;

namespace jowi::io {
  /*
    file_handle
    is a non owning descriptor produced by calling borrow on a file_descriptor.
  */
  export template <class fd_type> class file_handle {
    fd_type __fd;

  public:
    using native_handle_type = fd_type;
    file_handle(fd_type fd) noexcept : __fd{fd} {}
    native_handle_type fd() const noexcept {
      return __fd;
    }
    file_handle<fd_type> borrow() const noexcept {
      return *this;
    }
  };
  /*
    This controls the lifetime of a file descriptor. The file descriptor class automatically
    closes a file descriptor when it goes out of scope.
 */
  export template <class fd_type, std::invocable<fd_type> closer_t, fd_type invalid_value = -1>
  struct file_descriptor {
    using native_handle_type = fd_type;
    constexpr file_descriptor(native_handle_type fd, closer_t closer) noexcept :
      __fd{fd}, __closer{std::move(closer)} {}
    constexpr file_descriptor(native_handle_type fd) noexcept
      requires(std::is_default_constructible_v<closer_t>)
      : __fd{fd}, __closer{} {}
    constexpr file_descriptor(file_descriptor &&o) noexcept : __fd{o.__fd}, __closer{o.__closer} {
      o.__fd = invalid_value;
    }
    constexpr file_descriptor(const file_descriptor &) = delete;
    constexpr file_descriptor &operator=(file_descriptor &&o) noexcept {
      __fd = o.__fd;
      __closer = o.__closer;
      o.__fd = invalid_value;
      return *this;
    }
    constexpr file_descriptor &operator=(const file_descriptor &) = delete;
    constexpr native_handle_type fd() const noexcept {
      return __fd;
    }
    constexpr file_handle<native_handle_type> borrow() const noexcept {
      return file_handle{__fd};
    }
    ~file_descriptor() noexcept {
      if (__fd != invalid_value) {
        __closer(__fd);
      }
    }

  private:
    closer_t __closer;
    native_handle_type __fd;
  };

  export template <class fd_type>
  concept is_file_descriptor = requires(const fd_type &fd) {
    { fd.fd() } -> std::same_as<typename fd_type::native_handle_type>;
    { fd.borrow() } -> std::same_as<file_handle<typename fd_type::native_handle_type>>;
  };
}