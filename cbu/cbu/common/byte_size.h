/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2020, chys <admin@CHYS.INFO>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of chys <admin@CHYS.INFO> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY chys <admin@CHYS.INFO> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL chys <admin@CHYS.INFO> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace cbu {
inline namespace cbu_byte_size {

template <typename U, typename V>
  requires (std::is_convertible_v<U*, V*> || std::is_convertible_v<V*, U*>)
inline constexpr std::ptrdiff_t byte_distance(U *p, V *q) {
  return (reinterpret_cast<std::intptr_t>(q) -
      reinterpret_cast<std::intptr_t>(p));
}

template <typename U>
inline constexpr U *byte_advance(U *p, std::ptrdiff_t u) {
  return reinterpret_cast<U *>(reinterpret_cast<std::intptr_t>(p) + u);
}

template<typename U>
inline constexpr U *byte_back(U *p, std::ptrdiff_t u) {
  return reinterpret_cast<U *>(reinterpret_cast<std::intptr_t>(p) - u);
}

// A class with similar semantics as size_t, but stores size in bytes
template <std::size_t N> requires (N > 0)
class ByteSize {
 private:
  explicit constexpr ByteSize(int, std::size_t bytes) noexcept : bytes_(bytes) {
  }

 public:
  ByteSize() noexcept = default;
  constexpr ByteSize(std::size_t count) noexcept : bytes_(count * N) {}
  constexpr ByteSize(const ByteSize &other) noexcept = default;

  static constexpr ByteSize from_bytes(std::size_t bytes) noexcept {
    return ByteSize(0, bytes);
  }

  template <std::size_t M> requires (N > M && N % M == 0)
  constexpr ByteSize(const ByteSize<M> &other) noexcept
    : bytes_(other.bytes() * (N / M)) {
  }

  template <std::size_t M> requires (N < M && M % N == 0)
  constexpr ByteSize(const ByteSize<M> &other) noexcept
    : bytes_(other.bytes() / (M / N)) {
  }

  template <std::size_t M> requires (N % M != 0 && M % N != 0)
  constexpr ByteSize(const ByteSize<M> &other) noexcept
    : ByteSize(std::size_t(other)) {
  }

  constexpr ByteSize &operator = (const ByteSize &other) noexcept = default;

  template <typename U, typename V>
    requires (sizeof(U) == N && sizeof(V) == N &&
              (std::is_convertible_v<U*, V*> || std::is_convertible_v<V*, U*>))
  constexpr ByteSize(U *lo, V *hi) : bytes_(byte_distance(lo, hi)) {}

  constexpr std::size_t bytes() const noexcept { return bytes_; }
  constexpr std::size_t size() const noexcept { return bytes_ / N; }

  constexpr operator std::size_t() const noexcept { return size(); }
  constexpr explicit operator bool() const noexcept { return bytes_; }

  constexpr ByteSize &operator *= (std::size_t k) noexcept {
    bytes_ *= k;
    return *this;
  }

  template <std::size_t M>
  ByteSize &operator *= (ByteSize<M>) noexcept = delete;

  constexpr ByteSize &operator += (const ByteSize &other) noexcept {
    bytes_ += other.bytes_;
    return *this;
  }

  constexpr ByteSize &operator -= (const ByteSize &other) noexcept {
    bytes_ -= other.bytes_;
    return *this;
  }

  constexpr ByteSize &operator ++ () noexcept { bytes_ += N; return *this; }
  constexpr ByteSize &operator -- () noexcept { bytes_ -= N; return *this; }
  constexpr ByteSize operator ++ (int) noexcept {
    ByteSize r = *this;
    ++*this;
    return r;
  }
  constexpr ByteSize operator -- (int) noexcept {
    ByteSize r = *this;
    --*this;
    return r;
  }

 private:
  std::size_t bytes_;
};

#define CBU_DEFINE_BYTESIZE_COMP(op) \
  template <std::size_t N> \
  inline constexpr bool operator op(ByteSize<N> a, ByteSize<N> b) noexcept { \
    return (a.bytes() op b.bytes()); \
  }
CBU_DEFINE_BYTESIZE_COMP(==)
CBU_DEFINE_BYTESIZE_COMP(!=)
CBU_DEFINE_BYTESIZE_COMP(>=)
CBU_DEFINE_BYTESIZE_COMP(<=)
CBU_DEFINE_BYTESIZE_COMP(>)
CBU_DEFINE_BYTESIZE_COMP(<)
#undef CBU_DEFINE_BYTESIZE_COMP

template <std::size_t N, typename T> requires std::is_integral<T>::value
inline constexpr ByteSize<N> operator * (const ByteSize<N> &a, T b) noexcept {
  return ByteSize<N>::from_bytes(a.bytes() * b);
}

template <std::size_t N, typename T> requires std::is_integral<T>::value
inline constexpr ByteSize<N> operator * (T a, const ByteSize<N> &b) noexcept {
  return ByteSize<N>::from_bytes(b.bytes() * a);
}

template <std::size_t N>
inline constexpr ByteSize<N> operator + (const ByteSize<N> &a,
                                         const ByteSize<N> &b) noexcept {
  return ByteSize<N>::from_bytes(a.bytes() + b.bytes());
}

template <std::size_t N>
inline constexpr ByteSize<N> operator - (const ByteSize<N> &a,
                                         const ByteSize<N> &b) noexcept {
  return ByteSize<N>::from_bytes(a.bytes() - b.bytes());
}

// Explicitly define them to avoid "ByteSize + int" being compiled as
// "size_t(ByteSize) + int"
template <std::size_t N, typename T> requires std::is_integral<T>::value
inline constexpr ByteSize<N> operator + (const ByteSize<N> &a, T b) noexcept {
  return ByteSize<N>::from_bytes(a.bytes() + b * N);
}

template <std::size_t N, typename T> requires std::is_integral<T>::value
inline constexpr ByteSize<N> operator - (const ByteSize<N> &a, T b) noexcept {
  return ByteSize<N>::from_bytes(a.bytes() - b * N);
}

template <std::size_t N, typename T> requires std::is_integral<T>::value
inline constexpr ByteSize<N> operator + (T a, const ByteSize<N> &b) noexcept {
  return ByteSize<N>::from_bytes(a * N + b.bytes());
}

template <std::size_t N, typename T> requires std::is_integral<T>::value
inline constexpr ByteSize<N> operator - (T a, const ByteSize<N> &b) noexcept {
  return ByteSize<N>::from_bytes(a * N - b.bytes());
}

template <std::size_t N>
inline constexpr ByteSize<N> operator + (const ByteSize<N> &a) noexcept {
  return a;
}

template <std::size_t N> ByteSize<N> operator - (const ByteSize<N> &a) = delete;

// Unfortunately we are unable to overload
// operator [] (T *, ByteSize<sizeof(T)>)

template <typename T>
inline constexpr T *operator + (T *lo,
                                const ByteSize<sizeof(T)> &diff) noexcept {
  return byte_advance(lo, diff.bytes());
}

template <typename T>
inline constexpr T *&operator += (T *&lo,
                                  const ByteSize<sizeof(T)> &diff) noexcept {
  lo = byte_advance(lo, diff.bytes());
  return lo;
}

template <typename T>
inline constexpr T *operator + (const ByteSize<sizeof(T)> &diff,
                                T *lo) noexcept {
  return byte_advance(lo, diff.bytes());
}

template <typename T>
inline constexpr T *operator - (T *lo,
                                const ByteSize<sizeof(T)> &diff) noexcept {
  return byte_back(lo, diff.bytes());
}

template <typename T>
inline constexpr T *&operator -= (T *&lo,
                                  const ByteSize<sizeof(T)> &diff) noexcept {
  lo = byte_back(lo, diff.bytes());
  return lo;
}

} // inline namespace cbu_byte_size
} // namespace cbu
