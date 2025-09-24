module;
#include <concepts>
export module jowi.io:file_descriptor;

/**
 * @file file_descriptor.cc
 * @brief RAII helpers for managing native file descriptors.
 */
namespace jowi::io {
  /**
   * @brief Lightweight non-owning wrapper around a native file descriptor.
   * @tparam fd_type Native descriptor type.
   */
  export template <class fd_type> class file_handle {
    fd_type __fd;

  public:
    using native_handle_type = fd_type;
    /**
     * @brief Stores the provided descriptor without taking ownership.
     * @param fd Native descriptor to wrap.
     */
    file_handle(fd_type fd) noexcept : __fd{fd} {}
    /**
     * @brief Retrieves the wrapped descriptor value.
     * @return Native descriptor.
     */
    native_handle_type fd() const noexcept {
      return __fd;
    }
    /**
     * @brief Creates another non-owning view of the descriptor.
     * @return Copy of the current handle.
     */
    file_handle<fd_type> borrow() const noexcept {
      return *this;
    }
  };
  /**
   * @brief Owning RAII wrapper that closes the descriptor via the supplied closer functor.
   * @tparam fd_type Native descriptor type.
   * @tparam closer_t Functor that closes the descriptor.
   * @tparam invalid_value Sentinel representing an invalid descriptor.
   */
  export template <class fd_type, std::invocable<fd_type> closer_t, fd_type invalid_value = -1>
  struct file_descriptor {
    using native_handle_type = fd_type;
    /**
     * @brief Takes ownership of a descriptor and closer functor.
     * @param fd Native descriptor to manage.
     * @param closer Functor invoked to close the descriptor.
     */
    constexpr file_descriptor(native_handle_type fd, closer_t closer) noexcept :
      __fd{fd}, __closer{std::move(closer)} {}
    /**
     * @brief Uses a default-constructed closer when available.
     * @param fd Native descriptor to manage.
     */
    constexpr file_descriptor(native_handle_type fd) noexcept
      requires(std::is_default_constructible_v<closer_t>)
      : __fd{fd}, __closer{} {}
    /**
     * @brief Transfers ownership from another descriptor.
     * @param o Source descriptor whose ownership is transferred.
     */
    constexpr file_descriptor(file_descriptor &&o) noexcept : __fd{o.__fd}, __closer{o.__closer} {
      o.__fd = invalid_value;
    }
    constexpr file_descriptor(const file_descriptor &) = delete;
    /**
     * @brief Move-assigns ownership from another descriptor.
     * @param o Source descriptor whose ownership is transferred.
     * @return Reference to this descriptor.
     */
    constexpr file_descriptor &operator=(file_descriptor &&o) noexcept {
      __fd = o.__fd;
      __closer = o.__closer;
      o.__fd = invalid_value;
      return *this;
    }
    constexpr file_descriptor &operator=(const file_descriptor &) = delete;
    /**
     * @brief Returns the stored native descriptor.
     * @return Managed native descriptor.
     */
    constexpr native_handle_type fd() const noexcept {
      return __fd;
    }
    /**
     * @brief Creates a non-owning handle to the current descriptor.
     * @return Non-owning file handle.
     */
    constexpr file_handle<native_handle_type> borrow() const noexcept {
      return file_handle{__fd};
    }
    /**
     * @brief Invokes the closer on destruction when the descriptor is valid.
     */
    ~file_descriptor() noexcept {
      if (__fd != invalid_value) {
        __closer(__fd);
      }
    }

  private:
    closer_t __closer;
    native_handle_type __fd;
  };

  /**
   * @brief Concept that validates the presence of `fd()` and `borrow()` helpers.
   */
  export template <class fd_type>
  concept is_file_descriptor = requires(const fd_type &fd) {
    { fd.fd() } -> std::same_as<typename fd_type::native_handle_type>;
    { fd.borrow() } -> std::same_as<file_handle<typename fd_type::native_handle_type>>;
  };
}
