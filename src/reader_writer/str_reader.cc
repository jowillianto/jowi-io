module;
#include <algorithm>
#include <expected>
#include <iterator>
#include <string>
export module moderna.io:str_reader;
import :basic_io;
import :is_file;
import :error;

namespace moderna::io {
  export struct str_reader {
    using result_type = std::string;
    size_t read_count = -1;
    std::expected<std::string, fs_error> operator()(const is_basic_file auto &file) const {
      std::string buffer;
      auto buffer_inserter = std::back_inserter(buffer);
      return (*this)(file, buffer_inserter, read_count).transform([&]() {
        return std::move(buffer);
      });
    }
    std::expected<void, fs_error> operator()(
      const is_basic_file auto &file, std::back_insert_iterator<std::string> &it, size_t amount_left
    ) const {
      auto reader = basic_reader<4096>{amount_left};
      return reader(file).and_then([&](auto &&read_res) {
        std::ranges::copy_n(read_res.buffer.begin(), read_res.size, it);
        if (read_res.size == 4096) {
          return (*this)(file, it, amount_left - 4096);
        } else {
          return std::expected<void, fs_error>{};
        }
      });
    }
  };
}