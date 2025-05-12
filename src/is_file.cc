module;
#include <concepts>
#include <expected>
export module moderna.io:is_file;
import :error;
import :file_descriptor;

namespace moderna::io {
  /*
    is_basic_file
    Check if the current file object can produce a file descriptor.
  */
  export template <class file_type>
  concept is_basic_file = requires(const file_type file) {
    { file.fd() } -> is_file_descriptor;
  };

  export template <is_basic_file file_type>
  decltype(std::declval<file_type>().fd())::native_handle_type get_native_handle(
    const file_type &file
  ) {
    return file.fd().fd();
  }

  /*
    is_file_writer
    Checks if an object is a writer.
  */
  export template <class writer_type, class file_type>
  concept is_file_writer =
    requires(
      const writer_type &writer,
      const file_type &file,
      const typename writer_type::value_type &value
    ) {
      { std::declval<typename writer_type::value_type> };
      { std::declval<typename writer_type::result_type> };
      {
        writer(file, value)
      } -> std::same_as<std::expected<typename writer_type::result_type, fs_error>>;
    } &&
    is_basic_file<file_type>;

  /*
    is_file_reader
    Checks if an object is a reader.
  */
  export template <class reader_type, class file_type>
  concept is_file_reader = requires(const reader_type &reader, const file_type &file) {
    { std::declval<typename reader_type::result_type> };
    { reader(file) } -> std::same_as<std::expected<typename reader_type::result_type, fs_error>>;
  } && is_basic_file<file_type>;

  /*
    is_file_operator
    Checks if an object can be used as a file operator
  */
  export template <class operator_type, class file_type>
  concept is_file_operator = requires(const operator_type &op, const file_type &file) {
    { std::declval<typename operator_type::result_type> };
    { op(file) } -> std::same_as<std::expected<typename operator_type::result_type, fs_error>>;
  } && is_basic_file<file_type>;
}