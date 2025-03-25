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
  /*
    is_file
    checks if a given file object can produce a file descriptor by calling fd()
  */
  export template <class file_t>
  concept is_file = requires(const file_t file) {
    { file.fd() } -> is_file_descriptor;
  };

  /*
    Checks if a file object can be borrowed from. This usually means calling borrow() on the file
    descriptor.
  */
  export template <class file_t>
  concept is_borroweable_file = requires(const file_t file) {
    is_file<file_t>;
    { file.borrow() } -> is_file;
  };
  /*
    Checks if a file object can be written with the given type.
  */
  export template <class file_t, class fd_t, class write_t>
  concept is_writable = is_file_descriptor<fd_t> && requires(file_t f) {
    { f.write(std::declval<fd_t>(), std::declval<write_t>()) };
  };
  /*
    Checks if a file object can be read as the given type. This requires that the file reading
    process be endable. I.e. length endable or the file encoding itself allows for the detection
    of the length of the file.
  */
  export template <class reader_t, class fd_t>
  concept is_readable = is_file_descriptor<fd_t> && requires(reader_t f) {
    { f.read(std::declval<fd_t>()).value() };
  };

  /*
    Checks if a file can be seek as the given type. This requires that the file be seekable. The
    object relationship should prevent unseekable file to produce a seeker object.
  */
  export template <class file_t>
  concept is_seekable = requires(file_t f) {
    { f.seek(std::declval<size_t>(), std::declval<seek_mode>()).value() } -> std::same_as<off_t>;
    { f.tell() } -> std::same_as<off_t>;
    { f.seek_begin() } -> std::same_as<off_t>;
    { f.seek_end() } -> std::same_as<off_t>;
  };
  export template <class file_t>
  concept is_syncable = requires(file_t f) {
    { f.sync() };
  };
  export template <class exception_t>
  concept is_exception = requires(exception_t e) {
    { e.what() } -> std::convertible_to<std::string_view>;
  };

  /*
    Factory function for files.
  */
  export template <class opener_t>
  concept is_file_opener = requires(const opener_t opener) {
    { opener.open().value() } -> is_file;
  };
}