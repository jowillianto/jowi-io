#include <jowi/test_lib.hpp>
#include <future>
#include <thread>
import jowi.test_lib;
import jowi.io;
import jowi.generic;

namespace test_lib = jowi::test_lib;
namespace io = jowi::io;
namespace generic = jowi::generic;

int get_random_port() {
  return test_lib::random_integer(20000, 70000);
}

JOWI_SETUP(argc, argv) {
  test_lib::get_test_context().set_thread_count(1).set_time_unit(
    test_lib::test_time_unit::MILLI_SECONDS
  );
}

JOWI_ADD_TEST(test_create_listener) {
  int port = get_random_port();
  auto addr = io::ipv4_address::empty().port(port).all_interface();
  auto sock =
    test_lib::assert_expected_value(io::sock_options{addr}.create().and_then([](auto &&sock) {
      return std::move(sock).bind_listen(50);
    }));
}

JOWI_ADD_TEST(test_connect_to_listener) {
  int port = get_random_port();
  auto addr = io::ipv4_address::empty().port(port).all_interface();
  auto listener =
    test_lib::assert_expected_value(io::sock_options{addr}.create().and_then([](auto &&sock) {
      return std::move(sock).bind_listen(50);
    }));
  auto msg = test_lib::random_string(100);
  auto fut = std::async(
    std::launch::async,
    [](auto listener, auto msg) {
      auto conn = test_lib::assert_expected_value(listener.accept());
      test_lib::assert_expected_value(conn.write(msg));
    },
    std::move(listener),
    std::string{msg}
  );
  auto conn_addr = io::ipv4_address::empty().port(port);
  test_lib::assert_expected(conn_addr.addr("127.0.0.1"));
  auto connection = test_lib::assert_expected_value(
    io::sock_options{conn_addr}.create().and_then(&io::sock_free<io::ipv4_address>::connect)
  );
  generic::fixed_string<100> recv_msg;
  test_lib::assert_expected_value(connection.read<100>(recv_msg));
  test_lib::assert_equal(recv_msg, msg);
  fut.wait();
}

JOWI_ADD_TEST(test_connect_and_check_no_data) {
  int port = get_random_port();
  auto addr = io::ipv4_address::empty().port(port).all_interface();
  auto listener =
    test_lib::assert_expected_value(io::sock_options{addr}.create().and_then([](auto &&sock) {
      return std::move(sock).bind_listen(50);
    }));
  auto fut = std::async(
    std::launch::async,
    [](auto listener) { test_lib::assert_expected_value(listener.accept()); },
    std::move(listener)
  );
  auto conn_addr = io::ipv4_address::empty().port(port);
  test_lib::assert_expected(conn_addr.addr("127.0.0.1"));
  auto connection = test_lib::assert_expected_value(
    io::sock_options{conn_addr}.create().and_then(&io::sock_free<io::ipv4_address>::connect)
  );
  test_lib::assert_false(test_lib::assert_expected_value(connection.is_readable()));
  fut.wait();
}

JOWI_ADD_TEST(test_connect_and_check_have_data) {
  int port = get_random_port();
  auto addr = io::ipv4_address::empty().port(port).all_interface();
  auto listener =
    test_lib::assert_expected_value(io::sock_options{addr}.create().and_then([](auto &&sock) {
      return std::move(sock).bind_listen(50);
    }));
  auto msg = test_lib::random_string(100);
  auto fut = std::async(
    std::launch::async,
    [](auto listener, auto msg) {
      auto conn = test_lib::assert_expected_value(listener.accept());
      test_lib::assert_expected_value(conn.write(msg));
    },
    std::move(listener),
    msg
  );
  auto conn_addr = io::ipv4_address::empty().port(port);
  test_lib::assert_expected(conn_addr.addr("127.0.0.1"));
  auto connection = test_lib::assert_expected_value(
    io::sock_options{conn_addr}.create().and_then(&io::sock_free<io::ipv4_address>::connect)
  );
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  test_lib::assert_true(test_lib::assert_expected_value(connection.is_readable()));
  fut.wait();
}