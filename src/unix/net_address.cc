module;
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <algorithm>
#include <concepts>
#include <expected>
#include <string>
#include <string_view>
#include <utility>
export module jowi.io:net_address;
import :error;
import :sys_call;

namespace jowi::io {
  export template <class Addr>
  concept NetAddress = requires(Addr addr, const Addr caddr) {
    { caddr.sys_addr() } -> std::same_as<std::pair<const struct sockaddr *, socklen_t>>;
    { addr.sys_addr() } -> std::same_as<std::pair<struct sockaddr *, socklen_t>>;
    { Addr::addr_family() } -> std::same_as<unsigned char>;
    { Addr::empty() } -> std::same_as<Addr>;
  };

  export struct Ipv4Address {
  private:
    sockaddr_in __addr;

    Ipv4Address(sockaddr_in addr) noexcept : __addr{addr} {}

  public:
    int port() const noexcept {
      return __addr.sin_port;
    }
    std::string addr() const {
      return inet_ntoa(__addr.sin_addr);
    }

    std::pair<const struct sockaddr *, socklen_t> sys_addr() const noexcept {
      return std::pair{reinterpret_cast<const struct sockaddr *>(&__addr), sizeof(__addr)};
    }
    std::pair<struct sockaddr *, socklen_t> sys_addr() noexcept {
      return std::pair{reinterpret_cast<struct sockaddr *>(&__addr), sizeof(__addr)};
    }

    static unsigned char addr_family() noexcept {
      return AF_INET;
    }
    static Ipv4Address empty() noexcept {
      return Ipv4Address{{sizeof(__addr), static_cast<unsigned char>(addr_family()), 0, 0}};
    }
    static Ipv4Address listen_all(unsigned short port) noexcept {
      return Ipv4Address{
        {sizeof(__addr), static_cast<unsigned char>(addr_family()), port, INADDR_ANY}
      };
    }
    static std::expected<Ipv4Address, IoError> create(
      std::string_view host, unsigned short port
    ) noexcept {
      auto addr = listen_all(port);
      return sys_call_void(inet_aton, host.data(), &addr.__addr.sin_addr).transform([&]() {
        return addr;
      });
    }
  };

  export struct LocalAddress {
  private:
    sockaddr_un __addr;
    LocalAddress(sockaddr_un addr) noexcept : __addr{addr} {}

  public:
    std::string_view path() const noexcept {
      return __addr.sun_path;
    }
    std::pair<const struct sockaddr *, socklen_t> sys_addr() const {
      return std::pair{reinterpret_cast<const struct sockaddr *>(&__addr), sizeof(__addr)};
    }
    std::pair<struct sockaddr *, socklen_t> sys_addr() {
      return std::pair{reinterpret_cast<struct sockaddr *>(&__addr), sizeof(__addr)};
    }

    static unsigned char addr_family() {
      return AF_LOCAL;
    }

    static LocalAddress empty() {
      return LocalAddress{{addr_family(), 0}};
    }

    static LocalAddress with_address(std::string_view v) {
      auto addr = empty();
      auto copy_count = std::min(sizeof(__addr.sun_path) / sizeof(char) - 1, v.length());
      std::ranges::copy_n(v.begin(), copy_count, addr.__addr.sun_path);
      addr.__addr.sun_path[copy_count] = 0;
      return addr;
    }
  };
}