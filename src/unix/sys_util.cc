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

/**
 * @file unix/sys_util.cc
 * @brief Thin wrappers around POSIX syscalls providing std::expected semantics.
 */

namespace jowi::io {
  /**
   * @brief Invokes a syscall-like function and converts the result into `std::expected`.
   * @tparam F Callable type accepting the provided `Args`.
   * @tparam Args Argument types forwarded to the callable.
   * @param f Callable representing the syscall.
   * @param args Arguments forwarded to the callable.
   * @return Expected with the syscall result or `io_error` when the call fails.
   */
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
  /**
   * @brief Same as `sys_call` but discards the return value on success.
   * @tparam F Callable type accepting the provided `Args`.
   * @tparam Args Argument types forwarded to the callable.
   * @param f Callable representing the syscall.
   * @param args Arguments forwarded to the callable.
   * @return Empty expected on success or `io_error` on failure.
   */
  template <class F, class... Args> requires(std::invocable<F, Args...>)
  std::expected<void, io_error> sys_call_void(F &&f, Args &&...args) noexcept {
    return sys_call(std::forward<F>(f), std::forward<Args>(args)...).transform([](auto &&) {});
  }
}
