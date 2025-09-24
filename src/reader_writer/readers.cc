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
import :buffer;

/**
 * @file reader_writer/readers.cc
 * @brief Generic buffered readers supporting bytes, lines, and CSV rows.
 */

namespace jowi::io {
  /**
   * @brief Buffered reader providing byte-wise access primitives.
   * @tparam buffer_type Buffer used to stage IO operations.
   * @tparam file_type Readable file-like type satisfying `is_readable`.
   */
  export template <is_rw_buffer buffer_type, is_readable file_type> struct byte_reader {
  private:
    file_type __f;
    buffer_type __buf;
    const char *__last_read;

    std::expected<std::string_view, io_error> __buf_read_n(uint64_t n) noexcept {
      return read_buf().transform([&](auto &&) {
        uint64_t unread_size = std::distance(__last_read, __buf.read_end());
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
        const char *rend = std::ranges::find_if(rbeg, __buf.read_end(), std::forward<F>(f));
        __last_read = std::min(rend + 1, __buf.read_end());
        return std::pair{std::string_view{rbeg, rend}, rend != __buf.read_end()};
      });
    }

    std::expected<std::pair<std::string_view, bool>, io_error> __buf_read_eq(char x) noexcept {
      return read_buf().transform([&](auto &&buf) {
        const char *rbeg = __last_read;
        const char *rend = std::ranges::find(rbeg, __buf.read_end(), x);
        __last_read = std::min(rend + 1, __buf.read_end());
        return std::pair{std::string_view{rbeg, rend}, rend != __buf.read_end()};
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

    std::expected<void, io_error> __recursive_read_eq(char x, std::string &it) {
      std::expected<void, io_error> res;
      while (true) {
        auto buf = __buf_read_eq(x);
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
    byte_reader(buffer_type buf, file_type f) :
      __f{std::move(f)}, __buf{std::move(buf)}, __last_read{__buf.read_beg()} {}

    /**
     * @brief Returns the current readable view, refilling when exhausted.
     * @return Read-only view over buffered bytes or IO error.
     */
    std::expected<std::string_view, io_error> read_buf() noexcept {
      if (__last_read == __buf.read_end()) {
        return next_buf();
      }
      return __buf.read_buf();
    }
    /**
     * @brief Refills the buffer from the underlying file.
     * @return Read-only view over the refreshed buffer or IO error.
     */
    std::expected<std::string_view, io_error> next_buf() noexcept {
      __buf.reset();
      return __f.read(__buf).transform([&]() {
        __last_read = __buf.read_beg();
        return __buf.read_buf();
      });
    }
    /**
     * @brief Reads a fixed number of bytes from the stream.
     * @param n Number of bytes to read, or `std::numeric_limits<uint64_t>::max()` for all data.
     * @return String containing the requested bytes or IO error.
     */
    std::expected<std::string, io_error> read_n(uint64_t n) {
      std::string buf;
      // auto it = std::back_inserter(buf);
      return __recursive_read_n(n, buf).transform([&]() { return std::move(buf); });
    }

    /**
     * @brief Reads the entire stream.
     * @return Accumulated bytes or IO error.
     */
    std::expected<std::string, io_error> read() {
      return read_n(-1);
    }

    template <std::invocable<char> F> requires(std::same_as<std::invoke_result_t<F, char>, bool>)
    /**
     * @brief Reads bytes until the predicate returns true.
     * @tparam F Predicate type returning `bool` for a supplied character.
     * @param f Predicate invoked for each character.
     * @return Accumulated bytes (excluding delimiter) or IO error.
     */
    std::expected<std::string, io_error> read_until(F &&f) {
      std::string buf;
      return __recursive_read_until(std::forward<F>(f), buf).transform([&]() {
        return std::move(buf);
      });
    }

    /**
     * @brief Reads bytes until the delimiter character is encountered.
     * @param r Delimiter character.
     * @return Accumulated bytes (excluding delimiter) or IO error.
     */
    std::expected<std::string, io_error> read_until(char r) {
      std::string buf;
      return __recursive_read_eq(r, buf).transform([&]() { return std::move(buf); });
    }

    /**
     * @brief Releases the underlying file object.
     * @return File instance previously owned by the reader.
     */
    file_type release() && noexcept {
      return std::move(__f);
    }
  };

  export template <is_rw_buffer buffer_type, is_readable file_type>
  /**
   * @brief Reader specializing in line-oriented extraction.
   * @tparam buffer_type Buffer type satisfying `is_rw_buffer`.
   * @tparam file_type Readable file abstraction.
   */
  struct line_reader : private byte_reader<buffer_type, file_type> {
  protected:
    using byte_reader = byte_reader<buffer_type, file_type>;

  private:
    /**
     * @brief Repeatedly reads lines into the supplied container.
     * @param it Back inserter receiving new lines.
     * @return Success or IO error from the underlying reader.
     */
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
    /**
     * @brief Constructs a line reader.
     * @param buf Buffer used for staged IO.
     * @param f Readable file or socket.
     */
    line_reader(buffer_type buf, file_type f) : byte_reader{std::move(buf), std::move(f)} {}

    /**
     * @brief Reads a single line terminated by `\n`.
     * @return Line contents including the newline or IO error.
     */
    std::expected<std::string, io_error> read_line() {
      return byte_reader::read_until('\n');
    }

    /**
     * @brief Reads successive non-empty lines until an empty line is encountered.
     * @return Collection of lines or IO error.
     */
    std::expected<std::vector<std::string>, io_error> read_lines() {
      std::vector<std::string> lines;
      auto it = std::back_inserter(lines);
      return __read_lines(it).transform([&]() { return std::move(lines); });
    }

    /**
     * @brief Releases the underlying file object.
     * @return File instance previously owned by the reader.
     */
    file_type release() && noexcept {
      return byte_reader::release();
    }
  };

  export template <is_rw_buffer buffer_type, is_readable file_type>
  /**
   * @brief Reader that produces CSV rows split on a configurable delimiter.
   * @tparam buffer_type Buffer type satisfying `is_rw_buffer`.
   * @tparam file_type Readable file abstraction.
   */
  struct csv_reader : private byte_reader<buffer_type, file_type> {
  protected:
    using byte_reader = byte_reader<buffer_type, file_type>;

  private:
    char __delim;

    /**
     * @brief Reads CSV rows into the supplied container until an empty row is encountered.
     * @param it Back inserter receiving parsed rows.
     * @return Success or IO error from the underlying reader.
     */
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
    /**
     * @brief Constructs a CSV reader.
     * @param buf Buffer used for staged IO.
     * @param f Readable file or socket.
     * @param delim Column delimiter (defaults to comma).
     */
    csv_reader(buffer_type buf, file_type f, char delim = ',') :
      byte_reader{std::move(buf), std::move(f)}, __delim{delim} {}

    /**
     * @brief Reads a single CSV row.
     * @return Optional row (nullopt when encountering an empty line) or IO error.
     */
    std::expected<std::optional<std::vector<std::string>>, io_error> read_row() {
      std::vector<std::string> cols;
      auto it = std::back_inserter(cols);
      return byte_reader::read_until('\n').transform(
        [&](std::string &&line) -> std::optional<std::vector<std::string>> {
          if (line.empty()) {
            return std::nullopt;
          }
          std::optional<uint64_t> quote_beg;
          std::optional<uint64_t> quote_end;
          uint64_t col_beg_id = 0;
          for (uint64_t i = 0; i < line.length(); i += 1) {
            char x = line[i];
            if (x == ',' && ((quote_beg && quote_end) || !(quote_beg || quote_end))) {
              uint64_t col_beg = quote_beg.value_or(col_beg_id);
              uint64_t col_end = quote_end.value_or(i);
              it = std::string{line.begin() + col_beg, line.begin() + col_end};
              quote_beg.reset();
              quote_end.reset();
              col_beg_id = i + 1;
            } else if (x == '"' && ((quote_beg && quote_end) || !(quote_beg || quote_end))) {
              quote_beg = i + 1;
              quote_end.reset();
            } else if (x == '"' && (quote_beg && !quote_end)) {
              quote_end = i;
            }
        }
        if (col_beg_id != line.length()) {
          it = std::string{line.begin() + col_beg_id, line.begin() + line.length() - 1};
        }
        return std::move(cols);
      }
      );
    }

    /**
     * @brief Reads all available CSV rows.
     * @return Collection of parsed rows or IO error.
     */
    std::expected<std::vector<std::vector<std::string>>, io_error> read_rows() {
      std::vector<std::vector<std::string>> cols;
      auto it = std::back_inserter(cols);
      return __read_rows(it).transform([&]() { return std::move(cols); });
    }

    /**
     * @brief Releases the underlying file object.
     * @return File instance previously owned by the reader.
     */
    file_type release() && noexcept {
      return byte_reader::release();
    }
  };
}
