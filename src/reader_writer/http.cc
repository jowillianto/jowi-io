module;
#include <algorithm>
#include <expected>
#include <format>
#include <ranges>
#include <regex>
#include <string>
#include <string_view>
#include <vector>
export module jowi.io:http;
import :readers;
import :buffer;
import :is_file;
import :error;

/**
 * @file reader_writer/http.cc
 * @brief HTTP parsing helpers built on top of the generic byte reader utilities.
 */

namespace jowi::io::http {
  /**
   * @brief Error categories encountered while parsing HTTP data.
   */
  export enum struct http_error_type { empty_string = 0, invalid_value, io_error };
  /**
   * @brief Lightweight error type carrying HTTP parsing metadata.
   */
  export struct http_error {
  private:
    http_error_type __t;
    generic::fixed_string<64> __msg;

  public:
    /**
     * @brief Builds a formatted HTTP error description.
     * @param t Error category.
     * @param fmt Format string describing the failure.
     * @param args Additional arguments formatted into the message.
     */
    template <class... Args> requires(std::formattable<Args, char> && ...)
    http_error(http_error_type t, std::format_string<Args...> fmt, Args &&...args) noexcept :
      __t{t}, __msg{} {
      __msg.emplace_format(fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief Provides the formatted error description.
     * @return Null-terminated string describing the error.
     */
    const char *what() const noexcept {
      return __msg.c_str();
    }

    /**
     * @brief Retrieves the error category.
     * @return Enumerated error type.
     */
    http_error_type type() const noexcept {
      return __t;
    }

    /**
     * @brief Converts an IO-layer error into an HTTP parsing error.
     * @param e Source IO error.
     * @return HTTP error containing the IO error message.
     */
    static http_error from_io_error(io_error e) {
      return http_error{http_error_type::io_error, "{}", e.what()};
    }
  };

  /**
   * @brief Represents an HTTP status code with optional textual description lookup.
   */
  export struct http_status {
  private:
    using status_map_entry = std::pair<unsigned int, generic::fixed_string<40>>;
    static std::vector<status_map_entry> __status_to_name;
    unsigned int __code;

  public:
    /**
     * @brief Stores the status code for later inspection.
     * @param code HTTP status code value.
     */
    http_status(unsigned int code) : __code{code} {}

    /**
     * @brief Returns the numeric status code.
     * @return Numeric HTTP status code.
     */
    unsigned int code() const noexcept {
      return __code;
    }

    /**
     * @brief Looks up the canonical status text.
     * @return Optional view containing the status text, or nullopt if unknown.
     */
    std::optional<std::string_view> name() const noexcept {
      return status_name(__code);
    }

    /**
     * @brief Resolves a textual status name for the supplied code.
     * @param code HTTP status code to translate.
     * @return Optional view containing the status text, or nullopt if unknown.
     */
    static std::optional<std::string_view> status_name(unsigned int code) noexcept {
      auto it = std::ranges::find(__status_to_name, code, &status_map_entry::first);
      if (it == __status_to_name.end()) {
        return std::nullopt;
      }
      return std::string_view{it->second};
    }
  };

  /**
   * @brief Container for HTTP header key-value pairs with validation helpers.
   */
  export struct http_header {
    using http_header_entry = std::pair<std::string, std::string>;

  private:
    constexpr static generic::fixed_string __valid_header_pattern{"^[a-zA-Z0-9_-]+$"};
    constexpr static generic::fixed_string __valid_value_pattern{"^.+$"};
    std::vector<http_header_entry> __headers;

  public:
    /**
     * @brief Constructs an empty header set.
     */
    http_header() : __headers{} {}

    /**
     * @brief Adds a header entry after validating name and value.
     * @param name Header key.
     * @param value Header value.
     * @return Success or validation error.
     */
    std::expected<void, http_error> add_header(std::string_view name, std::string_view value) {
      return validate_header_name(name).and_then([&]() {
        return validate_header_value(value).transform([&]() {
          __headers.emplace_back(std::string{name}, std::string{value});
        });
      });
    }

    /**
     * @brief Parses a raw header line and appends it to the container.
     * @param line Single `name: value` header line.
     * @return Success or validation error.
     */
    std::expected<void, http_error> add_header(std::string_view line) {
      return validate_header_line(line).transform([&](auto &&p) {
        __headers.emplace_back(std::move(p.first), std::move(p.second));
      });
    }

    /**
     * @brief Finds the first header value matching the supplied name.
     * @tparam T Comparable type for header names.
     * @param name Header name to search for.
     * @return Optional view containing the header value.
     */
    std::optional<std::string_view> first_of(
      const generic::is_comparable<std::string> auto &name
    ) const noexcept {
      auto it = std::ranges::find(__headers, name, &http_header_entry::first);
      if (it == __headers.end()) {
        return std::nullopt;
      }
      return std::string_view{it->second};
    }

    /**
     * @brief Returns a lazy view filtering values for the provided header name.
     * @tparam T Comparable type for header names.
     * @param name Header name to match.
     * @return Transform view yielding matching header values.
     */
    auto filter(generic::is_comparable<std::string> auto name) const noexcept {
      return std::ranges::transform_view{
        std::ranges::filter_view{
          __headers, [name = std::move(name)](const auto &p) { return name == p.first; }
        },
        &http_header_entry::second
      };
    }

    /**
     * @brief Returns the number of stored headers.
     * @return Count of header entries.
     */
    size_t size() const noexcept {
      return __headers.size();
    }
    /**
     * @brief Checks whether any headers are present.
     * @return True when the container is empty.
     */
    bool empty() const noexcept {
      return __headers.empty();
    }
    /**
     * @brief Returns an iterator to the first header.
     * @return Iterator to beginning of header collection.
     */
    auto begin() const noexcept {
      return __headers.begin();
    }
    /**
     * @brief Returns an iterator past the last header.
     * @return Iterator marking end of header collection.
     */
    auto end() const noexcept {
      return __headers.end();
    }
    /**
     * @brief Returns a const iterator to the first header.
     * @return Const iterator to beginning of header collection.
     */
    auto cbegin() const noexcept {
      return __headers.begin();
    }
    /**
     * @brief Returns a const iterator past the last header.
     * @return Const iterator marking end of header collection.
     */
    auto cend() const noexcept {
      return __headers.end();
    }

    /**
     * @brief Validates a header name against the allowed character set.
     * @param name Header name to validate.
     * @return Success or descriptive validation error.
     */
    static std::expected<void, http_error> validate_header_name(std::string_view name) noexcept {
      if (name.empty()) {
        return std::unexpected{
          http_error{http_error_type::empty_string, "header name cannot be empty"}
        };
      }
      auto reg_exp = std::regex{__valid_header_pattern.c_str()};
      if (!std::regex_match(name.begin(), name.end(), reg_exp)) {
        return std::unexpected{
          http_error{http_error_type::invalid_value, "{} fail '{}'", name, __valid_header_pattern}
        };
      }
      return {};
    }
    /**
     * @brief Validates a header value.
     * @param value Header value to validate.
     * @return Success or descriptive validation error.
     */
    static std::expected<void, http_error> validate_header_value(std::string_view value) noexcept {
      if (value.empty()) {
        return std::unexpected{
          http_error{http_error_type::empty_string, "header value cannot be empty"}
        };
      }
      auto reg_exp = std::regex{__valid_value_pattern.c_str()};
      if (!std::regex_match(value.begin(), value.end(), reg_exp)) {
        return std::unexpected{http_error{
          http_error_type::invalid_value, "{} fail regex '{}'", value, __valid_value_pattern
        }};
      }
      return {};
    }
    /**
     * @brief Parses and validates a single header line.
     * @param line Raw header line including separator.
     * @return Pair of header name and value, or validation error.
     */
    static std::expected<http_header_entry, http_error> validate_header_line(
      std::string_view line
    ) {
      auto colon_pos = std::ranges::find(line, ':');
      if (colon_pos == line.end()) {
        return std::unexpected{
          http_error{http_error_type::invalid_value, "':' expected in {}", line}
        };
      }
      auto header_value_beg =
        std::ranges::find_if_not(colon_pos + 1, line.end(), [](char c) { return isspace(c); });
      auto header_name = std::string_view{line.begin(), colon_pos};
      auto header_value = std::string_view{header_value_beg, line.end()};
      return validate_header_name(header_name).and_then([&]() {
        return validate_header_value(header_value).transform([&]() {
          return http_header_entry{std::string{header_name}, std::string{header_value}};
        });
      });
    }
    /**
     * @brief Parses CRLF separated header lines.
     * @param lines Block of header lines separated by `\r\n`.
     * @return Header container or validation error.
     */
    static std::expected<http_header, http_error> validate_header_lines(std::string_view lines) {
      auto header = http_header{};
      for (auto &&line : std::ranges::split_view{lines, "\r\n"}) {
        auto header_line = std::string_view{line.begin(), line.end()};
        if (header_line.empty()) continue;
        auto res = header.add_header(std::string_view{line.begin(), line.end()});
        if (!res) {
          return std::unexpected{res.error()};
        }
      }
      return header;
    }
  };

  export struct http_path {
  private:
    std::string __path;

  public:
    /**
     * @brief Wraps an HTTP path string for additional helpers.
     * @param path HTTP path string.
     */
    http_path(std::string path) : __path{std::move(path)} {}
  };

  /**
   * @brief Basic configuration produced by parsing the start line and headers.
   */
  export struct http_config {
    std::string method;
    std::string path;
    http_header headers;
  };

  export template <is_rw_buffer buffer_type, is_readable file_type>
  struct http_reader : private byte_reader<buffer_type, file_type> {
  protected:
    using byte_reader = byte_reader<buffer_type, file_type>;

  private:
    /**
     * @brief Recursively reads HTTP headers until a blank line is encountered.
     * @param header Header container to populate.
     * @return Success or HTTP parsing error.
     */
    std::expected<void, http_error> __read_header(http_header &header) {
      auto header_line = byte_reader::read_until('\n').transform_error(http_error::from_io_error);
      if (!header_line) {
        return std::unexpected{header_line.error()};
      }
      if (header_line->empty() || header_line->size() == 1) {
        return {};
      }
      if (!header_line->ends_with('\r')) {
        return std::unexpected{http_error{http_error_type::invalid_value, "\r is missing"}};
      }
      return header
        .add_header(
          std::string_view{header_line->begin(), header_line->begin() + header_line->size() - 1}
        )
        .and_then([&]() { return __read_header(header); });
    }

  public:
    /**
     * @brief Constructs the reader with a buffer and file-like source.
     * @param buf Buffer used for staged reads.
     * @param f Readable file or socket.
     */
    http_reader(buffer_type buf, file_type f) : byte_reader{std::move(buf), std::move(f)} {}

    /**
     * @brief Reads the request/response start line followed by headers.
     * @return Parsed HTTP configuration or error.
     */
    std::expected<http_config, http_error> read_config() {
      return byte_reader::read_until(' ')
        .and_then([&](auto &&method) {
          return byte_reader::read_until(' ').and_then([&](auto &&path) {
            return byte_reader::read_until('\n').transform([&](auto &&) {
              return http_config{std::move(method), std::move(path), http_header{}};
            });
          });
        })
        .transform_error(http_error::from_io_error)
        .and_then([&](auto &&conf) -> std::expected<http_config, http_error> {
          return __read_header(conf.headers).transform([&]() { return std::move(conf); });
        });
    }
  };
};

/**
 * @brief Static lookup table mapping HTTP status codes to canonical text.
 */
std::vector<std::pair<unsigned int, jowi::generic::fixed_string<40>>>
  jowi::io::http::http_status::__status_to_name = {
    // 1XX
    {100, jowi::generic::fixed_string<40>("CONTINUE")},
    {101, jowi::generic::fixed_string<40>("SWITCHING_PROTOCOLS")},
    {102, jowi::generic::fixed_string<40>("PROCESSING")},
    {103, jowi::generic::fixed_string<40>("EARLY HINTS")},
    // 2XX
    {200, jowi::generic::fixed_string<40>("OK")},
    {201, jowi::generic::fixed_string<40>("CREATED")},
    {202, jowi::generic::fixed_string<40>("ACCEPTED")},
    {203, jowi::generic::fixed_string<40>("NON_AUTHORITATIVE_INFORMATION")},
    {204, jowi::generic::fixed_string<40>("NO_CONTENT")},
    {205, jowi::generic::fixed_string<40>("RESET_CONTENT")},
    {206, jowi::generic::fixed_string<40>("PARTIAL_CONTENT")},
    {207, jowi::generic::fixed_string<40>("MULTI_STATUS")},
    {208, jowi::generic::fixed_string<40>("ALREADY_REPORTED")},
    {226, jowi::generic::fixed_string<40>("IM_USED")},
    // 3XX
    {300, jowi::generic::fixed_string<40>("MULTIPLE_CHOICES")},
    {301, jowi::generic::fixed_string<40>("MOVED_PERMANENTLY")},
    {302, jowi::generic::fixed_string<40>("FOUND")},
    {303, jowi::generic::fixed_string<40>("SEE_OTHER")},
    {304, jowi::generic::fixed_string<40>("NOT_MODIFIED")},
    {305, jowi::generic::fixed_string<40>("USE_PROXY")},
    {306, jowi::generic::fixed_string<40>("SWITCH_PROXY")},
    {307, jowi::generic::fixed_string<40>("TEMPORARY_REDIRECT")},
    {308, jowi::generic::fixed_string<40>("PERMANENT_REDIRECT")},
    // 4XX
    {400, jowi::generic::fixed_string<40>("BAD_REQUEST")},
    {401, jowi::generic::fixed_string<40>("UNAUTHORIZED")},
    {402, jowi::generic::fixed_string<40>("DEPRECATION_WARNING")},
    {403, jowi::generic::fixed_string<40>("FORBIDDEN")},
    {404, jowi::generic::fixed_string<40>("NOT_FOUND")},
    {405, jowi::generic::fixed_string<40>("METHOD_NOT_ALLOWED")},
    {406, jowi::generic::fixed_string<40>("NOT_ACCEPTABLE")},
    {407, jowi::generic::fixed_string<40>("PROXY_AUTHENTICATION_REQUIRED")},
    {408, jowi::generic::fixed_string<40>("REQUEST_TIMEOUT")},
    {409, jowi::generic::fixed_string<40>("CONFLICT")},
    {410, jowi::generic::fixed_string<40>("GONE")},
    {411, jowi::generic::fixed_string<40>("LENGTH_REQUIRED")},
    {412, jowi::generic::fixed_string<40>("PRECONDITION_FAILED")},
    {413, jowi::generic::fixed_string<40>("REQUEST_ENTITY_TOO_LARGE")},
    {414, jowi::generic::fixed_string<40>("REQUEST_URI_TOO_LARGE")},
    {415, jowi::generic::fixed_string<40>("UNSUPPORTED_MEDIA_TYPE")},
    {416, jowi::generic::fixed_string<40>("REQUESTED_RANGE_NOT_SATISFIABLE")},
    {417, jowi::generic::fixed_string<40>("EXPECTATION_FAILED")},
    {418, jowi::generic::fixed_string<40>("I_AM_A_TEAPOT")},
    {421, jowi::generic::fixed_string<40>("MISDIRECTED_REQUEST")},
    {422, jowi::generic::fixed_string<40>("UNPROCESSABLE_ENTITY")},
    {423, jowi::generic::fixed_string<40>("LOCKED")},
    {424, jowi::generic::fixed_string<40>("FAILED_DEPENDENCY")},
    {425, jowi::generic::fixed_string<40>("TOO_EARLY")},
    {426, jowi::generic::fixed_string<40>("UPGRADE_REQUIRED")},
    {428, jowi::generic::fixed_string<40>("PRECONDITION_REQUIRED")},
    {429, jowi::generic::fixed_string<40>("TOO_MANY_REQUESTS")},
    {431, jowi::generic::fixed_string<40>("REQUEST_HEADER_FIELDS_TOO_LARGE")},
    {451, jowi::generic::fixed_string<40>("UNAVAILABLE_FOR_LEGAL_REASONS")},
    // 5XX
    {500, jowi::generic::fixed_string<40>("INTERNAL_SERVER_ERROR")},
    {501, jowi::generic::fixed_string<40>("NOT_IMPLEMENTED")},
    {502, jowi::generic::fixed_string<40>("BAD_GATEWAY")},
    {503, jowi::generic::fixed_string<40>("SERVICE_UNAVAILABLE")},
    {504, jowi::generic::fixed_string<40>("GATEWAY_TIMEOUT")},
    {505, jowi::generic::fixed_string<40>("HTTP_VERSION_NOT_SUPPORTED")},
    {506, jowi::generic::fixed_string<40>("VARIANT_ALSO_NEGOTIATES")},
    {507, jowi::generic::fixed_string<40>("INSUFFICIENT_STORAGE")},
    {508, jowi::generic::fixed_string<40>("LOOP_DETECTED")},
    {510, jowi::generic::fixed_string<40>("NOT_EXTENDED")},
    {511, jowi::generic::fixed_string<40>("NETWORK_AUTHENTICATION_REQUIRED")}
};
