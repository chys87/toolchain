/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2025, chys <admin@CHYS.INFO>
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
#include <limits>
#include <type_traits>

namespace cbu {

template <typename U>
constexpr std::ptrdiff_t byte_distance(const U* p, const U* q) {
  if constexpr (!std::is_void_v<U>) {
    if consteval {
      return (q - p) * std::ptrdiff_t(sizeof(U));
    }
    if constexpr (sizeof(U) == 1) {
      return q - p;
    }
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
    if consteval {
      if (u % std::ptrdiff_t(sizeof(U)) == 0)
        return (p + u / std::ptrdiff_t(sizeof(U)));
    }
    if constexpr (sizeof(U) == 1) {
      return p + u;
    }
  }
  return reinterpret_cast<U*>(reinterpret_cast<std::intptr_t>(p) + u);
}

template <typename U>
constexpr U* byte_back(U* p, std::ptrdiff_t u) {
  if constexpr (!std::is_void_v<U>) {
    if consteval {
      if (u % std::ptrdiff_t(sizeof(U)) == 0)
        return (p - u / std::ptrdiff_t(sizeof(U)));
    }
    if constexpr (sizeof(U) == 1) {
      return p - u;
    }
  }
  return reinterpret_cast<U*>(reinterpret_cast<std::intptr_t>(p) - u);
}

// A class with similar semantics as size_t, but stores size in bytes.
// ByteSize<T> should compile even if T is an incomplete type, so long as
// sizeof(T) is not required.
template <typename T, typename U = std::size_t>
class ByteSize {
 public:
  static_assert(std::is_same_v<T, std::remove_cvref_t<T>>);
  static_assert(std::is_same_v<U, unsigned long long> ||
                std::is_same_v<U, unsigned long> ||
                std::is_same_v<U, unsigned> ||
                std::is_same_v<U, unsigned short> ||
                std::is_same_v<U, unsigned char>);

  // Uninitialized, for performance
  ByteSize() noexcept = default;
  constexpr ByteSize(U count) noexcept : bytes_(count * sizeof(T)) {}
  constexpr ByteSize(const ByteSize& other) noexcept = default;

  static constexpr ByteSize from_bytes(U bytes) noexcept {
    ByteSize r;
    r.bytes_ = bytes;
    return r;
  }

  template <typename V, typename W>
    requires(sizeof(T) >= sizeof(V) && sizeof(T) % sizeof(V) == 0)
  constexpr ByteSize(const ByteSize<V, W>& other) noexcept
      : bytes_(other.bytes() * (sizeof(T) / sizeof(V))) {}

  template <typename V, typename W>
    requires(sizeof(T) < sizeof(V) && sizeof(V) % sizeof(T) == 0)
  constexpr ByteSize(const ByteSize<V, W>& other) noexcept
      : bytes_(other.bytes() / (sizeof(V) / sizeof(T))) {}

  template <typename V, typename W>
    requires(!(sizeof(T) >= sizeof(V) && sizeof(T) % sizeof(V) == 0) &&
             !(sizeof(T) < sizeof(V) && sizeof(V) % sizeof(T) == 0))
  constexpr ByteSize(const ByteSize<V, W>& other) noexcept
      : bytes_(other.bytes() / sizeof(V) * sizeof(T)) {}

  constexpr ByteSize& operator=(const ByteSize& other) noexcept = default;

  template <typename V>
    requires(std::is_convertible_v<V (*)[], T (*)[]>)
  constexpr ByteSize(const V* lo, const V* hi) noexcept
      : bytes_(byte_distance(lo, hi)) {}

  constexpr U bytes() const noexcept { return bytes_; }
  constexpr U size() const noexcept { return bytes_ / sizeof(T); }

  constexpr bool valid() const noexcept { return bytes_ % sizeof(T) == 0; }

  constexpr operator U() const noexcept { return size(); }
  constexpr explicit operator bool() const noexcept { return bytes_; }

  constexpr ByteSize& operator*=(U k) noexcept {
    bytes_ *= k;
    return *this;
  }

  constexpr ByteSize& operator/=(U k) noexcept {
    if constexpr ((sizeof(T) & (sizeof(T) - 1)) == 0) {
      bytes_ = (bytes_ / k) & ~(sizeof(T) - 1);
    } else {
      bytes_ = bytes_ / sizeof(T) / k * sizeof(T);
    }
    return *this;
  }

  constexpr ByteSize& operator%=(U k) noexcept {
    bytes_ %= k * sizeof(T);
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

  constexpr ByteSize& operator+=(U n) noexcept {
    bytes_ += n * sizeof(T);
    return *this;
  }

  constexpr ByteSize& operator-=(U n) noexcept {
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
  U bytes_;
};

template <typename T, typename U>
  requires std::is_same_v<std::remove_cv_t<T>, std::remove_cv_t<U>>
ByteSize(T*, U*) -> ByteSize<std::remove_cv_t<T>>;

// Comparisons

template <typename T, typename TU, typename U, typename UU>
constexpr bool operator==(const ByteSize<T, TU>& a, const ByteSize<U, UU>& b) noexcept {
  // Don't delete the first if constexpr, so that incomplete types are supported
  if constexpr (std::is_same_v<T, U>)
    return a.bytes() == b.bytes();
  else if constexpr (sizeof(T) == sizeof(U))
    return a.bytes() == b.bytes();
  else
    return a.size() == b.size();
}

template <typename T, typename TU, typename U, typename UU>
constexpr auto operator<=>(const ByteSize<T, TU>& a,
                           const ByteSize<U, UU>& b) noexcept {
  // Don't delete the first if constexpr, so that incomplete types are supported
  if constexpr (std::is_same_v<T, U>)
    return a.bytes() <=> b.bytes();
  else if constexpr (sizeof(T) == sizeof(U))
    return std::weak_ordering(a.bytes() <=> b.bytes());
  else
    return std::weak_ordering(a.size() <=> b.size());
}

template <typename T, typename TU, std::integral U>
constexpr bool operator==(const ByteSize<T, TU>& a, U b) noexcept {
  return a.bytes() == b * sizeof(T);
}

template <typename T, typename TU, std::integral U>
constexpr std::weak_ordering operator<=>(const ByteSize<T, TU>& a, U b) noexcept {
  return a.bytes() <=> b * sizeof(T);
}

// multiplication
template <typename T, typename TU, std::integral U>
constexpr auto operator*(const ByteSize<T, TU>& a, U b) noexcept {
  return ByteSize<T, std::make_unsigned_t<std::common_type_t<TU, U>>>::
      from_bytes(a.bytes() * b);
}

template <typename T, typename TU, std::integral U>
constexpr auto operator*(U a, const ByteSize<T, TU>& b) noexcept {
  return ByteSize<T, std::make_unsigned_t<std::common_type_t<TU, U>>>::
      from_bytes(b.bytes() * a);
}

template <typename T, typename TU, typename U, typename UU>
constexpr auto operator*(const ByteSize<T, TU>& a,
                         const ByteSize<U, UU>& b) noexcept {
  return a.size() * b.size();
}

// division
template <typename T, typename TU, std::integral U>
constexpr ByteSize<T, TU> operator/(const ByteSize<T, TU>& a, U b) noexcept {
  if constexpr ((sizeof(T) & (sizeof(T) - 1)) == 0)
    return ByteSize<T, TU>::from_bytes((a.bytes() / b) & ~(sizeof(T) - 1));
  else
    return ByteSize<T, TU>::from_bytes(a.bytes() / sizeof(T) / b * sizeof(T));
}

template <typename T, typename TU, std::integral U>
constexpr auto operator/(U a, const ByteSize<T, TU>& b) noexcept {
  return a / TU(b);
}

template <typename T, typename TU, typename U, typename UU>
constexpr auto operator/(const ByteSize<T, TU>& a,
                         const ByteSize<U, UU>& b) noexcept {
  return a / TU(UU(b));
}

// modulo
template <typename T, typename TU, std::integral U>
constexpr ByteSize<T, TU> operator%(const ByteSize<T, TU>& a, U b) noexcept {
  return ByteSize<T, TU>::from_bytes(a.bytes() % (b * sizeof(T)));
}

template <typename T, typename TU, std::integral U>
constexpr auto operator%(U a, const ByteSize<T, TU>& b) noexcept {
  return a % TU(b);
}

template <typename T, typename TU, typename U, typename UU>
constexpr auto operator%(const ByteSize<T, TU>& a, const ByteSize<U, UU>& b) noexcept {
  return a % TU(UU(b));
}

// addition
template <typename T, typename TU, typename U, typename UU>
constexpr auto operator+(const ByteSize<T, TU>& a, const ByteSize<U, UU>& b) noexcept {
  if constexpr (std::is_same_v<T, U>)
    return ByteSize<T, std::common_type_t<TU, UU>>::from_bytes(a.bytes() + b.bytes());
  else
    return a.size() + b.size();
}

template <typename T, typename TU, typename U, typename UU>
constexpr auto operator-(const ByteSize<T, TU>& a, const ByteSize<U, UU>& b) noexcept {
  if constexpr (std::is_same_v<T, U>)
    return ByteSize<T, std::common_type_t<TU, UU>>::from_bytes(a.bytes() -
                                                               b.bytes());
  else
    return a.size() - b.size();
}

// Explicitly define them to avoid "ByteSize + int" being compiled as
// "size_t(ByteSize) + int"
template <typename T, typename TU, std::integral U>
constexpr auto operator+(const ByteSize<T, TU>& a, U b) noexcept {
  return ByteSize<T, std::make_unsigned_t<std::common_type_t<TU, U>>>::
      from_bytes(a.bytes() + b * sizeof(T));
}

template <typename T, typename TU, std::integral U>
constexpr auto operator-(const ByteSize<T, TU>& a, U b) noexcept {
  return ByteSize<T, std::make_unsigned_t<std::common_type_t<TU, U>>>::
      from_bytes(a.bytes() - b * sizeof(T));
}

template <typename T, typename TU, std::integral U>
constexpr auto operator+(U a, const ByteSize<T, TU>& b) noexcept {
  return ByteSize<T, std::make_unsigned_t<std::common_type_t<TU, U>>>::
      from_bytes(a * sizeof(T) + b.bytes());
}

template <typename T, typename TU, std::integral U>
constexpr auto operator-(U a, const ByteSize<T, TU>& b) noexcept {
  return ByteSize<T, std::make_unsigned_t<std::common_type_t<TU, U>>>::
      from_bytes(a * sizeof(T) - b.bytes());
}

// Unfortunately we are unable to overload
// operator [] (T *, ByteSize<T>)

template <typename T, typename U, typename UU>
constexpr T* operator+(T* lo, const ByteSize<U, UU>& diff) noexcept {
  if constexpr (sizeof(T) == sizeof(U))
    return byte_advance(lo, diff.bytes());
  else
    return lo + diff.size();
}

template <typename T, typename U, typename UU>
constexpr T*& operator+=(T*& lo, const ByteSize<U, UU>& diff) noexcept {
  if constexpr (sizeof(T) == sizeof(U))
    return lo = byte_advance(lo, diff.bytes());
  else
    return lo += diff.size();
}

template <typename T, typename U, typename UU>
constexpr T* operator+(const ByteSize<U, UU>& diff, T* lo) noexcept {
  return lo + diff;
}

template <typename T, typename U, typename UU>
constexpr T* operator-(T* lo, const ByteSize<U, UU>& diff) noexcept {
  if constexpr (sizeof(T) == sizeof(U))
    return byte_back(lo, diff.bytes());
  else
    return lo - diff.size();
}

template <typename T, typename U, typename UU>
constexpr T*& operator-=(T*& lo, const ByteSize<U, UU>& diff) noexcept {
  if constexpr (sizeof(T) == sizeof(U))
    return lo = byte_back(lo, diff.bytes());
  else
    return lo -= diff.size();
}

}  // namespace cbu

namespace std {

template <typename T, typename U>
struct numeric_limits<cbu::ByteSize<T, U>> : std::numeric_limits<U> {
  static constexpr cbu::ByteSize<T, U> min() noexcept { return 0; }
  static constexpr cbu::ByteSize<T, U> max() noexcept {
    return std::numeric_limits<U>::max() / sizeof(T);
  }
};

}  // namespace std
