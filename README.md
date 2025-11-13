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
  `std::expected<T, IoError>`, allowing clean monadic chaining.
- **`noexcept` default**: Most member functions are annotated `noexcept`, making
  it safe to rely on unwinding-free error paths.
- **RAII-first**: Owning wrappers clean up file descriptors automatically via
  injected closer functors.

## Module Overview

- `jowi.io`
  - Umbrella module that re-exports all IO submodules so downstream code can
    simply `import jowi.io;`.

- `jowi.io:buffer`
  - Concepts `IsReadableBuffer`, `IsWritableBuffer`, and `IsRwBuffer` are
    used to constrain templated IO helpers.
  - `DynBuffer` (member highlights):
    - `read_buf()`, `read_beg()`, `read_end()`, `read_size()`, `is_full()` – all
      `noexcept` views of the readable region.
    - `write_beg()`, `writable_size()`, `finish_write()`, `reset()`, `resize()` –
      manage the writable region; shrinking/growth never throws.
  - `FixedBuffer<N>` mirrors the same API using a compile-time capacity.

- `jowi.io:error`
  - `IoError` extends `std::exception`, captures `errno`, and formats messages
    via `std::format`. Use `IoError::str_error(errno)` to translate system
    errors.

- `jowi.io:file_descriptor`
  - `FileHandle::fd()` and `FileHandle::borrow()` expose the descriptor value
    without taking ownership.
  - `FileDescriptor` supports move construction/assignment, automatically
    calling the closer in its `noexcept` destructor.
  - `IsFileDescriptor` concept checks compatibility with generic utilities.

- `jowi.io:fd_type`
  - Defines the POSIX closer functor and aliases `FileType` (owning) and
    `FileHandleType` (non-owning) for use across the library.

- `jowi.io:is_file`
  - Concepts (`IsReadable`, `IsWritable`, `IsSeekable`, `IsSyncable`,
    `IsSized`, `IsFile`) encapsulate the capability contracts used across the
    library.
  - `BasicFile` exposes `handle()` and `invalid_file()` for lightweight
    descriptor passing.

- `jowi.io:readers`
  - `ByteReader` combines a buffer and readable file, providing:
    - `read_buf()` / `next_buf()` for buffer management.
    - `read_n(n)`, `read()`, `read_until(pred)`, and `read_until(char)` returning
      `std::expected<std::string, IoError>`.
    - `release()` to transfer ownership of the underlying file.
  - `LineReader` builds on `ByteReader` with `read_line()` and `read_lines()`.
  - `CsvReader` offers `read_row()` (optional row) and `read_rows()` for bulk
    ingestion.

- `jowi.io:local_file`
  - `LocalFile` member highlights (all `noexcept` unless returning
    `std::expected`):
    - `write(std::string_view)` and `read(buffer)`.
    - `seek(SeekMode, off_t)`, `seek_beg/seek_cur/seek_end`, `size()`,
      `truncate(off_t)`, `sync()`.
    - `handle()` returns a borrowed descriptor for integration with other APIs.
  - `OpenOptions` provides fluent toggles: `read()`, `write()`, `read_write()`,
    `truncate()`, `append()`, `create()`, then `open(path)`.

- `jowi.io:pipe`
  - `open_pipe(non_blocking)` yields `{ReaderPipe, WriterPipe}`.
  - `ReaderPipe::read(buffer)` and `WriterPipe::write(view)` forward to
    `sys_read`/`sys_write` while `is_readable()`/`is_writable()` wrap
    `sys_file_poller`.
- `jowi.io:in_mem_file`
  - `InMemFile` is a growable, vector-backed file object that satisfies the
    readable/writable/seekable portions of `IsFile`, making it ideal for tests
    or buffering data entirely in memory.

- `jowi.io:sys_util`
  - `sys_call(func, args...)` returns `expected<decltype(func(args...)), IoError>`.
  - `sys_call_void(func, args...)` discards success values while preserving
    `IoError` reporting.

- `jowi.io:sys_file`
  - Wrappers: `sys_seek`, `sys_truncate`, `sys_write`, `sys_sync`, `sys_read` –
    all return `std::expected` and leave buffers consistent on failure.
  - Utilities: `sys_file_poller::read_poller()` / `write_poller()` and
    `sys_fcntl`, `sys_fcntl_or_flag` for descriptor flags.

- `jowi.io:sys_net`
  - Address helpers: `Ipv4Address::addr()`, `port()`, `all_interface()`,
    `localhost()`, `sys_addr()`, `sys_domain()`; `LocalAddress::addr()` etc.
  - `SockRw` provides `read(buffer)`, `write(view)`, `is_readable()`,
    `is_writable()`, and `handle()` for connected descriptors.
  - `TcpSocket<addr>` offers `create(addr)`, `connect()`, and `listen(backlog)`.
  - `TcpListener<addr>` exposes `accept()`, `async_accept()`, `is_readable()`,
    and `handle()`.
  - `UdpSocket<addr>` provides `create(addr)`, `bind()`, and `connect()` helpers.

## Usage Notes

The modules are designed to compose: start from `jowi.io` for a single import,
then opt into specific concepts or concrete types. Buffer implementations work
hand-in-hand with the reader utilities, while the Unix back-end helpers
translate POSIX errors into the shared `IoError` type.

## Examples

### Reading a File Into Memory

```cpp
import jowi.io;
import std;

using namespace jowi::io;

int main() {
  auto file = OpenOptions{}.read().open("/var/log/system.log");
  if (!file) return 1;

  auto reader = ByteReader{DynBuffer{4096}, std::move(*file)};
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

  DynBuffer buf{32};
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
  auto addr = Ipv4Address{}.all_interface().port(8080);
  auto tcp = Ipv4TcpSocket::create(addr);
  if (!tcp) return 1;

  auto listener = std::move(*tcp).listen(64);
  if (!listener) return 1;

  auto next = listener->accept();
  if (!next) return 1;

  auto &[client_addr, connection] = *next;
  (void)client_addr;
  (void)connection;
  return 0;
}
```
