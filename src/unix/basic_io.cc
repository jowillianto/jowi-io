module;
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <expected>
#include <iterator>
#include <string_view>
#include <unistd.h>
export module moderna.io:basic_io;
import :is_file;
import :error;

namespace moderna::io {
  /*
    basic_reader
    wraps the read system call so that it reads into a temporary buffer array. This will cap the
    amount read at buffer_size.
  */
  export template <size_t buffer_size> struct basic_reader {
    const size_t read_count = buffer_size;
    struct result_type {
      std::array<char, buffer_size + 1> buffer;
      size_t size;
    };
    std::expected<result_type, fs_error> operator()(const is_basic_file auto &file) const noexcept {
      std::expected<result_type, fs_error> res{result_type{.size = 0}};
      int amount_to_read = std::min(read_count, buffer_size);
      int amount_read =
        read(get_native_handle(file), static_cast<void *>(res->buffer.data()), amount_to_read);
      int err_no = errno;
      if (amount_read == -1 && (err_no == EWOULDBLOCK || err_no == EAGAIN)) {
        res->size = 0;
      } else if (amount_read == -1) {
        res = std::unexpected{fs_error::make(err_no, strerror(err_no))};
      } else {
        res->size = static_cast<size_t>(amount_read);
        res->buffer[amount_read] = '\0';
      }
      return res;
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
    std::expected<result_type, fs_error> operator()(
      const is_basic_file auto &file, value_type v
    ) const noexcept {
      int write_count =
        write(get_native_handle(file), static_cast<const void *>(v.data()), v.size());
      int err_no = errno;
      if (write_count == -1) {
        return std::unexpected{fs_error::make(err_no, strerror(err_no))};
      }
      return write_count;
    }
  };
}