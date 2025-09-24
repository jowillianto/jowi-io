module;
#include <cstring>
#include <exception>
#include <format>
export module jowi.io:error;
import jowi.generic;

/**
 * @file error.cc
 * @brief Error types specific to IO operations.
 */
namespace jowi::io {
  /**
   * @brief Exception wrapper carrying an errno value and formatted message.
   */
  export struct io_error : public std::exception {
  private:
    int __err_code;
    generic::fixed_string<64> __msg;

  public:
    /**
     * @brief Constructs an IO error with errno and formatted description.
     * @param err_code Originating errno value.
     * @param fmt Format string for the message.
     * @param args Format arguments.
     */
    template <class... Args> requires(std::formattable<Args, char> && ...)
    io_error(int err_code, std::format_string<Args...> fmt, Args &&...args) noexcept :
      __err_code{err_code}, __msg{} {
      __msg.emplace_format(fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief Returns the stored errno value.
     * @return Errno captured at construction.
     */
    int err_code() const noexcept {
      return __err_code;
    }
    /**
     * @brief Returns the formatted error message.
     * @return Null-terminated error description.
     */
    const char *what() const noexcept {
      return __msg.begin();
    }

    /**
     * @brief Creates an IO error using `strerror` for the supplied errno.
     * @param err_no Error number used to produce the message.
     * @return IO error containing errno and associated string message.
     */
    static io_error str_error(int err_no) noexcept {
      return io_error{err_no, "{}", ::strerror(err_no)};
    }
  };
}
