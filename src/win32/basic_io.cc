module;
#include <array>
#include <cerrno>
#include <cstring>
#include <expected>
#include <string_view>
export module moderna.io:basic_io;
import :is_file;
import :error;

namespace moderna::io {
  /*
    basic_reader
    wraps the read system call so that it reads into a temporary buffer array
  */
  export template <size_t N> struct basic_reader {
    using result_type = std::pair<std::array<char, N>, int>;
    std::expected<result_type, fs_error> operator()(const is_basic_file auto &file) {
      // std::array<char, N + 1> buffer;
      // int read_count = read(get_native_handle(file), static_cast<void *>(buffer.data()), N);
      // int err_no = errno;
      // if (read_count == -1) {
      //   return std::unexpected{fs_error::make(err_no, strerror(err_no))};
      // } else {
      //   buffer[read_count] = '\0';
      //   return std::pair{buffer, read_count};
      // }
    }
  };

  /*
    basic_writer
    wraps the write system call so that it writes and return the amount written. Compose higher
    order writers with this writer.
  */
  export struct basic_writer {
    using value_type = std::string_view;
    using result_type = int;
    std::expected<result_type, fs_error> operator()(const is_basic_file auto &file, value_type v) {
      // int write_count = read(get_native_handle(file), static_cast<void *>(v.data()), v.size());
      // int err_no = errno;
      // if (write_count == -1) {
      //   return std::unexpected{fs_error::make(err_no, strerror(err_no))};
      // }
      // return write_count;
    }
  };
}