module;
#include <unistd.h>
export module moderna.io:stdstream;
import :file_opener;
import :is_file_descriptor;
import :borrowed_file_descriptor;
import :file_reader;
import :file_writer;

namespace moderna::io {
  export file_reader<borrowed_file_descriptor<native_handle_type<unix_fd>>> stdin() {
    return file_reader{borrowed_file_descriptor{STDIN_FILENO}};
  }
  export file_writer<borrowed_file_descriptor<native_handle_type<unix_fd>>> stdout() {
    return file_writer{borrowed_file_descriptor{STDOUT_FILENO}};
  }
  export file_writer<borrowed_file_descriptor<native_handle_type<unix_fd>>> stderr() {
    return file_writer{borrowed_file_descriptor{STDERR_FILENO}};
  }
}