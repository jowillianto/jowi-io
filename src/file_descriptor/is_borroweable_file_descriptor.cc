module;
#include <concepts>
export module moderna.io:is_borroweable_file_descriptor;
import :borrowed_file_descriptor;
import :is_file_descriptor;

namespace moderna::io {
  export template <class T>
  concept is_borroweable_file_descriptor = requires(const T t) {
    is_file_descriptor<T>;
    { t.borrow() } -> std::same_as<borrowed_file_descriptor<typename T::native_handle_t>>;
  };
}