#pragma once

// Convenient utilities for C++
// Don't include this file directly

#ifndef TX32_INLINE
# error "Don't include me directly. Include tinyx32.h instead."
#endif

template <typename T>
inline T *Memcpy(T *dst, const void *src, size_t n) {
    return static_cast<T *>(memcpy(dst, src, n));
}

template <typename T>
inline T *Mempcpy(T *dst, const void *src, size_t n) {
    return static_cast<T *>(mempcpy(dst, src, n));
}
