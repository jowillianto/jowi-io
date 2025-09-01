import moderna.test_lib;
import moderna.io;
#include <moderna/test_lib.hpp>
#include <algorithm>
#include <utility>

namespace test_lib = moderna::test_lib;
namespace io = moderna::io;

MODERNA_SETUP(argc, argv) {
  test_lib::get_test_context().set_thread_count(1).set_time_unit(
    test_lib::test_time_unit::MILLI_SECONDS
  );
}

MODERNA_ADD_TEST(test_read) {
  auto f = io::make_byte_reader<2048>(
    test_lib::assert_expected_value(io::open_options{}.read().open(READ_FILE))
  );
  test_lib::assert_equal(
    test_lib::assert_expected_value(f.read()), "HELLO WORLD 0\nHELLO WORLD 1\nHELLO WORLD 2\n"
  );
}

MODERNA_ADD_TEST(test_read_small_buf) {
  auto f = io::make_byte_reader<5>(
    test_lib::assert_expected_value(io::open_options{}.read().open(READ_FILE))
  );
  test_lib::assert_equal(
    test_lib::assert_expected_value(f.read()), "HELLO WORLD 0\nHELLO WORLD 1\nHELLO WORLD 2\n"
  );
}

MODERNA_ADD_TEST(test_read_n) {
  auto f = io::make_byte_reader<2048>(
    test_lib::assert_expected_value(io::open_options{}.read().open(READ_FILE))
  );
  test_lib::assert_equal(test_lib::assert_expected_value(f.read_n(5)), "HELLO");
}

MODERNA_ADD_TEST(test_read_until) {
  auto f = io::make_byte_reader<2048>(
    test_lib::assert_expected_value(io::open_options{}.read().open(READ_FILE))
  );
  test_lib::assert_equal(test_lib::assert_expected_value(f.read_until('\n')), "HELLO WORLD 0");
  test_lib::assert_equal(test_lib::assert_expected_value(f.read_until('\n')), "HELLO WORLD 1");
  test_lib::assert_equal(test_lib::assert_expected_value(f.read_until('\n')), "HELLO WORLD 2");
}

MODERNA_ADD_TEST(test_read_until_small_buf) {
  auto f = io::make_byte_reader<5>(
    test_lib::assert_expected_value(io::open_options{}.read().open(READ_FILE))
  );
  test_lib::assert_equal(test_lib::assert_expected_value(f.read_until('\n')), "HELLO WORLD 0");
  test_lib::assert_equal(test_lib::assert_expected_value(f.read_until('\n')), "HELLO WORLD 1");
  test_lib::assert_equal(test_lib::assert_expected_value(f.read_until('\n')), "HELLO WORLD 2");
}

MODERNA_ADD_TEST(test_read_line) {
  auto f = io::make_line_reader<2048>(
    test_lib::assert_expected_value(io::open_options{}.read().open(READ_FILE))
  );
  auto lines = test_lib::assert_expected_value(f.read_lines());
  test_lib::assert_equal(lines[0], "HELLO WORLD 0");
  test_lib::assert_equal(lines[1], "HELLO WORLD 1");
  test_lib::assert_equal(lines[2], "HELLO WORLD 2");
}

MODERNA_ADD_TEST(test_read_line_small_buf) {
  auto f = io::make_line_reader<5>(
    test_lib::assert_expected_value(io::open_options{}.read().open(READ_FILE))
  );
  auto lines = test_lib::assert_expected_value(f.read_lines());
  test_lib::assert_equal(lines[0], "HELLO WORLD 0");
  test_lib::assert_equal(lines[1], "HELLO WORLD 1");
  test_lib::assert_equal(lines[2], "HELLO WORLD 2");
}

MODERNA_ADD_TEST(csv_reader_test) {
  auto f = io::make_csv_reader<2048>(
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

// MODERNA_ADD_TEST(read_test) {
//   auto open_res = io::open_file<io::open_mode::read>(READ_FILE);
//   test_lib::assert_expected(open_res);
//   auto file = std::move(open_res.value());
//   auto r_value = file.read();
//   test_lib::assert_expected(r_value);
//   test_lib::assert_equal(r_value.value(), "HELLO WORLD 0\nHELLO WORLD 1\nHELLO WORLD 2\n");
// }

// MODERNA_ADD_TEST(read_n_test) {
//   auto open_res = io::open_file<io::open_mode::read>(READ_FILE);
//   test_lib::assert_expected(open_res);
//   auto file = std::move(open_res.value());
//   auto r_value = file.read(11);
//   test_lib::assert_expected(r_value);
//   test_lib::assert_equal(r_value.value(), "HELLO WORLD");
// }

// MODERNA_ADD_TEST(read_seek_n_test) {
//   auto open_res = io::open_file<io::open_mode::read>(READ_FILE);
//   test_lib::assert_expected(open_res);
//   auto file = std::move(open_res.value());
//   auto res = file.seek(14);
//   auto r_value = file.read(11);
//   test_lib::assert_expected(r_value);
//   test_lib::assert_equal(r_value.value(), "HELLO WORLD");
// }

// MODERNA_ADD_TEST(write_test) {
//   auto open_res = io::open_file<io::open_mode::write_truncate>(WRITE_FILE);
//   test_lib::assert_expected(open_res);
//   auto file = std::move(open_res.value());
//   auto rd = test_lib::random_string(100);
//   auto res = file.write(rd);
//   test_lib::assert_expected(res);
//   auto r_opener = io::open_file<io::open_mode::read>(WRITE_FILE);
//   test_lib::assert_expected(r_opener);
//   auto r_file = std::move(r_opener.value());
//   auto r_value = r_file.read();
//   test_lib::assert_expected(r_value);
// }

// MODERNA_ADD_TEST(write_seek_test) {
//   auto open_res = io::open_file<io::open_mode::write_truncate>(WRITE_FILE);
//   test_lib::assert_expected(open_res);
//   auto file = std::move(open_res.value());
//   auto rd = test_lib::random_string(100);
//   auto res = file.write(rd);
//   test_lib::assert_expected(res);
//   auto res_seek = file.seek(0);
//   test_lib::assert_expected(res_seek);
//   res = file.write("HELLO WORLD");
//   test_lib::assert_expected(res);
//   rd.replace(0, 11, "HELLO WORLD");
//   auto r_opener = io::open_file<io::open_mode::read>(WRITE_FILE);
//   test_lib::assert_expected(r_opener);
//   auto r_file = std::move(r_opener.value());
//   auto r_value = r_file.read();
//   test_lib::assert_expected(r_value);
//   test_lib::assert_equal(r_value.value(), rd);
// }

// MODERNA_ADD_TEST(open_file_fail) {
//   auto open_res = io::open_file<io::open_mode::read>("/non/existent/file");
//   test_lib::assert_false(open_res.has_value(), "Non existent file should not be openable");
// }

// MODERNA_ADD_TEST(open_file_write_delete_file) {
//   auto open_res = io::open_file<io::open_mode::write_truncate>("/tmp/lol");
//   test_lib::assert_expected(open_res);
// }

// MODERNA_ADD_TEST(csv_reader_test) {
//   auto res = io::open_file<io::open_mode::read>{READ_CSV_FILE};
//   test_lib::assert_expected(res);
//   auto file = std::move(res.value());
//   auto lines_res = file.read(io::csv_reader{});
//   test_lib::assert_expected(lines_res);
//   auto lines = std::move(lines_res.value());
//   test_lib::assert_equal(lines.size(), 10);
//   for (size_t i = 0; i != lines.size(); i += 1) {
//     test_lib::assert_equal(lines[i].size(), (i / 2) + 1);
//     if (i % 2 == 0) {
//       test_lib::assert_true(std::ranges::find(lines[i], "WORLD") == lines[i].end());
//     } else {
//       test_lib::assert_true(std::ranges::find(lines[i], "HELLO") == lines[i].end());
//     }
//   }
// }

// MODERNA_ADD_TEST(csv_writer_test) {
//   std::vector<std::vector<std::string>> write_content{
//     {test_lib::random_string(11)},
//     {test_lib::random_string(11), test_lib::random_string(11)},
//     {test_lib::random_string(11), test_lib::random_string(11), test_lib::random_string(11)},
//     {test_lib::random_string(11),
//      test_lib::random_string(11),
//      test_lib::random_string(11),
//      test_lib::random_string(11)},
//     {test_lib::random_string(11),
//      test_lib::random_string(11),
//      test_lib::random_string(11),
//      test_lib::random_string(11),
//      test_lib::random_string(11)}
//   };
//   auto res = io::open_file<io::open_mode::write_truncate>{WRITE_CSV_FILE};
//   test_lib::assert_expected(res);
//   auto file = std::move(res.value());
//   auto w_res = file.write(write_content, io::csv_writer{});
//   test_lib::assert_expected(w_res);

//   auto r_f_res = io::open_file<io::open_mode::read>{WRITE_CSV_FILE};
//   test_lib::assert_expected(r_f_res);
//   auto r_file = std::move(r_f_res.value());
//   auto r_res = r_file.read(io::csv_reader{});
//   test_lib::assert_expected(r_res);
//   auto r_cont = std::move(r_res.value());
//   for (size_t i = 0; i < write_content.size(); i += 1) {
//     for (size_t j = 0; j < write_content[i].size(); j += 1) {
//       auto &entry = r_cont.at(i).at(j);
//       test_lib::assert_equal(entry, write_content[i][j]);
//     }
//   }
// }