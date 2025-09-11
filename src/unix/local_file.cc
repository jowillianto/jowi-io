module;
#include <sys/fcntl.h>
#include <bitset>
#include <expected>
#include <filesystem>
#include <unistd.h>
export module jowi.io:local_file;
import jowi.generic;
import :fd_type;
import :error;
import :sys_file;
import :is_file;

namespace jowi::io {
  namespace fs = std::filesystem;

  export struct local_file {
  private:
    file_type __f;
    bool __eof;
    local_file(file_type f) noexcept : __f{std::move(f)}, __eof{false} {}
    friend struct open_options;

  public:
    std::expected<size_t, io_error> write(std::string_view v) noexcept {
      return sys_write(__f.fd(), v);
    }
    template <size_t N> std::expected<void, io_error> read(generic::fixed_string<N> &buf) noexcept {
      return sys_read(__f.fd(), buf);
    }
    bool eof() const noexcept {
      return __eof;
    }
    std::expected<off_t, io_error> seek(seek_mode s, off_t offset) noexcept {
      return sys_seek(__f.fd(), s, offset);
    }
    std::expected<off_t, io_error> seek_beg(off_t offset) noexcept {
      return seek(seek_mode::START, offset);
    }
    std::expected<off_t, io_error> seek_cur(off_t offset) noexcept {
      return seek(seek_mode::CURRENT, offset);
    }
    std::expected<off_t, io_error> seek_end(off_t offset) noexcept {
      return seek(seek_mode::END, offset);
    }
    std::expected<size_t, io_error> size() noexcept {
      return seek(seek_mode::CURRENT, 0).and_then([&](off_t cur_pos) {
        return seek(seek_mode::END, 0).and_then([&](size_t fsize) {
          return seek(seek_mode::START, cur_pos).transform([&](auto &&) { return fsize; });
        });
      });
    }
    std::expected<off_t, io_error> truncate(off_t size) noexcept {
      return sys_truncate(__f.fd(), size);
    }
    std::expected<void, io_error> sync() noexcept {
      return sys_sync(__f.fd());
    }
    file_handle<int> handle() const {
      return __f.borrow();
    }
  };

  export struct open_options {
  private:
    std::bitset<32> __opt;

  public:
    open_options() noexcept : __opt{0} {}
    open_options &read() noexcept {
      __opt.set(0, false);
      __opt.set(1, false);
      __opt |= O_RDONLY;
      return *this;
    }
    open_options &write() noexcept {
      __opt.set(0, false);
      __opt.set(1, false);
      __opt |= O_WRONLY;
      return *this;
    }
    open_options &read_write() noexcept {
      __opt.set(0, false);
      __opt.set(1, false);
      __opt |= O_RDWR;
      return *this;
    }
    open_options &truncate() noexcept {
      __opt |= O_TRUNC;
      return *this;
    }
    open_options &append() noexcept {
      __opt |= O_APPEND;
      return *this;
    }
    open_options &create() noexcept {
      __opt |= O_CREAT;
      return *this;
    }
    std::expected<local_file, io_error> open(const fs::path &p) const noexcept {
      return sys_call(::open, p.c_str(), static_cast<int>(__opt.to_ulong()))
        .transform([](auto &&fd) { return local_file{file_type{fd}}; });
    }
  };
}