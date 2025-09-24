# IO
Exception-safe IO building blocks written in modern C++23 modules.

## Background
Working with raw file descriptors and POSIX APIs usually means manual error
handling. The `jowi.io` modules wrap those primitives with helpers that return
`std::expected`, enforce RAII semantics, and provide buffered reader utilities.
Most exported functions are marked `noexcept` so failures are funnelled through
`std::expected`, keeping control flow predictable.

## Requirements

- **Compiler**: A C++23-capable toolchain with module support (`import` is used
  throughout the library).
- **Standard library**: Requires `<expected>`, `<format>`, and other C++23
  facilities.

## Design Guarantees

- **`std::expected` everywhere**: Operations that can fail return
  `std::expected<T, io_error>`, allowing clean monadic chaining.
- **`noexcept` default**: Most member functions are annotated `noexcept`, making
  it safe to rely on unwinding-free error paths.
- **RAII-first**: Owning wrappers clean up file descriptors automatically via
  injected closer functors.

## Module Overview

- `jowi.io`
  - Umbrella module that re-exports all IO submodules so downstream code can
    simply `import jowi.io;`.

- `jowi.io:buffer`
  - Concepts `is_readable_buffer`, `is_writable_buffer`, and `is_rw_buffer` are
    used to constrain templated IO helpers.
  - `dyn_buffer` (member highlights):
    - `read_buf()`, `read_beg()`, `read_end()`, `read_size()`, `is_full()` – all
      `noexcept` views of the readable region.
    - `write_beg()`, `writable_size()`, `finish_write()`, `reset()`, `resize()` –
      manage the writable region; shrinking/growth never throws.
  - `fixed_buffer<N>` mirrors the same API using a compile-time capacity.

- `jowi.io:error`
  - `io_error` extends `std::exception`, captures `errno`, and formats messages
    via `std::format`. Use `io_error::str_error(errno)` to translate system
    errors.

- `jowi.io:file_descriptor`
  - `file_handle::fd()` and `file_handle::borrow()` expose the descriptor value
    without taking ownership.
  - `file_descriptor` supports move construction/assignment, automatically
    calling the closer in its `noexcept` destructor.
  - `is_file_descriptor` concept checks compatibility with generic utilities.

- `jowi.io:fd_type`
  - Defines the POSIX closer functor and aliases `file_type` (owning) and
    `file_handle_type` (non-owning) for use across the library.

- `jowi.io:is_file`
  - Concepts (`is_readable`, `is_writable`, `is_seekable`, `is_syncable`,
    `is_sized`, `is_file`) encapsulate the capability contracts used across the
    library.
  - `basic_file` exposes `handle()` and `invalid_file()` for lightweight
    descriptor passing.

- `jowi.io:readers`
  - `byte_reader` combines a buffer and readable file, providing:
    - `read_buf()` / `next_buf()` for buffer management.
    - `read_n(n)`, `read()`, `read_until(pred)`, and `read_until(char)` returning
      `std::expected<std::string, io_error>`.
    - `release()` to transfer ownership of the underlying file.
  - `line_reader` builds on `byte_reader` with `read_line()` and `read_lines()`.
  - `csv_reader` offers `read_row()` (optional row) and `read_rows()` for bulk
    ingestion.

- `jowi.io:local_file`
  - `local_file` member highlights (all `noexcept` unless returning
    `std::expected`):
    - `write(std::string_view)` and `read(buffer)`.
    - `seek(seek_mode, off_t)`, `seek_beg/seek_cur/seek_end`, `size()`,
      `truncate(off_t)`, `sync()`.
    - `handle()` returns a borrowed descriptor for integration with other APIs.
  - `open_options` provides fluent toggles: `read()`, `write()`, `read_write()`,
    `truncate()`, `append()`, `create()`, then `open(path)`.

- `jowi.io:pipe`
  - `open_pipe(non_blocking)` yields `{reader_pipe, writer_pipe}`.
  - `reader_pipe::read(buffer)` and `writer_pipe::write(view)` forward to
    `sys_read`/`sys_write` while `is_readable()`/`is_writable()` wrap
    `sys_file_poller`.

- `jowi.io:sys_util`
  - `sys_call(func, args...)` returns `expected<decltype(func(args...)), io_error>`.
  - `sys_call_void(func, args...)` discards success values while preserving
    `io_error` reporting.

- `jowi.io:sys_file`
  - Wrappers: `sys_seek`, `sys_truncate`, `sys_write`, `sys_sync`, `sys_read` –
    all return `std::expected` and leave buffers consistent on failure.
  - Utilities: `sys_file_poller::read_poller()` / `write_poller()` and
    `sys_fcntl`, `sys_fcntl_or_flag` for descriptor flags.

- `jowi.io:sys_net`
  - Address helpers: `ipv4_address::addr()`, `port()`, `all_interface()`,
    `localhost()`, `sys_addr()`, `sys_domain()`; `local_address::addr()` etc.
  - `sock_rw` provides `read(buffer)`, `write(view)`, `is_readable()`,
    `is_writable()`, and `handle()`.
  - `sock_free` manages sockets pre-connection via `non_blocking()`,
    `bind()`, `listen()`, `bind_listen()`, `connect()`, and `accept()`.
  - `sock_options(addr)` chooses `tcp()`/`udp()` and `create()` to obtain a
    `sock_free` instance.

## Usage Notes

The modules are designed to compose: start from `jowi.io` for a single import,
then opt into specific concepts or concrete types. Buffer implementations work
hand-in-hand with the reader utilities, while the Unix back-end helpers
translate POSIX errors into the shared `io_error` type.

## Examples

### Reading a File Into Memory

```cpp
import jowi.io;
import std;

using namespace jowi::io;

int main() {
  auto file = open_options{}.read().open("/var/log/system.log");
  if (!file) return 1;

  auto reader = byte_reader{dyn_buffer{4096}, std::move(*file)};
  auto contents = reader.read();
  if (!contents) return 1;

  std::println("{} bytes read", contents->size());
  return 0;
}
```

### Writing to a Pipe and Polling Readiness

```cpp
import jowi.io;
import std;

using namespace jowi::io;

int main() {
  auto pipe_pair = open_pipe();
  if (!pipe_pair) return 1;

  auto &[reader, writer] = *pipe_pair;
  auto write_res = writer.write("hello\n");
  if (!write_res) return 1;

  dyn_buffer buf{32};
  if (auto ready = reader.is_readable(); !ready || !*ready) return 1;

  if (auto read_res = reader.read(buf); !read_res) return 1;

  std::string_view payload{buf.read_beg(), buf.read_end()};
  std::println("pipe read: {}", payload);
  return 0;
}
```

### Creating a Listening Socket

```cpp
import jowi.io;
import std;

using namespace jowi::io;

int main() {
  auto addr = ipv4_address{}.all_interface().port(8080);
  auto sock = sock_options{addr}.tcp().create();
  if (!sock) return 1;

  auto listener = std::move(*sock).bind_listen(64);
  if (!listener) return 1;

  // Later: std::move(*listener).accept() -> sock_rw
  return 0;
}
```
