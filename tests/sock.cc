#include <jowi/test_lib.hpp>
#include <coroutine>
#include <filesystem>
#include <format>
#include <future>
#include <string>
#include <string_view>
#include <thread>
import jowi.test_lib;
import jowi.io;
import jowi.generic;
import jowi.asio;

namespace test_lib = jowi::test_lib;
namespace fs = std::filesystem;
namespace io = jowi::io;

int get_random_port() {
  return test_lib::random_integer(20000, 70000);
}

JOWI_SETUP(argc, argv) {
  test_lib::get_test_context().set_thread_count(1).set_time_unit(
    test_lib::TestTimeUnit::MILLI_SECONDS
  );
}

static std::vector<fs::path> local_sockets;

const fs::path &issue_socket() {
  auto p = fs::path{std::format("/tmp/sock{}", local_sockets.size())};
  return local_sockets.emplace_back(p);
}

JOWI_ADD_TEST(test_ipv4_tcp) {
  int port = test_lib::random_integer(20'000, 30'0000);
  auto server_conf = io::Ipv4Address::listen_all(port);
  auto server_addr = test_lib::assert_expected_value(io::Ipv4Address::create("127.0.0.1", port));
  auto server = test_lib::assert_expected_value(io::create_tcp_listener(server_conf, 50));
  auto msg = test_lib::random_string(100);
  auto fut = std::async(
    std::launch::async,
    [](auto server, auto msg) {
      auto accept_res = server.accept();
      while (!accept_res) {
        accept_res = server.accept();
      }
      auto sock = test_lib::assert_expected_value(std::move(accept_res).value());
      test_lib::assert_expected_value(sock.send(msg, false));
    },
    std::move(server),
    std::string_view{msg}
  );
  auto client = test_lib::assert_expected_value(io::tcp_connect(server_addr));
  auto buf = io::DynBuffer{2048};
  test_lib::assert_expected_value(client.recv(buf, false));
  test_lib::assert_equal(buf.read(), msg);
}

JOWI_ADD_TEST(test_local_tcp) {
  auto server_conf = io::LocalAddress::with_address(issue_socket().c_str());
  auto server_addr = server_conf;
  auto server = test_lib::assert_expected_value(io::create_tcp_listener(server_conf, 50));
  auto msg = test_lib::random_string(100);
  auto fut = std::async(
    std::launch::async,
    [](auto server, auto msg) {
      auto accept_res = server.accept();
      while (!accept_res) {
        accept_res = server.accept();
      }
      auto sock = test_lib::assert_expected_value(std::move(accept_res).value());
      test_lib::assert_expected_value(sock.send(msg, false));
    },
    std::move(server),
    std::string_view{msg}
  );
  auto client = test_lib::assert_expected_value(io::tcp_connect(server_addr));
  auto buf = io::DynBuffer{2048};
  test_lib::assert_expected_value(client.recv(buf, false));
  test_lib::assert_equal(buf.read(), msg);
}

JOWI_ADD_TEST(test_ipv4_udp) {
  int port = test_lib::random_integer(20'000, 30'0000);
  auto server_conf = io::Ipv4Address::listen_all(port);
  auto server_addr = test_lib::assert_expected_value(io::Ipv4Address::create("127.0.0.1", port));
  auto msg = test_lib::random_string(100);
  auto buf = io::DynBuffer{2048};
  auto server = test_lib::assert_expected_value(io::create_udp_bind(server_conf));
  auto client =
    test_lib::assert_expected_value(io::create_udp_socket<std::decay_t<decltype(server_conf)>>());
  test_lib::assert_expected_value(client.send(msg, server_addr, false));
  test_lib::assert_expected_value(server.recv(buf, false));
  test_lib::assert_equal(buf.read(), msg);
}

JOWI_TEARDOWN() {
  for (auto sock : local_sockets) {
    fs::remove(sock);
  }
}