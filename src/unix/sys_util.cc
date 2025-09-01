module;
#include <cerrno>
#include <concepts>
#include <expected>
#include <unistd.h>
export module moderna.io:sys_util;
import :error;
import :fd_type;

namespace moderna::io {
  template <class F, class... Args>
    requires(std::invocable<F, Args...>)
  std::expected<std::invoke_result_t<F, Args...>, io_error> sys_call(F &&f, Args... args) noexcept {
    auto res = std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    int err_no = errno;
    if (res == -1) {
      return std::unexpected{io_error::str_error(err_no)};
    }
    return res;
  }
  template <class F, class... Args>
    requires(std::invocable<F, Args...>)
  std::expected<void, io_error> sys_call_void(F &&f, Args... args) noexcept {
    return sys_call(std::forward<F>(f), std::forward<Args>(args)...).transform([](auto &&) {});
  }
}
