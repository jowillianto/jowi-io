module;
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

namespace moderna::io {
  export template <is_borroweable_file_descriptor fd_t, open_mode mode> struct rw_file {
  private:
    using borrowed_fd = borrowed_file_descriptor<native_handle_type<fd_t>>;

  public:
    rw_file(fd_t fd) : __fd{std::move(fd)} {}
    rw_file(rw_file &&) noexcept = default;
    rw_file(const rw_file &) = default;
    rw_file &operator=(rw_file &&) noexcept = default;
    rw_file &operator=(const rw_file &) = default;

    file_reader<borrowed_fd> get_reader() const & noexcept
      requires(mode != open_mode::write_append && mode != open_mode::write_truncate)
    {
      return file_reader{__fd.borrow()};
    }
    file_writer<borrowed_fd> get_writer() const & noexcept
      requires(mode != open_mode::read)
    {
      return file_writer{__fd.borrow()};
    }
    file_seeker<borrowed_fd> get_seeker() const & noexcept {
      return file_seeker{__fd.borrow()};
    }
    file_syncer<borrowed_fd> get_syncer() const & noexcept {
      return file_syncer{__fd.borrow()};
    }
    file_reader<fd_t> get_reader() && noexcept
      requires(mode != open_mode::write_append && mode != open_mode::write_truncate)
    {
      return file_reader{std::move(__fd)};
    }
    file_writer<fd_t> get_writer() && noexcept
      requires(mode != open_mode::read)
    {
      return file_writer{std::move(__fd)};
    }
    file_seeker<fd_t> get_seeker() && noexcept {
      return file_seeker{std::move(__fd)};
    }
    file_syncer<fd_t> get_syncer() && noexcept {
      return file_syncer{std::move(__fd)};
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