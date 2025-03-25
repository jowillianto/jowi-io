module;
#include <unistd.h>
export module moderna.io:stdstream;
import :file_opener;
import :is_file_descriptor;
import :borrowed_file_descriptor;
import :file_reader;
import :file_writer;

namespace moderna::io {
  export auto std_in = readable_file{borrowed_file_descriptor{STDIN_FILENO}};
  export auto std_out = writable_file{borrowed_file_descriptor{STDOUT_FILENO}};
  export auto std_err = writable_file{borrowed_file_descriptor{STDERR_FILENO}};
}