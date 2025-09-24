module;
#include <unistd.h>
export module jowi.io:fd_type;
import :file_descriptor;

namespace jowi::io {
  /**
   * @brief Defines platform-specific file descriptor helpers.
   */

  /**
   * @brief Functor responsible for closing native file descriptors.
   */
  export struct file_closer {
    /**
     * @brief Invokes `close` on the supplied descriptor.
     * @param fd Native file descriptor to close.
     */
    void operator()(int fd) const noexcept {
      close(fd);
    }
  };

  /**
   * @brief Owning file descriptor type for POSIX systems.
   */
  export using file_type = file_descriptor<int, file_closer>;
  /**
   * @brief Non-owning view over a native file descriptor.
   */
  export using file_handle_type = file_handle<int>;
}
