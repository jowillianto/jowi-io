module;
#include <cstring>
#include <exception>
#include <string_view>
export module moderna.io:error;
import moderna.generic;
namespace moderna::io {
  export class fs_error : public std::exception {
    int __code;
    generic::static_string<32> __msg;

  public:
    constexpr fs_error(int err_code, generic::static_string<32> msg) noexcept :
      __code{err_code}, __msg{std::move(msg)} {}

    constexpr int err_code() const noexcept {
      return __code;
    }

    const char *what() const noexcept {
      return __msg.begin();
    }

    std::string_view msg_view() const noexcept {
      return __msg.as_view();
    }

    template <size_t N>
    static constexpr fs_error make(int err_code, const char (&msg)[N])
      requires(N <= 32)
    {
      return fs_error{err_code, generic::make_static_string(msg).template resize<N>()};
    }
    static constexpr fs_error make(int err_code, std::string_view v) {
      return fs_error{err_code, generic::static_string<32>(v)};
    }
  };
}
