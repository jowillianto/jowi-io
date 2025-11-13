module;
#include <concepts>
#include <expected>
#include <string_view>
export module jowi.io:file;
import jowi.generic;
import :error;
import :buffer;

/**
 * @file IsFile.cc
 * @brief Concepts and helpers describing generic file capabilities. The concepts described here are
 * mostly described such that they are not related to OS related concepts.
 */

namespace jowi::io {
  /**
   * @brief File seek modes mirroring POSIX origin semantics.
   */
  export enum struct SeekMode { START, CURRENT, END };
  /**
   * @brief Concept describing write capability for a file abstraction.
   */
  export template <class FileType>
  concept IsWritable = requires(FileType &file, std::string_view c) {
    { file.write(c) } -> std::same_as<std::expected<size_t, IoError>>;
  };

  /**
   * @brief Concept describing read capability for a file abstraction.
   */
  export template <class FileType>
  concept IsReadable = requires(FileType &file, DynBuffer &buf) {
    { file.read(buf) } -> std::same_as<std::expected<void, IoError>>;
  };

  /**
   * @brief Concept describing seek capability for a file abstraction.
   */
  export template <class FileType>
  concept IsSeekable = requires(FileType &file, SeekMode m, long long offset) {
    { file.seek(offset) } -> std::same_as<std::expected<long long, IoError>>;
  };

  /**
   * @brief Concept describing synchronization capability for a file abstraction.
   */
  export template <class FileType>
  concept IsSyncable = requires(FileType &file) {
    { file.sync() } -> std::same_as<std::expected<void, IoError>>;
  };

  /**
   * @brief Concept describing the ability to query file size.
   */
  export template <class FileType>
  concept IsSized = requires(FileType &file) {
    { file.size() } -> std::same_as<std::expected<size_t, IoError>>;
  };

  export template <class FileType>
  concept IsOsFile = requires(const FileType &file) {
    { file.native_handle() };
  };
}
