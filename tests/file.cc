#include <jowi/test_lib.hpp>
#include <algorithm>
#include <string>
import jowi.test_lib;
import jowi.io;

namespace test_lib = jowi::test_lib;
namespace io = jowi::io;

JOWI_SETUP(argc, argv) {
  test_lib::get_test_context().set_thread_count(1).set_time_unit(
    test_lib::test_time_unit::MILLI_SECONDS
  );
}

JOWI_ADD_TEST(test_read) {
  auto f = io::byte_reader(
    io::fixed_buffer<2048>{},
    test_lib::assert_expected_value(io::open_options{}.read().open(READ_FILE))
  );
  test_lib::assert_equal(
    test_lib::assert_expected_value(f.read()), "HELLO WORLD 0\nHELLO WORLD 1\nHELLO WORLD 2\n"
  );
}

JOWI_ADD_TEST(test_read_small_buf) {
  auto f = io::byte_reader(
    io::fixed_buffer<5>{},
    test_lib::assert_expected_value(io::open_options{}.read().open(READ_FILE))
  );
  test_lib::assert_equal(
    test_lib::assert_expected_value(f.read()), "HELLO WORLD 0\nHELLO WORLD 1\nHELLO WORLD 2\n"
  );
}

JOWI_ADD_TEST(test_read_n) {
  auto f = io::byte_reader(
    io::fixed_buffer<2048>{},
    test_lib::assert_expected_value(io::open_options{}.read().open(READ_FILE))
  );
  test_lib::assert_equal(test_lib::assert_expected_value(f.read_n(5)), "HELLO");
}

JOWI_ADD_TEST(test_read_until) {
  auto f = io::byte_reader(
    io::fixed_buffer<2048>{},
    test_lib::assert_expected_value(io::open_options{}.read().open(READ_FILE))
  );
  test_lib::assert_equal(test_lib::assert_expected_value(f.read_until('\n')), "HELLO WORLD 0");
  test_lib::assert_equal(test_lib::assert_expected_value(f.read_until('\n')), "HELLO WORLD 1");
  test_lib::assert_equal(test_lib::assert_expected_value(f.read_until('\n')), "HELLO WORLD 2");
}

JOWI_ADD_TEST(test_read_until_small_buf) {
  auto f = io::byte_reader(
    io::fixed_buffer<5>{},
    test_lib::assert_expected_value(io::open_options{}.read().open(READ_FILE))
  );
  test_lib::assert_equal(test_lib::assert_expected_value(f.read_until('\n')), "HELLO WORLD 0");
  test_lib::assert_equal(test_lib::assert_expected_value(f.read_until('\n')), "HELLO WORLD 1");
  test_lib::assert_equal(test_lib::assert_expected_value(f.read_until('\n')), "HELLO WORLD 2");
}

JOWI_ADD_TEST(test_read_line) {
  auto f = io::line_reader(
    io::fixed_buffer<2048>{},
    test_lib::assert_expected_value(io::open_options{}.read().open(READ_FILE))
  );
  auto lines = test_lib::assert_expected_value(f.read_lines());
  test_lib::assert_equal(lines[0], "HELLO WORLD 0");
  test_lib::assert_equal(lines[1], "HELLO WORLD 1");
  test_lib::assert_equal(lines[2], "HELLO WORLD 2");
}

JOWI_ADD_TEST(test_read_line_small_buf) {
  auto f = io::line_reader(
    io::fixed_buffer<5>{},
    test_lib::assert_expected_value(io::open_options{}.read().open(READ_FILE))
  );
  auto lines = test_lib::assert_expected_value(f.read_lines());
  test_lib::assert_equal(lines[0], "HELLO WORLD 0");
  test_lib::assert_equal(lines[1], "HELLO WORLD 1");
  test_lib::assert_equal(lines[2], "HELLO WORLD 2");
}

JOWI_ADD_TEST(csv_reader_test) {
  auto f = io::csv_reader(
    io::fixed_buffer<2048>{},
    test_lib::assert_expected_value(io::open_options{}.read().open(READ_CSV_FILE))
  );
  auto rows = test_lib::assert_expected_value(f.read_rows());
  test_lib::assert_equal(rows.size(), 10);
  for (size_t i = 0; i != rows.size(); i += 1) {
    test_lib::assert_equal(rows[i].size(), (i / 2) + 1);
    if (i % 2 == 0) {
      test_lib::assert_true(std::ranges::find(rows[i], "WORLD") == rows[i].end());
    } else {
      test_lib::assert_true(std::ranges::find(rows[i], "HELLO") == rows[i].end());
    }
  }
}