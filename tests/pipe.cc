import jowi.test_lib;
import jowi.io;
import jowi.generic;

namespace test_lib = jowi::test_lib;
namespace io = jowi::io;
namespace generic = jowi::generic;

#include <jowi/test_lib.hpp>
#include <utility>

JOWI_SETUP(argc, argv) {
  test_lib::get_test_context().set_thread_count(1).set_time_unit(
    test_lib::test_time_unit::MILLI_SECONDS
  );
}

JOWI_ADD_TEST(test_pipe_create) {
  auto [r, w] = test_lib::assert_expected_value(io::open_pipe());
}

JOWI_ADD_TEST(test_pipe_simple_rw) {
  auto [r, w] = test_lib::assert_expected_value(io::open_pipe());
  auto msg = test_lib::random_string(100);
  test_lib::assert_expected(w.write(msg));
  auto buf = io::fixed_buffer<100>{};
  test_lib::assert_expected(r.read(buf));
  test_lib::assert_equal(buf.read_buf(), msg);
}

JOWI_ADD_TEST(test_pipe_check_data_noexist) {
  auto [r, w] = test_lib::assert_expected_value(io::open_pipe());
  test_lib::assert_false(test_lib::assert_expected_value(r.is_readable()));
}

JOWI_ADD_TEST(test_pipe_check_data_exist) {
  auto [r, w] = test_lib::assert_expected_value(io::open_pipe());
  auto msg = test_lib::random_string(100);
  test_lib::assert_expected(w.write(msg));
  test_lib::assert_true(test_lib::assert_expected_value(r.is_readable()));
}