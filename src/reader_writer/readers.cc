module;
#include <algorithm>
#include <concepts>
#include <expected>
#include <iterator>
#include <string>
#include <string_view>
export module jowi.io:readers;
import jowi.generic;
import :is_file;
import :error;

namespace jowi::io {
  export template <uint64_t buf_size, is_readable<buf_size> file_type> struct byte_reader {
  private:
    file_type __f;
    generic::fixed_string<buf_size> __buf;
    const char *__last_read;

    std::expected<std::string_view, io_error> __buf_read_n(uint64_t n) noexcept {
      return read_buf().transform([&](auto &&) {
        uint64_t unread_size = std::distance(__last_read, __buf.cend());
        uint64_t n_to_read = std::min(n, unread_size);
        const char *rbeg = __last_read;
        const char *rend = rbeg + n_to_read;
        __last_read = rend;
        return std::string_view{rbeg, rend};
      });
    }

    template <std::invocable<char> F> requires(std::same_as<std::invoke_result_t<F, char>, bool>)
    std::expected<std::pair<std::string_view, bool>, io_error> __buf_read_until(F &&f) noexcept {
      return read_buf().transform([&](auto &&buf) {
        const char *rbeg = __last_read;
        const char *rend = std::ranges::find_if(rbeg, __buf.cend(), std::forward<F>(f));
        __last_read = std::min(rend + 1, __buf.cend());
        return std::pair{std::string_view{rbeg, rend}, rend != __buf.cend()};
      });
    }

    std::expected<void, io_error> __recursive_read_n(uint64_t n, std::string &it) {
      std::expected<void, io_error> res;
      while (n != 0) {
        auto buf = __buf_read_n(n);
        if (!buf) {
          res = std::unexpected{buf.error()};
          break;
        }
        it.append_range(*buf);
        if (buf->length() == 0) {
          break;
        }
        n -= buf->length();
      }
      return res;
    }

    template <std::invocable<char> F> requires(std::same_as<std::invoke_result_t<F, char>, bool>)
    std::expected<void, io_error> __recursive_read_until(F &&f, std::string &it) {
      std::expected<void, io_error> res;
      while (true) {
        auto buf = __buf_read_until(std::forward<F>(f));
        if (!buf) {
          res = std::unexpected{buf.error()};
          break;
        }
        it.append_range(buf->first);
        if (buf->first.length() == 0 || buf->second) {
          break;
        }
      }
      return res;
    }

  public:
    byte_reader(file_type f) noexcept : __f{std::move(f)}, __buf{}, __last_read{__buf.begin()} {}

    /*
      Buffer Control
    */
    std::expected<std::string_view, io_error> read_buf() noexcept {
      if (__last_read == __buf.end()) {
        return next_buf();
      }
      return std::string_view{__last_read, __buf.end()};
    }
    std::expected<std::string_view, io_error> next_buf() noexcept {
      __buf.truncate();
      return __f.read(__buf).transform([&]() {
        __last_read = __buf.begin();
        return std::string_view{__buf};
      });
    }
    /*
      Read N in buffer.
    */
    std::expected<std::string, io_error> read_n(uint64_t n) {
      std::string buf;
      // auto it = std::back_inserter(buf);
      return __recursive_read_n(n, buf).transform([&]() { return std::move(buf); });
    }

    std::expected<std::string, io_error> read() {
      return read_n(-1);
    }

    template <std::invocable<char> F> requires(std::same_as<std::invoke_result_t<F, char>, bool>)
    std::expected<std::string, io_error> read_until(F &&f) {
      std::string buf;
      // auto it = std::back_inserter(buf);
      return __recursive_read_until(std::forward<F>(f), buf).transform([&]() {
        return std::move(buf);
      });
    }

    std::expected<std::string, io_error> read_until(char r) {
      return read_until([r](char l) { return l == r; });
    }

    file_type release() && noexcept {
      return std::move(__f);
    }
  };

  export template <uint64_t buf_size, is_readable<buf_size> file_type>
  byte_reader<buf_size, file_type> make_byte_reader(file_type f) {
    return byte_reader<buf_size, file_type>{std::move(f)};
  }

  export template <uint64_t buf_size, is_readable<buf_size> file_type> struct line_reader {
  private:
    byte_reader<buf_size, file_type> __reader;

    std::expected<void, io_error> __read_lines(
      std::back_insert_iterator<std::vector<std::string>> &it
    ) {
      std::expected<void, io_error> res;
      while (true) {
        auto line = read_line();
        if (!line) {
          res = std::unexpected{line.error()};
          break;
        }
        if (line->empty()) {
          break;
        }
        it = *line;
      }
      return res;
    }

  public:
    line_reader(file_type f) noexcept : __reader{std::move(f)} {}

    std::expected<std::string, io_error> read_line() {
      return __reader.read_until('\n');
    }

    std::expected<std::vector<std::string>, io_error> read_lines() {
      std::vector<std::string> lines;
      auto it = std::back_inserter(lines);
      return __read_lines(it).transform([&]() { return std::move(lines); });
    }

    file_type release() && noexcept {
      return __reader.release();
    }
  };

  export template <uint64_t buf_size, is_readable<buf_size> file_type>
  line_reader<buf_size, file_type> make_line_reader(file_type f) {
    return line_reader<buf_size, file_type>{std::move(f)};
  }

  export template <uint64_t buf_size, is_readable<buf_size> file_type> struct csv_reader {
  private:
    byte_reader<buf_size, file_type> __reader;
    char __delim;

    std::expected<void, io_error> __read_rows(
      std::back_insert_iterator<std::vector<std::vector<std::string>>> &it
    ) {
      std::expected<void, io_error> res;
      while (true) {
        auto row = read_row();
        if (!row) {
          res = std::unexpected{row.error()};
        }
        if (!(*row)) {
          break;
        }
        it = std::move(**row);
      }
      return res;
    }

  public:
    csv_reader(file_type f, char delim) noexcept : __reader{std::move(f)}, __delim{delim} {}

    std::expected<std::optional<std::vector<std::string>>, io_error> read_row() {
      std::vector<std::string> cols;
      auto it = std::back_inserter(cols);
      return __reader.read_until('\n').transform(
        [&](std::string &&line) -> std::optional<std::vector<std::string>> {
          if (line.empty()) {
            return std::nullopt;
          }
          std::optional<uint64_t> quote_pos = std::nullopt;
          uint64_t col_beg_id = 0;
          for (uint64_t i = 0; i < line.length(); i += 1) {
            char x = line[i];
            if (x == ',' && !quote_pos) {
              it = std::string{line.begin() + col_beg_id, line.begin() + i};
              col_beg_id = i + 1;
            } else if (x == '"' && !quote_pos) {
              quote_pos = i;
            } else if (x == '"' && quote_pos) {
              quote_pos = std::nullopt;
            }
          }
          if (col_beg_id != line.length()) {
            it = std::string{line.begin() + col_beg_id, line.begin() + line.length() - 1};
          }
          return std::move(cols);
        }
      );
    }

    std::expected<std::vector<std::vector<std::string>>, io_error> read_rows() {
      std::vector<std::vector<std::string>> cols;
      auto it = std::back_inserter(cols);
      return __read_rows(it).transform([&]() { return std::move(cols); });
    }

    file_type release() && noexcept {
      return __reader.release();
    }
  };

  export template <uint64_t buf_size, is_readable<buf_size> file_type>
  csv_reader<buf_size, file_type> make_csv_reader(file_type f, char delim = ',') {
    return csv_reader<buf_size, file_type>{std::move(f), delim};
  }
}