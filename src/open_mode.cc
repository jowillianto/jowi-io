module;

export module moderna.io:open_mode;

namespace moderna::io {
  export enum struct open_mode {
    read, // read only open mode
    write_truncate, // write truncate open mode
    write_append, // write append open mode
    read_write_truncate, // read write truncate open mode
    read_write_append, // read write append open mode
  };
}