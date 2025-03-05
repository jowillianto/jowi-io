import moderna.test_lib;
import moderna.io;

namespace test_lib = moderna::test_lib;
namespace io = moderna::io;

#include <moderna/test_lib.hpp>
#include <filesystem>
#include <utility>

MODERNA_SETUP(argc, argv) {
  test_lib::set_thread_count(1);
}

MODERNA_ADD_TEST(read_test) {
  auto opener = io::file_opener<io::open_mode::read>{READ_FILE}.open();
  test_lib::assert_expected(opener);
  auto file = std::move(opener.value());
  auto r_value = file.get_reader().read();
  test_lib::assert_expected(r_value);
  test_lib::assert_equal(r_value.value(), "HELLO WORLD 0\nHELLO WORLD 1\nHELLO WORLD 2\n");
}

MODERNA_ADD_TEST(read_n_test) {
  auto opener = io::file_opener<io::open_mode::read>{READ_FILE}.open();
  test_lib::assert_expected(opener);
  auto file = std::move(opener.value());
  auto r_value = file.get_reader().read(11);
  test_lib::assert_expected(r_value);
  test_lib::assert_equal(r_value.value(), "HELLO WORLD");
}

MODERNA_ADD_TEST(read_seek_n_test) {
  auto opener = io::file_opener<io::open_mode::read>{READ_FILE}.open();
  test_lib::assert_expected(opener);
  auto file = std::move(opener.value());
  auto res = file.get_seeker().seek(14);
  auto r_value = file.get_reader().read(11);
  test_lib::assert_expected(r_value);
  test_lib::assert_equal(r_value.value(), "HELLO WORLD");
}

MODERNA_ADD_TEST(write_test) {
  auto opener = io::file_opener<io::open_mode::write_truncate>{WRITE_FILE}.open();
  test_lib::assert_expected(opener);
  auto file = std::move(opener.value());
  auto rd = test_lib::random_string(100);
  auto res = file.get_writer().write(rd);
  test_lib::assert_expected(res);
  auto r_opener = io::file_opener<io::open_mode::read>{WRITE_FILE}.open();
  test_lib::assert_expected(r_opener);
  auto r_file = std::move(r_opener.value());
  auto r_value = r_file.get_reader().read();
  test_lib::assert_expected(r_value);
}

MODERNA_ADD_TEST(write_seek_test) {
  auto opener = io::file_opener<io::open_mode::write_truncate>{WRITE_FILE}.open();
  test_lib::assert_expected(opener);
  auto file = std::move(opener.value());
  auto rd = test_lib::random_string(100);
  auto writer = file.get_writer();
  auto seeker = file.get_seeker();
  auto res = writer.write(rd);
  test_lib::assert_expected(res);
  auto res_seek = seeker.seek(0);
  test_lib::assert_expected(res_seek);
  res = writer.write("HELLO WORLD");
  test_lib::assert_expected(res);
  rd.replace(0, 11, "HELLO WORLD");
  auto r_opener = io::file_opener<io::open_mode::read>{WRITE_FILE}.open();
  test_lib::assert_expected(r_opener);
  auto r_file = std::move(r_opener.value());
  auto r_value = r_file.get_reader().read();
  test_lib::assert_expected(r_value);
  test_lib::assert_equal(r_value.value(), rd);
}

MODERNA_ADD_TEST(open_file_fail) {
  auto opener = io::file_opener<io::open_mode::read>{"/non/existent/file"}.open();
  test_lib::assert_false(opener.has_value(), "Non existent file should not be openable");
}

MODERNA_ADD_TEST(open_file_write_delete_file) {
  auto path = std::filesystem::path{"/tmp"} / "lol";
  auto opener = io::file_opener<io::open_mode::write_truncate>{path};
  auto res_file = opener.open();
  test_lib::assert_expected(res_file);
}