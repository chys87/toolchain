#pragma once

// Convenient utilities for C++
// Don't include this file directly

#ifndef TX32_INLINE
# error "Don't include me directly. Include tinyx32.h instead."
#endif

#include <type_traits>
#include <utility>

# define TX32_NO_UNIQUE_ADDRESS

#if defined __has_cpp_attribute
# if __has_cpp_attribute(no_unique_address)
#  undef TX32_NO_UNIQUE_ADDRESS
#  define TX32_NO_UNIQUE_ADDRESS [[no_unique_address]]
# endif
#endif

template <typename T> requires std::is_trivially_copyable<T>::value
inline T *Memcpy(T *dst, const void *src, size_t n) {
  return static_cast<T *>(memcpy(dst, src, n));
}

template <typename T> requires std::is_trivially_copyable<T>::value
inline T *Mempcpy(T *dst, const void *src, size_t n) {
  return static_cast<T *>(mempcpy(dst, src, n));
}

template <typename T>
  requires (std::is_trivially_copyable<T>::value and sizeof(T) == 1)
inline T *Mempset(T *dst, T value, size_t n) {
  union {
    T t_value;
    char c_value;
  };
  t_value = value;
  dst = static_cast<T *>(memset(dst, c_value, n));
  return (dst + n);
}

template <typename A, typename B>
struct pair {
    TX32_NO_UNIQUE_ADDRESS A first;
    TX32_NO_UNIQUE_ADDRESS B second;
};

template <typename A, typename B>
pair(A, B) -> pair<A, B>;

namespace detail {

template <typename T, typename ChainMemcpy>
class ChainMemcpyBase {
public:
    explicit constexpr ChainMemcpyBase(T *p) : p_(p) {}

    ChainMemcpy &operator << (T v) {
        *p_++ = v;
        return static_cast<ChainMemcpy &>(*this);
    }

    void operator >> (T *&p) const {
        p = p_;
    }

    T *endptr() const { return p_; }

protected:
    T *p_;
};

} // namespace detail

template <typename T> requires std::is_trivially_copyable<T>::value
class ChainMemcpy : public detail::ChainMemcpyBase<T, ChainMemcpy<T>> {
public:
    using Base = detail::ChainMemcpyBase<T, ChainMemcpy>;
    using Base::Base;
    using Base::operator <<;

    template <typename U>
      requires (sizeof(U) == sizeof(T) and std::is_convertible<U, T>::value)
    ChainMemcpy &operator << (pair<U *, size_t> s) {
        this->p_ = Mempcpy(this->p_, s.first, s.second * sizeof(T));
        return *this;
    }
};

template <>
class ChainMemcpy<char> : public detail::ChainMemcpyBase<char,
                                                         ChainMemcpy<char>> {
public:
  using Base = detail::ChainMemcpyBase<char, ChainMemcpy>;
  using Base::Base;
  using Base::operator <<;

  template <typename T>
    requires (requires(const T &s) {
        { s.data() } -> const char *;
        { s.length() } -> size_t;
      })
  ChainMemcpy &operator << (const T &s) {
    this->p_ = Mempcpy(this->p_, s.data(), s.length());
    return *this;
  }

  ChainMemcpy &operator << (pair<const char *, size_t> s) {
    this->p_ = Mempcpy(this->p_, s.first, s.second);
    return *this;
  }

  ChainMemcpy &operator << (const char *s) {
    this->p_ = Mempcpy(this->p_, s, strlen(s));
    return *this;
  }

  ChainMemcpy &operator << (unsigned v) {
    this->p_ = utoa10(v, this->p_);
    return *this;
  }

  ChainMemcpy &operator << (int v) {
    this->p_ = itoa10(v, this->p_);
    return *this;
  }
};

template <typename T>
ChainMemcpy(T *) -> ChainMemcpy<T>;

namespace detail {

template <typename F>
struct Defer {
  const F &f;
  [[gnu::always_inline]]
  ~Defer() { f(); }
};

} // namespace detail

#define DEFER(...) DEFER_(__COUNTER__, __VA_ARGS__)
#define DEFER_(cnt,...) DEFER__(cnt, __VA_ARGS__)
#define DEFER__(cnt,...) \
  auto RAII_defer_fun_##cnt = [&]() -> void { __VA_ARGS__; }; \
  ::detail::Defer<decltype(RAII_defer_fun_##cnt)> \
    RAII_defer_##cnt{RAII_defer_fun_##cnt}; \
  (void)RAII_defer_##cnt /* Suppress warning */

template <typename T, typename Comp>
void sort(T *lo, T *hi, Comp comp) {
  --hi;
  if (hi <= lo)
    return;

  T *mid = lo; // [lo, mid] is sorted.
  do {
    T *p = mid;
    while (comp(p[1], p[0])) {
      std::swap(p[1], p[0]);
      if (p == lo)
        break;
      --p;
    }
    ++mid;
  } while (mid != hi);
}
