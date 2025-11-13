module;
#include <unistd.h>
#include <utility>
export module jowi.io:fd_type;
import jowi.generic;

namespace jowi::io {
  /**
   * @brief Defines platform-specific file descriptor helpers.
   */

  /**
   * @brief Functor responsible for closing native file descriptors.
   */
  export struct FileCloser {
    /**
     * @brief Invokes `close` on the supplied descriptor.
     * @param fd Native file descriptor to close.
     */
    void operator()(int fd) const noexcept {
      close(fd);
    }
  };

  export using FileDescriptor = generic::UniqueHandle<int, FileCloser>;

  struct FileDescriptorMover {
    FileDescriptor f;
    FileDescriptor operator()() {
      return std::move(f);
    }
  };

  export struct BasicOsFile {
  private:
    int __fd;

  public:
    BasicOsFile(int fd) : __fd{fd} {}

    int native_handle() const noexcept {
      return __fd;
    }
  };
}
