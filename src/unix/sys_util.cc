module;
#include <cerrno>
#include <concepts>
#include <coroutine>
#include <expected>
#include <future>
#include <unistd.h>
export module jowi.io:sys_util;
import :error;
import :fd_type;

namespace jowi::io {
  template <class F, class... Args> requires(std::invocable<F, Args...>)
  std::expected<std::invoke_result_t<F, Args...>, io_error> sys_call(
    F &&f, Args &&...args
  ) noexcept {
    auto res = std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    int err_no = errno;
    if (res == -1) {
      return std::unexpected{io_error::str_error(err_no)};
    }
    return res;
  }
  template <class F, class... Args> requires(std::invocable<F, Args...>)
  std::expected<void, io_error> sys_call_void(F &&f, Args &&...args) noexcept {
    return sys_call(std::forward<F>(f), std::forward<Args>(args)...).transform([](auto &&) {});
  }
}
