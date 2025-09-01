module;
#include <cstring>
#include <exception>
#include <format>
export module moderna.io:error;
import moderna.generic;
namespace moderna::io {
  export struct io_error : public std::exception {
  private:
    int __err_code;
    generic::fixed_string<64> __msg;

  public:
    template <class... Args> requires(std::formattable<Args, char> && ...)
    io_error(int err_code, std::format_string<Args...> fmt, Args &&...args) noexcept :
      __err_code{err_code}, __msg{} {
      __msg.emplace_format(fmt, std::forward<Args>(args)...);
    }

    int err_code() const noexcept {
      return __err_code;
    }
    const char *what() const noexcept {
      return __msg.begin();
    }

    static io_error str_error(int err_no) noexcept {
      return io_error{err_no, "{}", ::strerror(err_no)};
    }
  };
}
