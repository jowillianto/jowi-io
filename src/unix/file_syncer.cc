module;
#include <cerrno>
#include <cstring>
#include <expected>
export module moderna.io:file_syncer;
import :is_file;
import :error;

namespace moderna::io {

  /*
    Flushes the file without waiting for buffer.
  */
  export struct file_syncer {
    bool flush_metadata = false;
    using result_type = void;
    std::expected<void, fs_error> operator()(const is_basic_file auto &file) const noexcept {
      if (flush_metadata) {
        int res = fdatasync(get_native_handle(file));
        int err_no = errno;
        if (res == -1) {
          return std::unexpected{fs_error::make(err_no, strerror(err_no))};
        }
      } else {
        int res = fsync(get_native_handle(file));
        int err_no = errno;
        if (res == -1) {
          return std::unexpected{fs_error::make(err_no, strerror(err_no))};
        }
      }
    }
  };
}