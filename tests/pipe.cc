import moderna.test_lib;
import moderna.io;

namespace test_lib = moderna::test_lib;
namespace io = moderna::io;

#include <moderna/test_lib.hpp>
#include <utility>

MODERNA_SETUP(argc, argv) {
  test_lib::get_test_context().set_thread_count(1).set_time_unit(
    test_lib::test_time_unit::MILLI_SECONDS
  );
}

MODERNA_ADD_TEST(pipe_create) {
  auto pipe = io::open_pipe();
  test_lib::assert_expected(pipe);
}

MODERNA_ADD_TEST(pipe_simple_rw) {
  auto pipe_res = io::open_pipe();
  test_lib::assert_expected(pipe_res);
  auto pipe = std::move(pipe_res.value());
  auto msg = test_lib::random_string(100);
  auto write_res = pipe.writer.write(msg);
  test_lib::assert_expected(write_res);
  auto read_res = pipe.reader.read();
  test_lib::assert_expected(read_res);
  test_lib::assert_equal(read_res.value(), msg);
}

MODERNA_ADD_TEST(pipe_check_data_noexist) {
  auto pipe_res = io::open_pipe();
  test_lib::assert_expected(pipe_res);
  auto pipe = std::move(pipe_res.value());
  auto is_readable_res = pipe.reader.is_read_ready();
  test_lib::assert_expected(is_readable_res);
  test_lib::assert_equal(is_readable_res.value(), false);
}

MODERNA_ADD_TEST(pipe_check_data_exist) {
  auto pipe_res = io::open_pipe();
  test_lib::assert_expected(pipe_res);
  auto pipe = std::move(pipe_res.value());
  auto msg = test_lib::random_string(100);
  auto write_res = pipe.writer.write(msg);
  test_lib::assert_expected(write_res);
  auto is_readable_res = pipe.reader.is_read_ready();
  test_lib::assert_expected(is_readable_res);
  test_lib::assert_equal(is_readable_res.value(), true);
}