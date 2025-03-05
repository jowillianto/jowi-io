module;

export module moderna.io:open_mode;

namespace moderna::io {
  export enum struct open_mode {
    read,
    write_truncate,
    write_append,
    read_write_truncate,
    read_write_append
  };
}