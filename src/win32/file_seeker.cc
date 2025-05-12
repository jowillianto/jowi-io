module;
#include <cerrno>
#include <cstring>
#include <expected>
export module moderna.io:file_seeker;
import :is_file;
import :seek_mode;
import :error;

namespace moderna::io {
  export struct file_seeker {
    // using result_type = off_t;
    // off_t offset;
    // seek_mode mode;
    // std::expected<off_t, fs_error> operator()(const is_basic_file auto &file) const noexcept {
    //   off_t current_offset = lseek(get_native_handle(file), offset, get_seek_flags());
    //   int err_no = errno;
    //   if (current_offset == -1) {
    //     return std::unexpected{fs_error::make(err_no, strerror(errno))};
    //   }
    //   return current_offset;
    // }

    // int get_seek_flags() const noexcept {
    //   switch (mode) {
    //     case seek_mode::current:
    //       return SEEK_CUR;
    //     case seek_mode::start:
    //       return SEEK_SET;
    //     case seek_mode::end:
    //       return SEEK_END;
    //   }
    // }

    // static file_seeker begin_seeker() noexcept {
    //   return file_seeker{0, seek_mode::start};
    // }
    // static file_seeker end_seeker() noexcept {
    //   return file_seeker{0, seek_mode::end};
    // }
    // static file_seeker current_seeker() noexcept {
    //   return file_seeker{0, seek_mode::current};
    // }
  };
}