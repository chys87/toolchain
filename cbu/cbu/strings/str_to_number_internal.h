/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2024, chys <admin@CHYS.INFO>
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

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

#include "cbu/common/tags.h"

namespace cbu {

template <typename T>
struct StrToNumberPartialResult {
  std::optional<T> value_opt;
  const char* endptr;
};

// Compatible name
template <typename T>
using StrToIntegerPartialResult = StrToNumberPartialResult<T>;

namespace str_to_number_detail {

template <int base_, bool check_overflow_, bool halt_on_overflow_,
          unsigned long long overflow_threshold_>
struct IntegerOptionTag {
  static constexpr int base = base_;
  static constexpr bool check_overflow = check_overflow_;
  static constexpr bool halt_on_overflow = halt_on_overflow_;
  static constexpr unsigned long long overflow_threshold = overflow_threshold_;

  template <int new_base>
    requires(new_base == 0 || (new_base >= 2 && new_base <= 36))
  auto operator+(RadixTag<new_base>)
      -> IntegerOptionTag<new_base, check_overflow, halt_on_overflow,
                          overflow_threshold>;

  auto operator+(IgnoreOverflowTag)
      -> IntegerOptionTag<base, false, halt_on_overflow, overflow_threshold>;

  auto operator+(HaltScanOnOverflowTag)
      -> IntegerOptionTag<base, check_overflow, true, overflow_threshold>;

  template <unsigned long long threshold>
  auto operator+(OverflowThresholdTag<threshold>)
      -> IntegerOptionTag<base, check_overflow, halt_on_overflow, threshold>;
};

using IntegerDefaultOptionTag = IntegerOptionTag<10, true, false, ~0ull>;

template <typename... Options>
concept IntegerValidOptions = requires(Options&&... options) {
  {(IntegerDefaultOptionTag() + ... + options)};
};

template <typename... Options>
  requires IntegerValidOptions<Options...>
struct IntegerOptionParser {
  using Tag = decltype((IntegerDefaultOptionTag() + ... + Options()));
};

template <typename T, typename... Options>
concept IntegerSupported =
    std::integral<T> && sizeof(T) <= 8 && IntegerValidOptions<Options...>;

template <typename T>
using ConversionType = std::conditional_t<
    sizeof(T) == 8,
    std::conditional_t<std::is_signed_v<T>, std::int64_t, std::uint64_t>,
    std::conditional_t<std::is_signed_v<T>, std::int32_t, std::uint32_t>>;

template <typename T>
constexpr unsigned long long OverflowThresholdByType =
    std::is_signed_v<T>
        ? std::make_unsigned_t<T>(std::numeric_limits<T>::max()) + 1
        : std::numeric_limits<T>::max();

template <typename T>
inline constexpr auto extract_value(std::optional<T> v) noexcept {
  return v;
}

template <typename T>
inline constexpr auto extract_value(StrToNumberPartialResult<T> v) noexcept {
  return v.value_opt;
}

template <typename U, typename T>
inline constexpr auto replace_value(std::optional<U>, T v) noexcept {
  return v;
}

template <typename U, typename T>
inline constexpr StrToNumberPartialResult<T> replace_value(
    StrToNumberPartialResult<U> r, std::optional<T> v) noexcept {
  return {v, r.endptr};
}

constexpr bool isdigit(unsigned char c) noexcept {
  return unsigned(c - '0') < 10;
}

constexpr bool isxdigit(unsigned char c) noexcept {
  return isdigit(c) || (unsigned((c | 0x20) - 'a') < 6);
}

template <int base>
inline constexpr std::optional<unsigned> parse_one_digit(
    std::uint8_t c) noexcept {
  if constexpr (base <= 10) {
    if (unsigned C = c - '0'; C < base) return C;
  } else {
    if (unsigned C = c - '0'; C < 10) return C;
    if (unsigned C = (c | 0x20) - 'a'; C < base - 10) return C + 10;
  }
  return std::nullopt;
}

template <typename T>
inline constexpr T powu(T a, unsigned t, T r = T(1)) noexcept {
  while (t) {
    if (t & 1) r *= a;
    t >>= 1;
    a *= a;
  }
  return r;
}

// "daz" stands for "denormal as zero"
template <typename T>
inline constexpr T pow2_daz(int t) noexcept {
  if constexpr (!std::numeric_limits<T>::is_iec559 || sizeof(T) > 8) {
    return powu(t >= 0 ? T(2) : T(.5), t >= 0 ? t : -t);
  } else {
    int lo = std::numeric_limits<T>::min_exponent - 1;
    int hi = std::numeric_limits<T>::max_exponent - 1;
    if (t < lo || t > hi) [[unlikely]]
      return t < 0 ? T(0) : T(INFINITY);
    if consteval {
      return powu(t >= 0 ? T(2) : T(.5), t >= 0 ? t : -t);
    } else {
      using UT =
          std::conditional_t<(sizeof(T) <= 4), std::uint32_t, std::uint64_t>;
      union {
        UT ut;
        T res;
      };
      ut = UT(unsigned(t - lo + 1)) << (std::numeric_limits<T>::digits - 1);
      return res;
    }
  }
}

// Essentially the same as add_overflow in fastarith.h, but
// only supports unsigned integers, and we choose not to include fastarith.h
template <std::unsigned_integral T>
inline constexpr bool add_overflow(T a, T b, T* c) noexcept {
  if consteval {
    *c = a + b;
    return (*c < a || *c < b);
  } else {
    return __builtin_add_overflow(a, b, c);
  }
}

// threshold means max value
template <std::unsigned_integral T, int base,
          unsigned long long overflow_threshold>
inline constexpr std::optional<T> mul_base_add(T x, T digit) noexcept {
  constexpr T kTypeMax = ~T(0);
  constexpr T kMax =
      (overflow_threshold > kTypeMax ? kTypeMax : T(overflow_threshold));

  // We know that x <= kMax, and digit < base

  if constexpr (kMax <= (kTypeMax - (base - 1)) / base) {
    // kMax is small enough, so that we only need to check at the last step
    x = x * base + digit;
    if (x > kMax) return std::nullopt;
    return x;
  }

  // Check multiplication overflow
  // * If multiplicaiton won't overflow T, skip this test
  // * However, if checking this would guarantee addition never overflow,
  //   prefer checking here.
  constexpr bool kCheckBeforeMul =
      kMax > kTypeMax / base || kMax % base == base - 1;
  if constexpr (kCheckBeforeMul) {
    if (x > kMax / base) return std::nullopt;
  }
  x *= base;

  constexpr T kCurMax = (kCheckBeforeMul ? kMax / base : kMax) * base;
  // Now x <= kCurMax

  if constexpr (kCurMax <= kTypeMax - (base - 1)) {
    // This means addition never overflows T
    x += digit;
    // If overflowing kMax is possible, still need to check
    if constexpr (kCurMax > kMax - (base - 1)) {
      if (x > kMax) return std::nullopt;
    }
  } else if constexpr (kMax == kTypeMax) {
    // Just use native overflowcheck.
    if (add_overflow(x, digit, &x)) return std::nullopt;
  } else {
    // This is the most complicated case (though highly unlikely will anybody
    // use this actually)
    if (x > kMax - digit) return std::nullopt;
    x += digit;
  }
  return x;
}

template <std::integral T, bool partial, typename Tag>
constexpr auto str_to_integer(const char* s, const char* e) noexcept {
  auto make_ret = [&](std::optional<T> v) constexpr noexcept {
    if constexpr (partial)
      return StrToNumberPartialResult<T>{v, s};
    else
      return v;
  };
  auto make_bad_ret = [&] constexpr noexcept { return make_ret(std::nullopt); };

  // Handle negative values
  if constexpr (std::is_signed_v<T>) {
    if (s >= e) [[unlikely]]
      return make_bad_ret();

    T sign = 0;
    if (*s == '+' || *s == '-') {
      if (*s == '-') sign = T(-1);
      ++s;
    }
    auto abs_ret = str_to_integer<std::make_unsigned_t<T>, partial, Tag>(s, e);
    auto abs_opt_val = extract_value(abs_ret);
    if (!abs_opt_val) [[unlikely]]
      return replace_value(abs_ret, std::optional<T>(std::nullopt));
    T real_val = (*abs_opt_val ^ sign) - sign;
    if constexpr (Tag::overflow_threshold > std::numeric_limits<T>::max()) {
      // Test overflow after adding sign, whether real_val and sign has the
      // same sign bit.
      if (T(real_val ^ sign) < T(0)) [[unlikely]]
        return replace_value(abs_ret, std::optional<T>(std::nullopt));
    }
    return replace_value(abs_ret, std::optional(real_val));
  } else if constexpr (Tag::base == 0) {
    // Handle base 0
    if (s + 1 < e && *s == '0') {
      if (s[1] == 'x' || s[1] == 'X')
        return str_to_integer<T, partial, decltype(Tag() + HexTag())>(s + 2, e);
      else if ((s[1] == 'o' || s[1] == 'O') || isdigit(s[1]))
        return str_to_integer<T, partial, decltype(Tag() + OctTag())>(
            s + 1 + (s[1] == 'o' || s[1] == 'O'), e);
    }
    return str_to_integer<T, partial, decltype(Tag() + DecTag())>(s, e);
  } else {
    // Now T is unsigned, and base is between 2 and 36
    if (s >= e) [[unlikely]]
      return make_bad_ret();

    bool parsed_any_char = false;
    T val = 0;

    while (s < e) {
      auto digit_opt = parse_one_digit<Tag::base>(*s);
      if (!digit_opt) {
        if (partial && parsed_any_char) break;
        return make_bad_ret();
      }
      ++s;
      parsed_any_char = true;

      if constexpr (Tag::check_overflow) {
        auto new_val_opt = mul_base_add<T, Tag::base, Tag::overflow_threshold>(
            val, T(*digit_opt));
        if (!new_val_opt) [[unlikely]] {
          if constexpr (partial && !Tag::halt_on_overflow) {
            // In partial match, we need to consume all remaining digits, to
            // prevent surprise.
            while (s < e && parse_one_digit<Tag::base>(*s)) ++s;
          }
          return make_bad_ret();
        }
        val = *new_val_opt;
      } else {
        val = val * Tag::base + *digit_opt;
      }
    }

    return make_ret(val);
  }
};

template <bool supports_hex_ = false>
struct FpOptionTag {
  static constexpr bool supports_hex = supports_hex_;

  template <int new_base>
    requires(new_base == 0 || new_base == 10)
  auto operator+(RadixTag<new_base>) -> FpOptionTag<new_base == 0>;
};

using FpDefaultOptionTag = FpOptionTag<>;

template <typename... Options>
concept FpValidOptions = requires(Options&&... options) {
  {(FpDefaultOptionTag() + ... + options)};
};

template <typename... Options>
  requires FpValidOptions<Options...>
struct FpOptionParser {
  using Tag = decltype((FpDefaultOptionTag() + ... + Options()));
};

template <typename T, typename... Options>
concept FpSupported = std::numeric_limits<T>::is_iec559 &&
                      (std::numeric_limits<T>::digits < 63) &&
                      FpValidOptions<Options...>;

template <typename T>
class FpSignApply {
 public:
  constexpr FpSignApply(bool negative = false) noexcept
      : negative_(negative), sign_zero_(negative ? -T(0) : T(0)) {}
  constexpr FpSignApply& operator=(bool negative) noexcept {
    negative_ = negative;
    sign_zero_ = negative ? -T(0) : T(0);
    return *this;
  }
  constexpr T operator()(T val) noexcept {
    if !consteval {
#ifdef __AVX__
      if constexpr (std::is_same_v<T, float>) {
        asm ("vxorps %2, %1, %0" : "=x"(val) : "x"(val), "x"(sign_zero_));
        return val;
      } else if constexpr (std::is_same_v<T, double>) {
        asm ("vxorpd %2, %1, %0" : "=x"(val) : "x"(val), "x"(sign_zero_));
        return val;
      }
#endif
    }
    if (negative_) val = -val;
    return val;
  }

 private:
  bool negative_;
  T sign_zero_;
};

template <std::floating_point T, bool partial, typename Tag>
constexpr auto str_to_fp(const char* s, const char* e) noexcept {
  auto make_ret = [&](std::optional<T> v) constexpr noexcept {
    if constexpr (partial)
      return StrToNumberPartialResult<T>{v, s};
    else
      return v;
  };
  auto make_bad_ret = [&] constexpr noexcept { return make_ret(std::nullopt); };

  if (s >= e) [[unlikely]]
    return make_bad_ret();

  FpSignApply<T> sign;
  if (s < e && (*s == '+' || *s == '-')) sign = (*s++ == '-');

  // Handle inf and nan
  if (s < e && std::uintptr_t(e - s) >= 3) {
    std::uint32_t vl = std::uint8_t(s[0]) | (std::uint8_t(s[1]) << 8) |
                       (std::uint8_t(s[2]) << 16) | 0x202020;
    const std::uint32_t kInf = 'i' | ('n' << 8) | ('f' << 16);
    const std::uint32_t kNaN = 'n' | ('a' << 8) | ('n' << 16);
    if (vl == kInf || vl == kNaN) {
      T val = (vl == kInf) ? INFINITY : NAN;
      s += 3;
      if constexpr (!partial) {
        if (s != e) return make_bad_ret();
      }
      return make_ret(sign(val));
    }
  }

  using UT = std::conditional_t<(std::numeric_limits<T>::digits < 31),
                                std::uint32_t, std::uint64_t>;
  using ST = std::make_signed_t<UT>;
  constexpr UT kDecMulLimit = (std::numeric_limits<ST>::max() - 9) / 10;
  constexpr UT kHexMulLimit = (std::numeric_limits<ST>::max() - 15) / 16;

  // Handle hex
  if constexpr (Tag::supports_hex) {
    if (s < e && s + 1 < e && *s == '0' && (s[1] == 'x' || s[1] == 'X')) {
      s += 2;

      // The next character should either be an xdigit, or a "." followed by an
      // xdigit.
      if (s < e && isxdigit(*s))
        ;
      else if (s < e && *s == '.' && (s + 1 < e) && isxdigit(s[1]))
        ;
      else
        return make_bad_ret();

      UT u = 0;
      std::optional<unsigned> xdigit_opt;
      while (u <= kHexMulLimit && s < e &&
             (xdigit_opt = parse_one_digit<16>(*s))) {
        ++s;
        u = u * 16 + *xdigit_opt;
      }

      int xpow2 = 0;
      while (s < e && isxdigit(*s)) {
        xpow2 += 4;
        ++s;
      }

      if (s < e && *s == '.') {
        ++s;
        while (u <= kHexMulLimit && s < e &&
               (xdigit_opt = parse_one_digit<16>(*s))) {
          u = u * 16 + *xdigit_opt;
          ++s;
          xpow2 -= 4;
        }

        while (s < e && isxdigit(*s)) ++s;
      }

      // For hexadecimal floating-point values, the exponential part is
      // mandatory.
      if (s >= e || (*s != 'p' && *s != 'P')) [[unlikely]]
        return make_bad_ret();
      ++s;

      unsigned exp_sign = 0;
      if (s < e && (*s == '+' || *s == '-')) exp_sign = (*s++ == '-') ? -1u : 0;
      if (s >= e || !isdigit(*s)) [[unlikely]]
        return make_bad_ret();

      unsigned uexp = (*s++ - '0');
      while (uexp <= std::numeric_limits<unsigned>::max() / 16 && s < e &&
             isdigit(*s)) {
        uexp = uexp * 10 + (*s++ - '0');
      }
      while (s < e && isdigit(*s)) ++s;

      if constexpr (!partial) {
        if (s != e) return make_bad_ret();
      }

      xpow2 += (uexp ^ exp_sign) - exp_sign;

      T res = sign(ST(u));

      // This check is necessary to avoid 0 * inf = nan
      if (u != 0 && xpow2 != 0) {
        // Reduce xpow2 to closer to 0 to prevent 2 ** xpow2 from overflow.
        // We don't need to use a while loop - if we have to do this more than
        // once, the final result definitely would be 0, even taking into
        // account of denormal numbers.
        // Also, we don't need to check for large xpow2 vlaues, because we
        // multiply the result to an integer - if 2**xpow2 would overflow then
        // the final result would too.
        if (xpow2 < std::numeric_limits<T>::min_exponent - 1) {
          res *= powu(T(.5), -(std::numeric_limits<T>::min_exponent - 1));
          xpow2 -= std::numeric_limits<T>::min_exponent - 1;
        }

        res *= pow2_daz<T>(xpow2);
      }
      return make_ret(res);
    }
  }

  // Normal path: decimal
  // The next character should either be a digit, or a "." followed by a digit.
  if (s < e && isdigit(*s))
    ;
  else if (s < e && *s == '.' && (s + 1 < e) && isdigit(s[1]))
    ;
  else
    return make_bad_ret();

  UT u = 0;

  while (u <= kDecMulLimit && s < e && isdigit(*s)) u = u * 10 + (*s++ - '0');

  int xpow10 = 0;
  while (s < e && isdigit(*s)) {
    ++xpow10;
    ++s;
  }

  if (s < e && *s == '.') {
    ++s;

    while (u <= kDecMulLimit && s < e && isdigit(*s)) {
      u = u * 10 + (*s++ - '0');
      --xpow10;
    }

    while (s < e && isdigit(*s)) ++s;
  }

  if (s < e && (*s == 'e' || *s == 'E')) {
    ++s;

    unsigned exp_sign = 0;
    if (s < e && (*s == '+' || *s == '-')) exp_sign = (*s++ == '-') ? -1u : 0;
    if (s >= e || !isdigit(*s)) [[unlikely]]
      return make_bad_ret();

    unsigned uexp = (*s++ - '0');
    while (uexp <= std::numeric_limits<unsigned>::max() / 16 && s < e &&
           isdigit(*s)) {
      uexp = uexp * 10 + (*s++ - '0');
    }
    while (s < e && isdigit(*s)) ++s;

    xpow10 += (uexp ^ exp_sign) - exp_sign;
  }

  if constexpr (!partial) {
    if (s != e) return make_bad_ret();
  }

  int xpow2 = xpow10;
  int xpow5 = xpow10;

  // Try to reduce the exponent of 5, helping to reduce round errors.
  if (xpow5 < 0) {
    while (xpow5 < 0 && (u % 5 == 0)) {
      u /= 5;
      ++xpow5;
    }
  } else {
    while (xpow5 && (u <= (std::numeric_limits<ST>::max() / 5))) {
      u *= 5;
      --xpow5;
    }
  }

  T res = sign(ST(u));

  // This check is necessary to avoid 0 * inf = nan
  if (u != 0) {
    res *= pow2_daz<T>(xpow2);

    if (xpow5 != 0) {
      unsigned xp = (xpow5 >= 0 ? xpow5 : -xpow5);
      T r = powu(T(5), xp);
      if (xpow5 >= 0)
        res *= r;
      else
        res /= r;
    }
  }

  return make_ret(res);
}

}  // namespace str_to_number_detail
}  // namespace cbu
