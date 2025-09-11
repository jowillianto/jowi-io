module;
#include <concepts>
#include <expected>
#include <string_view>
export module jowi.io:is_file;
import jowi.generic;
import :error;
import :file_descriptor;

namespace jowi::io {
  export enum struct seek_mode { START, CURRENT, END };
  export template <class file_type>
  concept is_writable = requires(file_type &file, std::string_view c) {
    { file.write(c) } -> std::same_as<std::expected<size_t, io_error>>;
  };

  export template <class file_type, size_t N>
  concept is_readable = requires(file_type &file, generic::fixed_string<N> &buf) {
    { file.template read<N>(buf) } -> std::same_as<std::expected<void, io_error>>;
  };

  export template <class file_type>
  concept is_seekable = requires(file_type &file, seek_mode m, long long offset) {
    { file.seek(offset) } -> std::same_as<std::expected<long long, io_error>>;
  };

  export template <class file_type>
  concept is_syncable = requires(file_type &file) {
    { file.sync() } -> std::same_as<std::expected<void, io_error>>;
  };

  export template <class file_type>
  concept is_sized = requires(file_type &file) {
    { file.size() } -> std::same_as<std::expected<size_t, io_error>>;
  };
}