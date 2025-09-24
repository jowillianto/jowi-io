module;
#include <concepts>
#include <expected>
#include <string_view>
export module jowi.io:is_file;
import jowi.generic;
import :error;
import :file_descriptor;
import :buffer;

/**
 * @file is_file.cc
 * @brief Concepts and helpers describing generic file capabilities.
 */

namespace jowi::io {
  /**
   * @brief File seek modes mirroring POSIX origin semantics.
   */
  export enum struct seek_mode { START, CURRENT, END };
  /**
   * @brief Concept describing write capability for a file abstraction.
   */
  export template <class file_type>
  concept is_writable = requires(file_type &file, std::string_view c) {
    { file.write(c) } -> std::same_as<std::expected<size_t, io_error>>;
  };

  /**
   * @brief Concept describing read capability for a file abstraction.
   */
  export template <class file_type>
  concept is_readable = requires(file_type &file, dyn_buffer &buf) {
    { file.read(buf) } -> std::same_as<std::expected<void, io_error>>;
  };

  /**
   * @brief Concept describing seek capability for a file abstraction.
   */
  export template <class file_type>
  concept is_seekable = requires(file_type &file, seek_mode m, long long offset) {
    { file.seek(offset) } -> std::same_as<std::expected<long long, io_error>>;
  };

  /**
   * @brief Concept describing synchronization capability for a file abstraction.
   */
  export template <class file_type>
  concept is_syncable = requires(file_type &file) {
    { file.sync() } -> std::same_as<std::expected<void, io_error>>;
  };

  /**
   * @brief Concept describing the ability to query file size.
   */
  export template <class file_type>
  concept is_sized = requires(file_type &file) {
    { file.size() } -> std::same_as<std::expected<size_t, io_error>>;
  };

  /**
   * @brief Concept ensuring access to an underlying file descriptor handle.
   */
  export template <class file_type>
  concept is_file = requires(file_type &file) {
    { file.handle() } -> is_file_descriptor;
  };

  /**
   * @brief Minimal file wrapper that forwards handle access.
   * @tparam file_type Native handle type.
   */
  export template <class file_type> struct basic_file {
  private:
    io::file_handle<file_type> __f;

  public:
    /**
     * @brief Constructs the wrapper from a native handle value.
     * @param fd Native file descriptor value.
     */
    basic_file(file_type fd) : __f{fd} {}
    /**
     * @brief Returns the non-owning file handle.
     * @return File handle view of the descriptor.
     */
    io::file_handle<file_type> handle() const {
      return __f;
    }

    /**
     * @brief Produces a sentinel invalid file wrapper.
     * @return Wrapper containing an invalid descriptor.
     */
    static basic_file invalid_file() {
      return basic_file{-1};
    }
  };
}
