module;
#include <unistd.h>
export module moderna.io:fd_type;
import :file_descriptor;

namespace moderna::io {
  /*
    In this file, for Cross Platform Compatibility, the type of the file descriptor and file closer
    is expected to be declared over here.
  */

  /*
    DEFINITION BEGIN
  */
  /*
    Declares the file closer
  */
  export struct file_closer {
    void operator()(int fd) const noexcept {
      close(fd);
    }
  };

  /*
    Declares the native file descriptor type
  */
  export using file_type = file_descriptor<int, file_closer>;
}