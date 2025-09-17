module;
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <string>
#include <string_view>
export module jowi.io:buffer;
import jowi.generic;

namespace jowi::io {
  export template <class buffer_type>
  concept is_readable_buffer = requires(buffer_type b, const buffer_type cb, uint64_t w_size) {
    { cb.read_buf() } -> std::same_as<std::string_view>;
    { cb.read_beg() } -> std::same_as<const char *>;
    { cb.read_end() } -> std::same_as<const char *>;
    { cb.read_size() } -> std::same_as<uint64_t>;
    { cb.is_full() } -> std::same_as<bool>;
  };
  export template <class buffer_type>
  concept is_writable_buffer = requires(buffer_type b, const buffer_type cb) {
    { b.write_beg() } -> std::same_as<char *>;
    { cb.writable_size() } -> std::same_as<uint64_t>;
    { b.finish_write(std::declval<uint64_t>()) } -> std::same_as<void>;
    { b.reset() } -> std::same_as<void>;
  };
  export template <class buffer_type>
  concept is_rw_buffer = is_readable_buffer<buffer_type> && is_writable_buffer<buffer_type>;

  export struct dyn_buffer {
  private:
    std::string __buf;
    uint64_t __buf_size;

  public:
    dyn_buffer(size_t init_buf_size = 2048) {
      __buf.reserve(init_buf_size);
      auto inserter = std::back_inserter(__buf);
      std::ranges::fill_n(inserter, init_buf_size, '\0');
      __buf_size = 0;
    }

    // is_readable_buffer
    const char *read_beg() const noexcept {
      return __buf.begin().base();
    }
    const char *read_end() const noexcept {
      return (__buf.begin() + __buf_size).base();
    }
    std::string_view read_buf() const noexcept {
      return std::string_view{read_beg(), read_end()};
    }
    uint64_t read_size() const noexcept {
      return __buf_size;
    }
    bool is_full() const noexcept {
      return __buf.size() == __buf_size;
    }

    // is_writable_buffer
    char *write_beg() noexcept {
      return (__buf.begin() + __buf_size).base();
    }
    uint64_t writable_size() const noexcept {
      return std::distance(
        static_cast<std::string::const_iterator>(__buf.begin() + __buf_size), __buf.end()
      );
    }
    void finish_write(uint64_t w_size) noexcept {
      w_size = std::min(w_size, writable_size());
      __buf_size += w_size;
    }
    void reset() noexcept {
      __buf_size = 0;
    }

    // specific

    /*
      resizes the current buffer
    */
    void resize(uint64_t new_size) noexcept {
      __buf.resize(new_size);
      __buf_size = std::min(__buf_size, new_size);
    }
  };

  export template <uint64_t N> struct fixed_buffer {
  private:
    generic::fixed_string<N> __buf;

  public:
    fixed_buffer() : __buf{} {}

    // is_readable_buffer
    std::string_view read_buf() const noexcept {
      return std::string_view{__buf};
    }
    const char *read_beg() const noexcept {
      return __buf.cbegin();
    }
    const char *read_end() const noexcept {
      return __buf.cend();
    }
    uint64_t read_size() const noexcept {
      return __buf.length();
    }
    bool is_full() const noexcept {
      return N == __buf.length();
    }

    // is_writable_buffer
    char *write_beg() noexcept {
      return __buf.end();
    }
    uint64_t writable_size() const noexcept {
      return __buf.empty_space();
    }
    void finish_write(uint64_t w_size) noexcept {
      w_size = std::min(w_size, writable_size());
      __buf.unsafe_set_length(w_size);
    }
    void reset() noexcept {
      __buf.unsafe_set_length(0);
    }
  };

  static_assert(is_rw_buffer<fixed_buffer<2048>>);
  static_assert(is_rw_buffer<dyn_buffer>);
}