#pragma once

// Convenient utilities for C++
// Don't include this file directly

#ifndef TX32_INLINE
# error "Don't include me directly. Include tinyx32.h instead."
#endif

#include <type_traits>

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

template <typename A, typename B>
struct pair {
    TX32_NO_UNIQUE_ADDRESS A first;
    TX32_NO_UNIQUE_ADDRESS B second;
};

template <typename A, typename B>
pair(A, B) -> pair<A, B>;

template <typename T> requires std::is_trivially_copyable<T>::value
class ChainMemcpy {
public:
    explicit ChainMemcpy(T *p) : p_(p) {}

    template <typename U> requires sizeof(U) == sizeof(T) and std::is_convertible<U, T>::value
    ChainMemcpy &operator << (pair<U *, size_t> s) {
        p_ = Mempcpy(p_, s.first, s.second * sizeof(T));
        return *this;
    }

    ChainMemcpy &operator << (T v) {
        *p_++ = v;
        return *this;
    }

    ChainMemcpy &operator << (const char *s) requires sizeof(T) == 1 {
        p_ = Mempcpy(p_, s, strlen(s));
        return *this;
    }

    void operator >> (T *&p) const {
        p = p_;
    }

private:
    T *p_;
};
