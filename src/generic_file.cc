module;
#include <expected>
#include <string>
export module moderna.io:generic_file;
import :is_file;
import :file_descriptor;
import :open_mode;
import :error;
import :seek_mode;
import :file_seeker;
import :file_syncer;
import :basic_io;
import :file_poller;
import :str_reader;

namespace moderna::io {
  /*
    generic_file is a fully cross platform wrapper for opening and performing file io. A generic
    file encompasses a lot of features that can be turned on or off on demand.
  */
  export template <
    is_file_descriptor fd_type,
    bool allow_read,
    bool allow_write,
    bool allow_seek,
    bool allow_poll,
    bool allow_sync>
  class generic_file {
    using self = generic_file<fd_type, allow_read, allow_write, allow_seek, allow_poll, allow_sync>;
    fd_type __fd;

  public:
    generic_file(fd_type fd) : __fd{std::move(fd)} {}

    /*
      Reads using a specific reader from the local file.
    */
    template <is_file_reader<self> reader_type>
    std::expected<typename reader_type::result_type, fs_error> read(const reader_type &reader) const
      requires(allow_read)
    {
      return reader(*this);
    }
    std::expected<std::string, fs_error> read(size_t read_count = -1) const
      requires(allow_read)
    {
      return read(str_reader{read_count});
    }

    /*
      Writes using a specific writer from the local file.
    */
    template <is_file_writer<self> writer_type, class write_type>
    std::expected<typename writer_type::result_type, fs_error> write(
      const writer_type &writer, write_type &&value
    ) const
      requires(allow_write)
    {
      return writer(*this, std::forward<write_type>(value));
    }
    auto write(std::string_view value) const
      requires(allow_write)
    {
      return write(basic_writer{}, value);
    }
    /*
      local file additionally are seekable by nature.
    */
    /*
      Seek to a specific location
    */
    std::expected<typename file_seeker::result_type, fs_error> seek(
      typename file_seeker::result_type offset, seek_mode m = seek_mode::start
    ) const noexcept
      requires(allow_seek)
    {
      return apply_operation(file_seeker{offset, m});
    }
    /*
      Point the pointer to the start of a file.
    */
    std::expected<typename file_seeker::result_type, fs_error> seek_begin() const noexcept
      requires(allow_seek)
    {
      return apply_operation(file_seeker::begin_seeker());
    }
    /*
      Point the pointer to the end of the file
    */
    std::expected<typename file_seeker::result_type, fs_error> seek_end() const noexcept
      requires(allow_seek)
    {
      return apply_operation(file_seeker::end_seeker());
    }
    /*
      Get the current file offset.
    */
    std::expected<typename file_seeker::result_type, fs_error> tell() const noexcept
      requires(allow_seek)
    {
      return apply_operation(file_seeker::begin_seeker());
    }
    /*
      Get the size of the file. The file pointer will not be changed. On error, the file pointer
      could point to the end of file or point at the current location.
    */
    std::expected<typename file_seeker::result_type, fs_error> size() const noexcept
      requires(allow_seek)
    {
      return tell().and_then([&](auto current_offset) {
        return seek_end().and_then([&](auto file_size) {
          return seek(current_offset, seek_mode::start).transform([&](auto &&) {
            return file_size;
          });
        });
      });
    }
    /*
      Flush the file
    */
    std::expected<void, fs_error> flush(bool with_metadata = false) const noexcept
      requires(allow_sync)
    {
      return apply_operation(file_syncer{with_metadata});
    }

    /*
      Check if the file is read or write ready
    */
    std::expected<bool, fs_error> is_read_ready(int timeout = 0) const noexcept
      requires(allow_poll)
    {
      return apply_operation(file_poller::read_poller());
    }

    std::expected<bool, fs_error> is_write_ready(int timeout = 0) const noexcept
      requires(allow_poll)
    {
      return apply_operation(file_poller::write_poller());
    }

    /*
      apply a specific operation to the local file
    */
    template <is_file_operator<self> operator_type>
    std::expected<typename operator_type::result_type, fs_error> apply_operation(
      const operator_type &op
    ) const {
      return op(*this);
    }

    file_handle<typename fd_type::native_handle_type> fd() const noexcept {
      return __fd.borrow();
    }
    fd_type fd() && {
      return std::move(__fd);
    }
  };

  export template <is_file_descriptor fd_type> struct pipe_file {
    generic_file<fd_type, true, false, false, true, false> reader;
    generic_file<fd_type, false, true, false, true, false> writer;
  };

  export struct tcp_address {
    std::string ip;
    int port;
  };

  export template <is_file_descriptor fd_type> struct tcp_host {
    generic_file<fd_type, false, false, false, false, false> file;
    const tcp_address host; // Current listening address
  };

  export template <is_file_descriptor fd_type> struct tcp_connection {
    generic_file<fd_type, true, true, false, true, false> file;
    const tcp_address host; // Current device address
    const tcp_address client; // Connected device address
  };
}