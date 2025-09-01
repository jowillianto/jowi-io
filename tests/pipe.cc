import moderna.test_lib;
import moderna.io;
import moderna.generic;

namespace test_lib = moderna::test_lib;
namespace io = moderna::io;
namespace generic = moderna::generic;

#include <moderna/test_lib.hpp>
#include <utility>

MODERNA_SETUP(argc, argv) {
  test_lib::get_test_context().set_thread_count(1).set_time_unit(
    test_lib::test_time_unit::MILLI_SECONDS
  );
}

MODERNA_ADD_TEST(pipe_create) {
  auto [r, w] = test_lib::assert_expected_value(io::open_pipe());
}

MODERNA_ADD_TEST(pipe_simple_rw) {
  auto [r, w] = test_lib::assert_expected_value(io::open_pipe());
  auto msg = test_lib::random_string(100);
  test_lib::assert_expected(w.write(msg));
  auto buf = generic::fixed_string<100>{};
  test_lib::assert_expected(r.read(buf));
  test_lib::assert_equal(buf, msg);
}

MODERNA_ADD_TEST(pipe_check_data_noexist) {
  auto [r, w] = test_lib::assert_expected_value(io::open_pipe());
  test_lib::assert_false(test_lib::assert_expected_value(r.is_readable()));
}

MODERNA_ADD_TEST(pipe_check_data_exist) {
  auto [r, w] = test_lib::assert_expected_value(io::open_pipe());
  auto msg = test_lib::random_string(100);
  test_lib::assert_expected(w.write(msg));
  test_lib::assert_true(test_lib::assert_expected_value(r.is_readable()));
}