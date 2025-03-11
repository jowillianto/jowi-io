module;
#include <concepts>
#include <expected>
#include <filesystem>
#include <string_view>
export module moderna.io:file;
import :error;
import :seek_mode;
import :borrowed_file_descriptor;
import :is_file_descriptor;
/*
  This file describes the relationship between different file objects.
*/
namespace moderna::io {
  export template <class file_t, class write_t>
  concept is_writable = requires(file_t f) {
    { f.write(std::declval<write_t>()) } -> std::same_as<std::expected<void, fs_error>>;
  };
  export template <class file_t, class return_t>
  concept is_readable = requires(file_t f) {
    { f.read() } -> std::same_as<std::expected<return_t, fs_error>>;
    { f.read(std::declval<size_t>()) } -> std::same_as<std::expected<return_t, fs_error>>;
    { f.is_readable() } -> std::same_as<std::expected<bool, fs_error>>;
  };
  export template <class file_t>
  concept is_seekable = requires(file_t f) {
    {
      f.seek(std::declval<size_t>(), std::declval<seek_mode>())
    } -> std::same_as<std::expected<off_t, fs_error>>;
    { f.tell() } -> std::same_as<std::expected<off_t, fs_error>>;
    { f.seek_begin() } -> std::same_as<std::expected<off_t, fs_error>>;
    { f.seek_end() } -> std::same_as<std::expected<off_t, fs_error>>;
  };
  export template <class file_t>
  concept is_syncable = requires(file_t f) {
    { f.sync() } -> std::same_as<std::expected<void, fs_error>>;
  };
  export template <class exception_t>
  concept is_exception = requires(exception_t e) {
    { e.what() } -> std::convertible_to<std::string_view>;
  };

  export template <class file_t>
  concept is_file = requires(const file_t file) {
    { file.fd() } -> is_file_descriptor;
  };
  export template <class file_t>
  concept is_borroweable_file = requires(const file_t file) {
    is_file<file_t>;
    { file.borrow() } -> is_file;
  };

  /*
    Factory function for files.
  */
  export template <class opener_t>
  concept is_file_opener = requires(const opener_t opener) {
    { opener.open() } -> is_file;
  };
}