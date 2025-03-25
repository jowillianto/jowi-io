module;
#include <expected>
#include <string>
#include <unistd.h>
#include <utility>
export module moderna.io:rw_file;
import :is_borroweable_file_descriptor;
import :open_mode;
import :borrowed_file_descriptor;
import :is_file_descriptor;
import :file_reader;
import :file_writer;
import :file_seeker;
import :file_syncer;
import :file;
import :error;
import :file_poller;

namespace moderna::io {
  export template <is_borroweable_file_descriptor fd_t, open_mode mode> struct rw_file {
  private:
    using borrowed_fd = borrowed_file_descriptor<native_handle_type<fd_t>>;

  public:
    rw_file(fd_t fd) : __fd{std::move(fd)} {}

    /*
      seek because this file is seekable
    */
    std::expected<off_t, fs_error> seek(size_t offset, seek_mode s_mode = seek_mode::start)
      const noexcept {
      return file_seeker{fd()}.seek(offset, s_mode);
    }
    std::expected<off_t, fs_error> tell() const noexcept {
      return seek(0, seek_mode::current);
    }
    std::expected<off_t, fs_error> seek_end() const noexcept {
      return seek(0, seek_mode::end);
    }
    std::expected<bool, fs_error> is_eof() const noexcept {
      return tell().and_then([&](auto cur_offset) {
        return size().transform([&](auto fsize) { return cur_offset == fsize; });
      });
    }
    std::expected<off_t, fs_error> size() const noexcept {
      return tell().and_then([&](auto cur_offset) {
        return seek_end().and_then([&](auto fsize) {
          return seek(cur_offset).transform([&](auto cur_offset) { return fsize; });
        });
      });
    }
    std::expected<void, fs_error> sync(bool flush_meta) const noexcept {
      return file_syncer{fd()}.sync(flush_meta);
    }

    /*
      read with a specific reader
    */
    template <is_readable<borrowed_fd> reader_t>
    auto read(const reader_t &reader) const
      requires(mode != open_mode::write_append && mode != open_mode::write_truncate)
    {
      return reader.read(fd());
    }
    /*
      write with a specific writer
    */
    template <class write_t, is_writable<borrowed_fd, write_t> writer_t>
    auto write(write_t &&w, const writer_t &writer) const
      requires(mode != open_mode::read)
    {
      return writer.write(fd(), std::forward<write_t>(w));
    }

    /*
      Additional shortcut functions for read and write.
    */
    std::expected<std::string, fs_error> read(size_t nbytes) const {
      return read(str_reader{nbytes});
    }
    std::expected<std::string, fs_error> read() const {
      return read(str_eof_reader{});
    }
    std::expected<char, fs_error> read_one() const {
      return read(char_reader{});
    }
    std::expected<void, fs_error> write(std::string_view d) const {
      return write(d, str_writer{});
    }
    std::expected<void, fs_error> write_one(char x) const {
      return write(x, char_writer{});
    }

    // Readable or non readable
    std::expected<bool, fs_error> is_readable(int timeout = 0) const
      requires(mode != open_mode::write_append && mode != open_mode::write_truncate)
    {
      return file_poller::make_read_poller(timeout).poll_binary(fd());
    }
    std::expected<bool, fs_error> is_writable(int timeout = 0) const
      requires(mode != open_mode::read)
    {
      return file_poller::make_read_poller(timeout).poll_binary(fd());
    }

    borrowed_fd fd() const noexcept {
      return __fd.borrow();
    }
    rw_file<borrowed_fd, mode> borrow() const noexcept {
      return rw_file{fd()};
    }

  private:
    fd_t __fd;
  };
}