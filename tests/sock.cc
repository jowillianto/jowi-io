#include <jowi/test_lib.hpp>
#include <filesystem>
#include <future>
#include <string>
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
    io::sock_options{conn_addr}.create().and_then(&io::ipv4_sock_free::connect)
  );
  io::fixed_buffer<100> recv_msg;
  test_lib::assert_expected_value(connection.read(recv_msg));
  test_lib::assert_equal(recv_msg.read_buf(), msg);
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
    io::sock_options{conn_addr}.create().and_then(&io::ipv4_sock_free::connect)
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
    io::sock_options{conn_addr}.create().and_then(&io::ipv4_sock_free::connect)
  );
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  test_lib::assert_true(test_lib::assert_expected_value(connection.is_readable()));
  fut.wait();
}

JOWI_ADD_TEST(test_create_local_listener) {
  auto tmp_dir = std::filesystem::temp_directory_path();
  auto path = (tmp_dir / ("jowi_local_sock_" + test_lib::random_string(16))).string();
  test_lib::assert_true(path.length() < 107);
  std::error_code ec;
  std::filesystem::remove(path, ec);
  auto addr = io::local_address::empty();
  addr.addr(generic::fixed_string<107>{path});
  auto sock =
    test_lib::assert_expected_value(io::sock_options{addr}.create().and_then([](auto &&sock) {
      return std::move(sock).bind_listen(50);
    }));
  (void)sock;
  std::filesystem::remove(path, ec);
}

JOWI_ADD_TEST(test_local_connect_to_listener) {
  auto tmp_dir = std::filesystem::temp_directory_path();
  auto path = (tmp_dir / ("jowi_local_sock_" + test_lib::random_string(16))).string();
  test_lib::assert_true(path.length() < 107);
  std::error_code ec;
  std::filesystem::remove(path, ec);

  auto listener_addr = io::local_address::empty();
  listener_addr.addr(generic::fixed_string<107>{path});
  auto listener =
    test_lib::assert_expected_value(io::sock_options{listener_addr}.create().and_then([](auto &&sock) {
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

  auto conn_addr = io::local_address::empty();
  conn_addr.addr(generic::fixed_string<107>{path});
  auto connection = test_lib::assert_expected_value(
    io::sock_options{conn_addr}.create().and_then(&io::local_sock_free::connect)
  );

  io::fixed_buffer<100> recv_msg;
  test_lib::assert_expected_value(connection.read(recv_msg));
  test_lib::assert_equal(recv_msg.read_buf(), msg);
  fut.wait();

  std::filesystem::remove(path, ec);
}

JOWI_ADD_TEST(test_local_connect_and_check_no_data) {
  auto tmp_dir = std::filesystem::temp_directory_path();
  auto path = (tmp_dir / ("jowi_local_sock_" + test_lib::random_string(16))).string();
  test_lib::assert_true(path.length() < 107);
  std::error_code ec;
  std::filesystem::remove(path, ec);

  auto listener_addr = io::local_address::empty();
  listener_addr.addr(generic::fixed_string<107>{path});
  auto listener =
    test_lib::assert_expected_value(io::sock_options{listener_addr}.create().and_then([](auto &&sock) {
      return std::move(sock).bind_listen(50);
    }));

  std::promise<void> release_promise;
  auto release_future = release_promise.get_future();
  auto fut = std::async(
    std::launch::async,
    [](auto listener, std::future<void> release) {
      auto conn = test_lib::assert_expected_value(listener.accept());
      release.wait();
      (void)conn;
    },
    std::move(listener),
    std::move(release_future)
  );

  auto conn_addr = io::local_address::empty();
  conn_addr.addr(generic::fixed_string<107>{path});
  auto connection = test_lib::assert_expected_value(
    io::sock_options{conn_addr}.create().and_then(&io::local_sock_free::connect)
  );

  test_lib::assert_false(test_lib::assert_expected_value(connection.is_readable()));
  release_promise.set_value();
  fut.wait();

  std::filesystem::remove(path, ec);
}

JOWI_ADD_TEST(test_local_connect_and_check_have_data) {
  auto tmp_dir = std::filesystem::temp_directory_path();
  auto path = (tmp_dir / ("jowi_local_sock_" + test_lib::random_string(16))).string();
  test_lib::assert_true(path.length() < 107);
  std::error_code ec;
  std::filesystem::remove(path, ec);

  auto listener_addr = io::local_address::empty();
  listener_addr.addr(generic::fixed_string<107>{path});
  auto listener =
    test_lib::assert_expected_value(io::sock_options{listener_addr}.create().and_then([](auto &&sock) {
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

  auto conn_addr = io::local_address::empty();
  conn_addr.addr(generic::fixed_string<107>{path});
  auto connection = test_lib::assert_expected_value(
    io::sock_options{conn_addr}.create().and_then(&io::local_sock_free::connect)
  );

  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  test_lib::assert_true(test_lib::assert_expected_value(connection.is_readable()));
  fut.wait();

  std::filesystem::remove(path, ec);
}
