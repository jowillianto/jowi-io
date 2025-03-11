module;
#include <concepts>
export module moderna.io:is_file_descriptor;

namespace moderna::io {
  export template <class T>
  concept is_file_descriptor = requires(const T fd) {
    typename T::native_handle_t;
    { fd.fd() } -> std::same_as<typename T::native_handle_t>;
  };

  export template <is_file_descriptor T> using native_handle_type = typename T::native_handle_t;
}