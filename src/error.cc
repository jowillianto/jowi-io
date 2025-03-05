module;
#include <array>
#include <cstring>
#include <string_view>
export module moderna.io:error;

namespace moderna::io {
  export struct fs_error {
    int err_code;
    std::array<char, 32> err_msg;

    fs_error(int err_code, const char *msg) noexcept {
      err_code = err_code;
      strncpy(err_msg.data(), msg, err_msg.size() - 1);
      err_msg[err_msg.size() - 1] = '\0';
    }
    fs_error(const fs_error &) = default;
    fs_error(fs_error &&) noexcept = default;
    fs_error &operator=(const fs_error &) = default;
    fs_error &operator=(fs_error &&) noexcept = default;

    const char *what() const noexcept {
      return err_msg.data();
    }
    std::string_view msg_view() const noexcept {
      return std::string_view{err_msg.data()};
    }
  };
}
