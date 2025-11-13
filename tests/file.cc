#include <jowi/test_lib.hpp>
#include <filesystem>
#include <format>
#include <string>
import jowi.test_lib;
import jowi.io;

namespace test_lib = jowi::test_lib;
namespace io = jowi::io;
namespace fs = std::filesystem;

JOWI_SETUP(argc, argv) {
  test_lib::get_test_context().set_thread_count(1).set_time_unit(
    test_lib::TestTimeUnit::MILLI_SECONDS
  );
}

static auto tmp_write_path = fs::path{"/tmp/lolfile"};

JOWI_ADD_TEST(test_read_buf) {
  auto f = test_lib::assert_expected_value(io::OpenOptions{}.read().open(READ_FILE));
  auto buf = io::DynBuffer{2048};
  test_lib::assert_expected_value(f.read(buf));
  test_lib::assert_equal(buf.read(), "HELLO WORLD 0\nHELLO WORLD 1\nHELLO WORLD 2");
}

JOWI_ADD_TEST(test_write_file) {
  auto f = test_lib::assert_expected_value(
    io::OpenOptions{}.write().truncate().create().open(tmp_write_path)
  );
  auto buf = io::DynBuffer{2048};
  auto msg = test_lib::random_string(100);
  test_lib::assert_expected_value(f.write(msg));
  f = test_lib::assert_expected_value(io::OpenOptions{}.read().open(tmp_write_path));
  test_lib::assert_expected_value(f.read(buf));
  test_lib::assert_equal(buf.read(), msg);
}

JOWI_TEARDOWN() {
  if (fs::exists(tmp_write_path)) {
    fs::remove(tmp_write_path);
  }
}

// JOWI_ADD_TEST(test_read_buf) {
//   auto f = test_lib::assert_expected_value(io::OpenOptions{}.read().open(READ_FILE));
//   auto it = io::BufferIterator{io::DynBuffer{2048}, f};
//   test_lib::assert_equal(
//     test_lib::assert_expected_value(*it), "HELLO WORLD 0\nHELLO WORLD 1\nHELLO WORLD 2"
//   );
// }

// JOWI_ADD_TEST(test_read_line_by_line) {
//   auto f = test_lib::assert_expected_value(io::OpenOptions{}.read().open(READ_FILE));
//   uint32_t i = 0;
//   for (auto b : io::LineIterator{io::DynBuffer{2048}, f}) {
//     test_lib::assert_equal(
//       test_lib::assert_expected_value(std::move(b)), std::format("HELLO WORLD {}", i)
//     );
//     i += 1;
//   }
//   test_lib::assert_equal(i, 3);
// }

// JOWI_ADD_TEST(test_read_lbl_small_buf) {
//   auto f = test_lib::assert_expected_value(io::OpenOptions{}.read().open(READ_FILE));
//   uint32_t i = 0;
//   for (auto b : io::LineIterator{io::DynBuffer{5}, f}) {
//     test_lib::assert_equal(
//       test_lib::assert_expected_value(std::move(b)), std::format("HELLO WORLD {}", i)
//     );
//     i += 1;
//   }
//   test_lib::assert_equal(i, 3);
// }

// JOWI_ADD_TEST(test_read) {
//   auto f = test_lib::assert_expected_value(io::OpenOptions{}.read().open(READ_FILE));
//   for (auto b : io::buf_iterator{f, io::DynBuffer{2048}}) {
//     test_lib::assert_expected_value(b.value(), "HELLO WORLD 0\nHELLO WORLD 1\nHELLO WORLD 2\n");
//   }
// }

// JOWI_ADD_TEST(test_read_small_buf) {
//   auto f = test_lib::assert_expected_value(io::OpenOptions{}.read().open(READ_FILE));
//   auto acc =
//     std::accumulate(io::ByteReader{f, io::DynBuffer{5}}, std::string{}, [](auto prev, auto cur)
//     {
//       prev.append(cur);
//       return prev;
//     });
//   test_lib::assert_equal(
//     test_lib::assert_expected_value(f.read()), "HELLO WORLD 0\nHELLO WORLD 1\nHELLO WORLD 2\n"
//   );
// }

// JOWI_ADD_TEST(test_read_n) {
//   auto f = io::ByteReader(
//     io::FixedBuffer<2048>{},
//     test_lib::assert_expected_value(io::OpenOptions{}.read().open(READ_FILE))
//   );
//   test_lib::assert_equal(test_lib::assert_expected_value(f.read_n(5)), "HELLO");
// }

// JOWI_ADD_TEST(test_read_until) {
//   auto f = io::ByteReader(
//     io::FixedBuffer<2048>{},
//     test_lib::assert_expected_value(io::OpenOptions{}.read().open(READ_FILE))
//   );
//   test_lib::assert_equal(test_lib::assert_expected_value(f.read_until('\n')), "HELLO WORLD 0");
//   test_lib::assert_equal(test_lib::assert_expected_value(f.read_until('\n')), "HELLO WORLD 1");
//   test_lib::assert_equal(test_lib::assert_expected_value(f.read_until('\n')), "HELLO WORLD 2");
// }

// JOWI_ADD_TEST(test_read_until_small_buf) {
//   auto f = io::ByteReader(
//     io::FixedBuffer<5>{},
//     test_lib::assert_expected_value(io::OpenOptions{}.read().open(READ_FILE))
//   );
//   test_lib::assert_equal(test_lib::assert_expected_value(f.read_until('\n')), "HELLO WORLD 0");
//   test_lib::assert_equal(test_lib::assert_expected_value(f.read_until('\n')), "HELLO WORLD 1");
//   test_lib::assert_equal(test_lib::assert_expected_value(f.read_until('\n')), "HELLO WORLD 2");
// }

// JOWI_ADD_TEST(test_read_line) {
//   auto f = io::LineReader(
//     io::FixedBuffer<2048>{},
//     test_lib::assert_expected_value(io::OpenOptions{}.read().open(READ_FILE))
//   );
//   auto lines = test_lib::assert_expected_value(f.read_lines());
//   test_lib::assert_equal(lines[0], "HELLO WORLD 0");
//   test_lib::assert_equal(lines[1], "HELLO WORLD 1");
//   test_lib::assert_equal(lines[2], "HELLO WORLD 2");
// }

// JOWI_ADD_TEST(test_read_line_small_buf) {
//   auto f = io::LineReader(
//     io::FixedBuffer<5>{},
//     test_lib::assert_expected_value(io::OpenOptions{}.read().open(READ_FILE))
//   );
//   auto lines = test_lib::assert_expected_value(f.read_lines());
//   test_lib::assert_equal(lines[0], "HELLO WORLD 0");
//   test_lib::assert_equal(lines[1], "HELLO WORLD 1");
//   test_lib::assert_equal(lines[2], "HELLO WORLD 2");
// }

// JOWI_ADD_TEST(csv_reader_test) {
//   auto f = io::CsvReader(
//     io::FixedBuffer<2048>{},
//     test_lib::assert_expected_value(io::OpenOptions{}.read().open(READ_CSV_FILE))
//   );
//   auto rows = test_lib::assert_expected_value(f.read_rows());
//   test_lib::assert_equal(rows.size(), 10);
//   for (size_t i = 0; i != rows.size(); i += 1) {
//     test_lib::assert_equal(rows[i].size(), (i / 2) + 1);
//     if (i % 2 == 0) {
//       test_lib::assert_true(std::ranges::find(rows[i], "WORLD") == rows[i].end());
//     } else {
//       test_lib::assert_true(std::ranges::find(rows[i], "HELLO") == rows[i].end());
//     }
//   }
// }
