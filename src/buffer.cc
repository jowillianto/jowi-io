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
  concept WritableBuffer = requires(buffer_type b, const buffer_type cb, size_t w_size) {
    // start of the writable buffer
    { b.write_beg() } -> std::same_as<void *>;
    /*
     * mark the buffer as n bytes written. ASSUMING CONTIGUOUS WRITES.
     */
    { b.mark_write(w_size) } -> std::same_as<size_t>;
    /*
     * amount of bytes writable into write_beg, this should be CONTIGUOUS WRITES.
     */
    { cb.writable_size() } -> std::same_as<size_t>;
    // if the buffer is full
    { cb.is_writable() } -> std::same_as<bool>;
  };

  export template <class buffer_type>
  concept ReadableBuffer = requires(buffer_type b, const buffer_type cb, size_t r_size) {
    /*
     * start of the readable buffer. HAS TO BE CONTIGUOUS with read_end()
     */
    { cb.read_beg() } -> std::same_as<const void *>;
    /*
     * end of the readable buffer. HAS TO BE CONTIGUOUS with read_beg()
     */
    { cb.read_end() } -> std::same_as<const void *>;
    /*
     * read the buffer as a string_view
     */
    { cb.read() } -> std::same_as<std::string_view>;
    /*
     * mark n amounts to be read.
     */
    { b.mark_read(r_size) } -> std::same_as<size_t>;
    /*
     * get the amount of readable bytes (contiguous)
     */
    { cb.readable_size() } -> std::same_as<size_t>;
    /*
     * if there is anything to read
     */
    { cb.is_readable() } -> std::same_as<bool>;
  };

  export template <class buffer_type>
  concept RwBuffer = ReadableBuffer<buffer_type> && WritableBuffer<buffer_type>;

  export template <WritableBuffer Buffer> struct BufferWriteMarker {
    Buffer &buf;
    void operator()(size_t size) const noexcept {
      buf.mark_write(size);
    }
  };
  /*
   * dyn buffer is a circular buffer that is designed to abstract away the use of a circular buffer
   * in read and write. DynBuffer controls two pointer
   * - read_ptr: the read_ptr will maximally be the same as the write_ptr.
   * - write_ptr: the write_ptr cannot write past the read_ptr.
   * i.e.
   * read_ptr can move to overlap with write_ptr and write_ptr can move to overlap with read_ptr
   */
  export struct DynBuffer {
  private:
    std::vector<char> __buf;
    size_t __read_ptr;
    size_t __write_ptr;

  public:
    constexpr DynBuffer(size_t capacity) : __buf(capacity, '\0'), __read_ptr(0), __write_ptr(0) {}

    constexpr size_t capacity() const noexcept {
      return __buf.size();
    }

    // Write Section
    constexpr void *write_beg() noexcept {
      return static_cast<void *>((__buf.begin() + __write_ptr).base());
    }

    constexpr size_t mark_write(size_t w_size) noexcept {
      size_t max_offset = max_write_offset();
      size_t prev_write = __write_ptr;
      __write_ptr = std::min(max_offset, __write_ptr + w_size);
      return __write_ptr - prev_write;
    }

    constexpr size_t writable_size() const noexcept {
      return max_write_offset() - __write_ptr;
    }

    constexpr bool is_writable() const noexcept {
      return writable_size() != 0;
    }

    // Read Section
    constexpr const void *read_beg() const noexcept {
      return static_cast<const void *>((__buf.begin() + __read_ptr).base());
    }

    constexpr const void *read_end() const noexcept {
      return static_cast<const void *>((__buf.begin() + max_read_offset()).base());
    }

    constexpr std::string_view read() const noexcept {
      return std::string_view{
        static_cast<const char *>(read_beg()), static_cast<const char *>(read_end())
      };
    }

    constexpr size_t mark_read(size_t read_size) noexcept {
      size_t max_offset = max_read_offset();
      size_t prev_read = __read_ptr;
      __read_ptr = std::min(max_offset, read_size + prev_read);
      if (__read_ptr == capacity() && __write_ptr == capacity()) __write_ptr = 0;
      __read_ptr = __read_ptr % capacity();
      return __read_ptr - prev_read;
    }

    constexpr size_t readable_size() const noexcept {
      // contiguity guarantees that the read offset > __read_ptr
      return max_read_offset() - __read_ptr;
    }

    constexpr bool is_readable() const noexcept {
      return readable_size() != 0;
    }

    inline constexpr size_t max_read_offset() const noexcept {
      // read offset is max_readable_index + 1.
      if (__read_ptr <= __write_ptr) return __write_ptr;
      else
        return capacity();
    }

    inline constexpr size_t max_write_offset() const noexcept {
      if (__write_ptr < __read_ptr) return __read_ptr;
      else
        return capacity();
    }
  };
}
