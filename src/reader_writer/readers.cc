module;
#include <algorithm>
#include <concepts>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
export module jowi.io:readers;
import jowi.generic;
import :local_file;
import :file;
import :error;
import :buffer;

namespace jowi::io {
  enum struct NextAction { next_end, next_continue };
  export template <class T>
  concept Nextable = requires(T it) {
    { std::declval<typename T::value_type>() };
    { it.next() } -> std::same_as<std::optional<typename T::value_type>>;
  };

  export template <class T, class prev_value>
  concept ChainNextable = requires(T it, std::optional<prev_value> &p) {
    { std::declval<typename T::value_type>() };
    { it.next(p) } -> std::same_as<generic::Variant<typename T::value_type, NextAction>>;
  };

  export template <RwBuffer buf_type, IsReadable FileType> struct BufNextable {
  private:
    buf_type __b;
    FileType &__f;

  public:
    using value_type = std::expected<std::string_view, IoError>;
    BufNextable(buf_type b, FileType &f) : __b{std::move(b)}, __f{f} {}

    std::optional<value_type> next() {
      auto res = __f.read(__b);
      if (!res) {
        return std::unexpected{res.error()};
      }
      if (!__b.is_readable()) {
        return std::nullopt;
      }
      std::string_view v{
        static_cast<const char *>(__b.read_beg()), static_cast<const char *>(__b.read_end())
      };
      __b.mark_read(__b.readable_size());
      return v;
    }
  };

  export struct LineNextable {
  private:
    std::optional<std::string> __left;
    char __sep;

    std::optional<std::string> __parse_sep() {
      if (!__left) return std::nullopt;
      auto sep_it = std::ranges::find(__left->begin(), __left->end(), __sep);
      if (sep_it == __left->end()) return std::nullopt;
      std::string pre_sep = {__left->begin(), sep_it};
      __left.emplace(sep_it + 1, __left->end());
      return pre_sep;
    }

  public:
    using value_type = std::expected<std::string, IoError>;
    LineNextable(char sep = '\n') : __left{std::nullopt}, __sep{sep} {}

    generic::Variant<value_type, NextAction> next(
      std::optional<std::expected<std::string_view, IoError>> &prev
    ) {
      auto pre_sep = __parse_sep();
      // pre_sep prev or !prev
      if (pre_sep) return std::move(pre_sep).value();
      // !pre_sep !prev, nothing left to read
      if (!pre_sep && !prev) {
        // send all leftovers
        if (__left) {
          return std::move(__left).value();
        } else
          // nothing else not read
          return NextAction::next_end;
      }
      // !pre_sep prev , prev err
      if (prev && !(prev->has_value())) return std::unexpected{prev->error()};
      // !pre_sep prev, prev val
      __left.emplace(__left.value_or("").append(prev->value()));
      return NextAction::next_continue;
    }
  };

  export struct CsvNextable {
  private:
    char __sep;
    LineNextable __n;

  public:
    using value_type = std::expected<std::vector<std::string>, IoError>;
    CsvNextable(char sep = ',') : __sep{sep}, __n{'\n'} {}

    generic::Variant<value_type, NextAction> next(
      std::optional<std::expected<std::string_view, IoError>> &prev
    ) {}
  };

  export template <Nextable N, ChainNextable<typename N::value_type> C> struct NextableChain {
  private:
    N __n;
    C __c;
    std::optional<typename N::value_type> __v;

  public:
    using value_type = typename C::value_type;
    NextableChain(N n, C c) : __n{std::move(n)}, __c{std::move(c)}, __v{__n.next()} {}

    std::optional<typename C::value_type> next() {
      return __c.next(__v).visit(
        [](typename C::value_type v) -> std::optional<typename C::value_type> { return v; },
        [&](NextAction n) -> std::optional<typename C::value_type> {
          if (n == NextAction::next_end) {
            return std::nullopt;
          } else {
            __v = __n.next();
            return next();
          }
        }
      );
    }
  };

  export template <Nextable N> struct NextableIterator {
  private:
    struct IteratorState {
      N n;
      typename N::value_type v;
    };
    std::optional<IteratorState> __s;

    void __update_state() {
      if (__s) {
        auto res = __s->n.next();
        if (res) {
          __s.emplace(std::move(__s->n), std::move(res.value()));
        }
      }
    }

  public:
    NextableIterator() : __s{std::nullopt} {}
    NextableIterator(N n) : NextableIterator{} {
      auto res = n.next();
      if (res) {
        __s.emplace(std::move(n), std::move(res.value()));
      }
    }
    template <class... Args> requires(std::constructible_from<N, Args...>)
    NextableIterator(Args &&...args) : NextableIterator{N{std::forward<Args>(args)...}} {}

    NextableIterator &begin() noexcept {
      return *this;
    }
    NextableIterator end() noexcept {
      return NextableIterator{};
    }

    typename N::value_type operator*() noexcept {
      typename N::value_type v = std::move(__s->v);
      __update_state();
      return v;
    }

    NextableIterator &operator++() {
      return *this;
    }

    void operator++(int) {
      return *this;
    }

    friend bool operator==(const NextableIterator &l, const NextableIterator &r) {
      if (!l.__s && !r.__s) {
        return true;
      } else {
        return false;
      }
    }
  };

  // concatenation operator
  export template <Nextable left_type, ChainNextable<typename left_type::value_type> right_type>
  NextableChain<left_type, right_type> operator|(left_type l, right_type r) {
    return NextableChain{std::move(l), std::move(r)};
  }

  export template <WritableBuffer buf, IsReadable file>
  struct BufferIterator : NextableIterator<BufNextable<buf, file>> {
    BufferIterator(buf b, file &f) : NextableIterator<BufNextable<buf, file>>{b, f} {}
  };

  export template <WritableBuffer buf, IsReadable file>
  struct LineIterator : NextableIterator<NextableChain<BufNextable<buf, file>, LineNextable>> {
    LineIterator(buf b, file &f, char sep = '\n') :
      NextableIterator<NextableChain<BufNextable<buf, file>, LineNextable>>{
        BufNextable{std::move(b), f}, LineNextable{sep}
      } {}
  };
}
