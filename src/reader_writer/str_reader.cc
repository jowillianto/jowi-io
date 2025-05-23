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
    const size_t read_count = -1;
    std::expected<std::string, fs_error> operator()(const is_basic_file auto &file) const {
      std::string buffer;
      auto buffer_inserter = std::back_inserter(buffer);
      size_t amount_left = read_count;
      while (amount_left > 0) {
        auto reader = basic_reader<4096>(amount_left);
        auto read_res = basic_reader<4096>{amount_left}(file);
        if (!read_res) { return std::unexpected{std::move(read_res.error())}; }
        std::ranges::copy_n(read_res -> buffer.begin(), read_res -> size, buffer_inserter);
        if (read_res -> size == 4096) {
          amount_left -= 4096;
        }
        else {
          amount_left = 0;
        }
      }
      return std::move(buffer);
    }
  };
}