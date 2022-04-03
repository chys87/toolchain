/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2022, chys <admin@CHYS.INFO>
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

#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace cbu {

template <typename U>
constexpr std::ptrdiff_t byte_distance(const U* p, const U* q) {
  if constexpr (!std::is_void_v<U>) {
    if (std::is_constant_evaluated())
      return (q - p) * std::ptrdiff_t(sizeof(U));
  }
  return (reinterpret_cast<std::intptr_t>(q) -
          reinterpret_cast<std::intptr_t>(p));
}

// Same as byte_distance, but return value is unsigned
template <typename U>
constexpr std::make_unsigned_t<std::ptrdiff_t> byte_udistance(const U* p,
                                                              const U* q) {
  return byte_distance(p, q);
}

template <typename U>
constexpr U* byte_advance(U* p, std::ptrdiff_t u) noexcept {
  if constexpr (!std::is_void_v<U>) {
    if (std::is_constant_evaluated() && u % std::ptrdiff_t(sizeof(U)) == 0)
      return (p + u / std::ptrdiff_t(sizeof(U)));
  }
  return reinterpret_cast<U*>(reinterpret_cast<std::intptr_t>(p) + u);
}

template <typename U>
constexpr U* byte_back(U* p, std::ptrdiff_t u) {
  if constexpr (!std::is_void_v<U>) {
    if (std::is_constant_evaluated() && u % std::ptrdiff_t(sizeof(U)) == 0)
      return (p - u / std::ptrdiff_t(sizeof(U)));
  }
  return reinterpret_cast<U*>(reinterpret_cast<std::intptr_t>(p) - u);
}

// A class with similar semantics as size_t, but stores size in bytes.
// ByteSize<T> should compile even if T is an incomplete type, so long as
// sizeof(T) is not required.
template <typename T>
class ByteSize {
 public:
  static_assert(std::is_same_v<T, std::remove_cvref_t<T>>);

  // Uninitialized, for performance
  ByteSize() noexcept = default;
  constexpr ByteSize(std::size_t count) noexcept : bytes_(count * sizeof(T)) {}
  constexpr ByteSize(const ByteSize& other) noexcept = default;

  static constexpr ByteSize from_bytes(std::size_t bytes) noexcept {
    ByteSize r;
    r.bytes_ = bytes;
    return r;
  }

  template <typename U>
  constexpr ByteSize(const ByteSize<U>& other) noexcept
      : bytes_(sizeof(T) >= sizeof(U) && sizeof(T) % sizeof(U) == 0
                   ? other.bytes() * (sizeof(T) / sizeof(U))
               : sizeof(T) < sizeof(U) && sizeof(U) % sizeof(T) == 0
                   ? other.bytes() / (sizeof(U) / sizeof(T))
                   : other.bytes() / sizeof(U) * sizeof(T)) {}

  constexpr ByteSize& operator=(const ByteSize& other) noexcept = default;

  template <typename U>
    requires(std::is_convertible_v<U (*)[], T (*)[]>)
  constexpr ByteSize(const U* lo, const U* hi) noexcept
      : bytes_(byte_distance(lo, hi)) {}

  constexpr std::size_t bytes() const noexcept { return bytes_; }
  constexpr std::size_t size() const noexcept { return bytes_ / sizeof(T); }

  constexpr operator std::size_t() const noexcept { return size(); }
  constexpr explicit operator bool() const noexcept { return bytes_; }

  constexpr ByteSize& operator*=(std::size_t k) noexcept {
    bytes_ *= k;
    return *this;
  }

  constexpr ByteSize& operator+=(const ByteSize& other) noexcept {
    bytes_ += other.bytes_;
    return *this;
  }

  constexpr ByteSize& operator-=(const ByteSize& other) noexcept {
    bytes_ -= other.bytes_;
    return *this;
  }

  constexpr ByteSize& operator+=(std::size_t n) noexcept {
    bytes_ += n * sizeof(T);
    return *this;
  }

  constexpr ByteSize& operator-=(std::size_t n) noexcept {
    bytes_ -= n * sizeof(T);
    return *this;
  }

  constexpr ByteSize& operator++() noexcept {
    bytes_ += sizeof(T);
    return *this;
  }
  constexpr ByteSize& operator--() noexcept {
    bytes_ -= sizeof(T);
    return *this;
  }
  constexpr ByteSize operator++(int) noexcept {
    ByteSize r = *this;
    ++*this;
    return r;
  }
  constexpr ByteSize operator--(int) noexcept {
    ByteSize r = *this;
    --*this;
    return r;
  }

 private:
  std::size_t bytes_;
};

// Comparisons

template <typename T, typename U>
constexpr bool operator==(const ByteSize<T>& a, const ByteSize<U>& b) noexcept {
  // Don't delete the first if constexpr, so that incomplete types are supported
  if constexpr (std::is_same_v<T, U>)
    return a.bytes() == b.bytes();
  else if constexpr (sizeof(T) == sizeof(U))
    return a.bytes() == b.bytes();
  else
    return a.size() == b.size();
}

template <typename T, typename U>
constexpr auto operator<=>(const ByteSize<T>& a,
                           const ByteSize<U>& b) noexcept {
  // Don't delete the first if constexpr, so that incomplete types are supported
  if constexpr (std::is_same_v<T, U>)
    return a.bytes() <=> b.bytes();
  else if constexpr (sizeof(T) == sizeof(U))
    return std::weak_ordering(a.bytes() <=> b.bytes());
  else
    return std::weak_ordering(a.size() <=> b.size());
}

template <typename T, std::integral U>
constexpr bool operator==(const ByteSize<T>& a, U b) noexcept {
  return a.bytes() == b * sizeof(T);
}

template <typename T, std::integral U>
constexpr std::weak_ordering operator<=>(const ByteSize<T>& a, U b) noexcept {
  return a.bytes() <=> b * sizeof(T);
}

// multiplication
template <typename T, std::integral U>
constexpr ByteSize<T> operator*(const ByteSize<T>& a, U b) noexcept {
  return ByteSize<T>::from_bytes(a.bytes() * b);
}

template <typename T, std::integral U>
constexpr ByteSize<T> operator*(U a, const ByteSize<T>& b) noexcept {
  return ByteSize<T>::from_bytes(b.bytes() * a);
}

template <typename T, typename U>
constexpr auto operator*(const ByteSize<T>& a, const ByteSize<U>& b) noexcept {
  return a.size() * b.size();
}

// addition
template <typename T, typename U>
constexpr auto operator+(const ByteSize<T>& a, const ByteSize<U>& b) noexcept {
  if constexpr (std::is_same_v<T, U>)
    return ByteSize<T>::from_bytes(a.bytes() + b.bytes());
  else
    return a.size() + b.size();
}

template <typename T, typename U>
constexpr auto operator-(const ByteSize<T>& a, const ByteSize<U>& b) noexcept {
  if constexpr (std::is_same_v<T, U>)
    return ByteSize<T>::from_bytes(a.bytes() - b.bytes());
  else
    return a.size() - b.size();
}

// Explicitly define them to avoid "ByteSize + int" being compiled as
// "size_t(ByteSize) + int"
template <typename T, std::integral U>
constexpr ByteSize<T> operator+(const ByteSize<T>& a, U b) noexcept {
  return ByteSize<T>::from_bytes(a.bytes() + b * sizeof(T));
}

template <typename T, std::integral U>
constexpr ByteSize<T> operator-(const ByteSize<T>& a, U b) noexcept {
  return ByteSize<T>::from_bytes(a.bytes() - b * sizeof(T));
}

template <typename T, std::integral U>
constexpr ByteSize<T> operator+(U a, const ByteSize<T>& b) noexcept {
  return ByteSize<T>::from_bytes(a * sizeof(T) + b.bytes());
}

template <typename T, std::integral U>
constexpr ByteSize<T> operator-(U a, const ByteSize<T>& b) noexcept {
  return ByteSize<T>::from_bytes(a * sizeof(T) - b.bytes());
}

// Unfortunately we are unable to overload
// operator [] (T *, ByteSize<T>)

template <typename T, typename U>
constexpr T* operator+(T* lo, const ByteSize<U>& diff) noexcept {
  if constexpr (sizeof(T) == sizeof(U))
    return byte_advance(lo, diff.bytes());
  else
    return lo + std::size_t(diff);
}

template <typename T, typename U>
constexpr T*& operator+=(T*& lo, const ByteSize<U>& diff) noexcept {
  if constexpr (sizeof(T) == sizeof(U))
    return lo = byte_advance(lo, diff.bytes());
  else
    return lo += std::size_t(diff);
}

template <typename T, typename U>
constexpr T* operator+(const ByteSize<U>& diff, T* lo) noexcept {
  return lo + diff;
}

template <typename T, typename U>
constexpr T* operator-(T* lo, const ByteSize<U>& diff) noexcept {
  if constexpr (sizeof(T) == sizeof(U))
    return byte_back(lo, diff.bytes());
  else
    return lo - std::size_t(diff);
}

template <typename T, typename U>
constexpr T*& operator-=(T*& lo, const ByteSize<U>& diff) noexcept {
  if constexpr (sizeof(T) == sizeof(U))
    return lo = byte_back(lo, diff.bytes());
  else
    return lo -= std::size_t(diff);
}

}  // namespace cbu
