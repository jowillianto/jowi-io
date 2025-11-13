import jowi.test_lib;
import jowi.io;
import jowi.generic;

namespace test_lib = jowi::test_lib;
namespace io = jowi::io;
namespace generic = jowi::generic;

#include <jowi/test_lib.hpp>
#include <array>
#include <expected>
#include <ranges>
#include <string>

JOWI_SETUP(argc, argv) {
  test_lib::get_test_context().set_thread_count(1).set_time_unit(
    test_lib::TestTimeUnit::MILLI_SECONDS
  );
}

// /**
//   Test lists
// */
// std::array valid_headers = {
//   std::tuple{
//     generic::fixed_string<50>{"Date: Mon, 27 Jul 2009 12:28:53 GMT"},
//     generic::fixed_string<20>{"Date"},
//     generic::fixed_string<30>{"Mon, 27 Jul 2009 12:28:53 GMT"}
//   },
//   std::tuple{
//     generic::fixed_string<50>{"Server: Apache"},
//     generic::fixed_string<20>{"Server"},
//     generic::fixed_string<30>{"Apache"}
//   },
//   std::tuple{
//     generic::fixed_string<50>{"Last-Modified: Wed, 22 Jul 2009 19:15:56 GMT"},
//     generic::fixed_string<20>{"Last-Modified"},
//     generic::fixed_string<30>{"Wed, 22 Jul 2009 19:15:56 GMT"}
//   },
//   std::tuple{
//     generic::fixed_string<50>{"ETag: \"34aa387-d-1568eb00\""},
//     generic::fixed_string<20>{"ETag"},
//     generic::fixed_string<30>{"\"34aa387-d-1568eb00\""}
//   },
//   std::tuple{
//     generic::fixed_string<50>{"Accept-Ranges: bytes"},
//     generic::fixed_string<20>{"Accept-Ranges"},
//     generic::fixed_string<30>{"bytes"}
//   },
//   std::tuple{
//     generic::fixed_string<50>{"Content-Length: 51"},
//     generic::fixed_string<20>{"Content-Length"},
//     generic::fixed_string<30>{"51"}
//   },
//   std::tuple{
//     generic::fixed_string<50>{"Vary: Accept-Encoding"},
//     generic::fixed_string<20>{"Vary"},
//     generic::fixed_string<30>{"Accept-Encoding"}
//   },
//   std::tuple{
//     generic::fixed_string<50>{"Content-Type: text/plain"},
//     generic::fixed_string<20>{"Content-Type"},
//     generic::fixed_string<30>{"text/plain"}
//   }
// };

// generic::fixed_string valid_http_header{
//   "GET /api/users HTTP/1.1\r\n"
//   "Host: example.com\r\n"
//   "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36\r\n"
//   "Accept: application/json\r\n"
//   "Accept-Language: en-US,en;q=0.9\r\n"
//   "Accept-Encoding: gzip, deflate\r\n"
//   "Connection: keep-alive\r\n"
//   "\r\n"
// };

// JOWI_ADD_TEST(test_valid_http_combo) {
//   for (const auto &h : valid_headers) {
//     auto [name, value] =
//       test_lib::assert_expected_value(io::http::HttpHeader::validate_header_line(std::get<0>(h)));
//     test_lib::assert_equal(name, std::get<1>(h));
//     test_lib::assert_equal(value, std::get<2>(h));
//   }
// }

// JOWI_ADD_TEST(test_http_header_add_header_with_line) {
//   io::http::HttpHeader header{};
//   for (const auto &h : valid_headers) {
//     test_lib::assert_expected_value(header.add_header(std::get<0>(h)));
//   }
//   test_lib::assert_equal(header.size(), valid_headers.size());
// }

// JOWI_ADD_TEST(test_http_header_add_header_with_name_value) {
//   io::http::HttpHeader header{};
//   for (const auto &h : valid_headers) {
//     test_lib::assert_expected_value(header.add_header(std::get<1>(h), std::get<2>(h)));
//   }
//   test_lib::assert_equal(header.size(), valid_headers.size());
// }

// JOWI_ADD_TEST(test_http_header_first_of) {
//   io::http::HttpHeader header{};
//   for (const auto &h : valid_headers) {
//     test_lib::assert_expected_value(header.add_header(std::get<0>(h)));
//   }
//   test_lib::assert_equal(header.size(), valid_headers.size());
//   for (const auto &h : valid_headers) {
//     test_lib::assert_equal(header.first_of(std::get<1>(h)).value(), std::get<2>(h));
//   }
// }

// JOWI_ADD_TEST(test_http_header_filter) {
//   io::http::HttpHeader header{};
//   for (const auto &h : valid_headers) {
//     test_lib::assert_expected_value(header.add_header(std::get<0>(h)));
//   }
//   test_lib::assert_equal(header.size(), valid_headers.size());
//   for (const auto &h : valid_headers) {
//     auto vs = header.filter(std::get<1>(h));
//     test_lib::assert_false(vs.empty());
//     test_lib::assert_equal(vs.front(), std::get<2>(h));
//   }
// }

// JOWI_ADD_TEST(test_http_header_multiline) {
//   std::string header_str;
//   for (const auto &h : valid_headers) {
//     header_str.append_range(std::get<0>(h));
//     header_str.append_range("\r\n");
//   }
//   auto header =
//     test_lib::assert_expected_value(io::http::HttpHeader::validate_header_lines(header_str));
//   test_lib::assert_equal(header.size(), valid_headers.size());
//   for (const auto &h : valid_headers) {
//     auto vs = header.filter(std::get<1>(h));
//     test_lib::assert_false(vs.empty());
//     test_lib::assert_equal(vs.front(), std::get<2>(h));
//   }
// }

// JOWI_ADD_TEST(test_http_config) {
//   auto reader = io::http::ByteReader(
//     io::FixedBuffer<2048>{},
//     io::InMemFile{std::string{valid_http_header.begin(), valid_http_header.end()}}
//   );
//   auto config = test_lib::assert_expected_value(reader.read_config());
//   test_lib::assert_equal(config.method, "GET");
//   test_lib::assert_equal(config.path, "/api/users");
//   test_lib::assert_equal(config.headers.first_of("Host"), "example.com");
//   test_lib::assert_equal(
//     config.headers.first_of("User-Agent"),
//     "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"
//   );
//   test_lib::assert_equal(config.headers.first_of("Accept"), "application/json");
//   test_lib::assert_equal(config.headers.first_of("Accept-Language"), "en-US,en;q=0.9");
//   test_lib::assert_equal(config.headers.first_of("Accept-Encoding"), "gzip, deflate");
//   test_lib::assert_equal(config.headers.first_of("Connection"), "keep-alive");
// }
