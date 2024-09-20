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
#include <iterator>
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

template <std::sentinel_for<const char*> S>
struct normalize_sentinel_type {
  using type = S;
};

template <>
struct normalize_sentinel_type<char*> {
  using type = const char*;
};

template <std::sentinel_for<const char*>S>
using normalize_sentinel_t = typename normalize_sentinel_type<S>::type;

struct IntegerOptions {
  int base = 10;
  bool check_overflow = true;
  bool halt_on_overflow = false;
  unsigned long long overflow_threshold = ~0ull;

  template <int new_base>
    requires(new_base == 0 || (new_base >= 2 && new_base <= 36))
  constexpr IntegerOptions operator+(RadixTag<new_base>) const noexcept {
    auto res = *this;
    res.base = new_base;
    return res;
  }

  constexpr IntegerOptions operator+(IgnoreOverflowTag) const noexcept {
    auto res = *this;
    res.check_overflow = false;
    return res;
  }

  constexpr IntegerOptions operator+(HaltScanOnOverflowTag) const noexcept {
    auto res = *this;
    res.halt_on_overflow = true;
    return res;
  }

  template <unsigned long long threshold>
  constexpr IntegerOptions operator+(
      OverflowThresholdTag<threshold>) const noexcept {
    auto res = *this;
    res.overflow_threshold = threshold;
    return res;
  }
};

template <typename... Options>
concept IntegerValidOptions = requires(Options&&... options) {
  { (IntegerOptions() + ... + options) };
};

template <typename... Options>
  requires IntegerValidOptions<Options...>
inline constexpr IntegerOptions parse_integer_options() noexcept {
  return (IntegerOptions() + ... + Options());
}

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
  requires(base <= 10)
inline constexpr std::optional<unsigned> parse_digit(std::uint8_t c) noexcept {
  if (unsigned C = c - '0'; C < base) return C;
  return std::nullopt;
}

template <int base>
  requires(base > 10)
inline constexpr std::optional<unsigned> parse_digit(std::uint8_t c) noexcept {
  if (unsigned C = c - '0'; C < 10) return C;
  if (unsigned C = (c | 0x20) - 'a'; C < base - 10) return C + 10;
  return std::nullopt;
}

template <std::floating_point T>
inline constexpr T powu(T a, unsigned t,
                        std::type_identity_t<T> r = 1) noexcept {
  while (t) {
    if (t & 1) r *= a;
    t >>= 1;
    a *= a;
  }
  return r;
}

// "daz" stands for "denormal as zero"
template <std::floating_point T>
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

template <std::size_t N>
inline constexpr bool distance_at_least(const char* s, const char* e) noexcept {
  return std::size_t(e - s) >= N;
}

template <std::size_t N, std::sentinel_for<const char*> S>
  requires(!std::is_same_v<S, const char*>)
inline constexpr bool distance_at_least(const char* s, S e) noexcept {
  for (std::size_t n = N; n; --n) {
    if (s == e) return false;
    ++s;
  }
  return true;
}

template <std::unsigned_integral T, bool partial, IntegerOptions OPT,
          std::sentinel_for<const char*> S>
  requires(OPT.base != 0)
constexpr auto str_to_integer(const char* s, S e) noexcept {
  auto make_ret = [&](std::optional<T> v = std::nullopt) constexpr noexcept {
    if constexpr (partial)
      return StrToNumberPartialResult<T>{v, s};
    else
      return v;
  };

  if (s == e) [[unlikely]]
    return make_ret();

  bool parsed_any_char = false;
  T val = 0;

  while (s != e) {
    auto digit_opt = parse_digit<OPT.base>(*s);
    if (!digit_opt) {
      if (partial && parsed_any_char) break;
      return make_ret();
    }
    ++s;
    parsed_any_char = true;

    if constexpr (OPT.check_overflow) {
      auto new_val_opt = mul_base_add<T, OPT.base, OPT.overflow_threshold>(val, T(*digit_opt));
      if (!new_val_opt) [[unlikely]] {
        if constexpr (partial && !OPT.halt_on_overflow) {
          // In partial match, we need to consume all remaining digits, to
          // prevent surprise.
          while (s != e && parse_digit<OPT.base>(*s)) ++s;
        }
        return make_ret();
      }
      val = *new_val_opt;
    } else {
      val = val * OPT.base + *digit_opt;
    }
  }

  return make_ret(val);
};

template <std::unsigned_integral T, bool partial, IntegerOptions OPT,
          std::sentinel_for<const char*> S>
  requires(OPT.base == 0)
constexpr auto str_to_integer(const char* s, S e) noexcept {
  if (s != e && s + 1 != e && *s == '0') {
    if (s[1] == 'x' || s[1] == 'X')
      return str_to_integer<T, partial, OPT + HexTag()>(s + 2, e);
    else if ((s[1] == 'o' || s[1] == 'O') || isdigit(s[1]))
      return str_to_integer<T, partial, OPT + OctTag()>(s + 1 + (s[1] == 'o' || s[1] == 'O'), e);
    else if ((s[1] == 'b' || s[1] == 'B') || (s[1] == '0' || s[1] == '1'))
      return str_to_integer<T, partial, OPT + BinTag()>(s + 2, e);
  }
  return str_to_integer<T, partial, OPT + DecTag()>(s, e);
};

template <std::signed_integral T, bool partial, IntegerOptions OPT,
          std::sentinel_for<const char*> S>
constexpr auto str_to_integer(const char* s, S e) noexcept {
  auto make_ret = [&](std::optional<T> v) constexpr noexcept {
    if constexpr (partial)
      return StrToNumberPartialResult<T>{v, s};
    else
      return v;
  };
  auto make_bad_ret = [&] constexpr noexcept { return make_ret(std::nullopt); };

  if (s == e) [[unlikely]]
    return make_bad_ret();

  T sign = 0;
  if (*s == '+' || *s == '-') {
    if (*s == '-') sign = T(-1);
    ++s;
  }
  auto abs_ret = str_to_integer<std::make_unsigned_t<T>, partial, OPT>(s, e);
  auto abs_opt_val = extract_value(abs_ret);
  if (!abs_opt_val) [[unlikely]]
    return replace_value(abs_ret, std::optional<T>(std::nullopt));
  T real_val = (*abs_opt_val ^ sign) - sign;
  if constexpr (OPT.check_overflow && OPT.overflow_threshold > std::numeric_limits<T>::max()) {
    // Test overflow after adding sign, whether real_val and sign has the
    // same sign bit.
    if (T(real_val ^ sign) < T(0)) [[unlikely]]
      return replace_value(abs_ret, std::optional<T>(std::nullopt));
  }
  return replace_value(abs_ret, std::optional(real_val));
};

struct FpOptions {
  bool supports_hex = false;
  bool parse_inf_nan = true;
  bool finite_only = false;
  // scientific only applies to decimal values; for hexadecimal values,
  // scientific notation is mandatory
  bool scientific = true;

  template <int new_base>
    requires(new_base == 0 || new_base == 10)
  constexpr FpOptions operator+(RadixTag<new_base>) const noexcept {
    auto res = *this;
    res.supports_hex = (new_base == 0);
    return res;
  }

  constexpr FpOptions operator+(IgnoreInfNaNTag) const noexcept {
    auto res = *this;
    res.parse_inf_nan = false;
    return res;
  }

  constexpr FpOptions operator+(FiniteOnlyTag) const noexcept {
    auto res = *this;
    res.parse_inf_nan = false;
    res.finite_only = true;
    return res;
  }

  constexpr FpOptions operator+(NoScientificNotationTag) const noexcept {
    auto res = *this;
    res.scientific = false;
    return res;
  }
};

template <typename... Options>
concept FpValidOptions = requires(Options&&... options) {
  {(FpOptions() + ... + options)};
};

template <typename... Options>
  requires FpValidOptions<Options...>
inline constexpr FpOptions parse_fp_options() noexcept {
  return (FpOptions() + ... + Options());
}

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
  constexpr T operator()(T val) const noexcept {
    if !consteval {
#ifdef __AVX__
      if constexpr (std::is_same_v<T, float>) {
        asm ("vxorps %2, %1, %0" : "=x"(val) : "x"(val), "x"(sign_zero_));
        return val;
      } else if constexpr (std::is_same_v<T, double>) {
        asm ("vxorpd %2, %1, %0" : "=x"(val) : "x"(val), "x"(sign_zero_));
        return val;
      }
#elifdef __aarch64__
      if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
        asm ("eor %0.8b, %1.8b, %2.8b" : "=w"(val) : "w"(val), "w"(sign_zero_));
        return val;
      }
#endif
      return std::copysign(val, sign_zero_);
    }
    if (negative_) val = -val;
    return val;
  }

  constexpr T operator()() const noexcept { return sign_zero_; }

 private:
  bool negative_;
  T sign_zero_;
};

inline constexpr std::uint16_t pick2(const char* s) noexcept {
  std::uint16_t r = 0;
  if !consteval {
    __builtin_memcpy(&r, s, 2);
    return r;
  }
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  r = std::uint8_t(s[0]) | (std::uint8_t(s[1]) << 8);
#else
  r = std::uint8_t(s[1]) | (std::uint8_t(s[0]) << 8);
#endif
  return r;
}

inline constexpr std::uint32_t pick3(const char* s) noexcept {
  // The result is not big-endian on big-endian platforms. We only attempt to
  // provide consistent results on the same platform.
  return pick2(s) | (std::uint32_t(std::uint8_t(s[2])) << 16);
}

inline constexpr std::uint64_t pick8(const char* s) noexcept {
  std::uint64_t r = 0;
  if !consteval {
    __builtin_memcpy(&r, s, 8);
    return r;
  }
  for (int i = 0; i < 8; ++i) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    r |= std::uint64_t(std::uint8_t(s[i])) << (i * 8);
#else
    r |= std::uint64_t(std::uint8_t(s[i])) << ((7 - i) * 8);
#endif
  }
  return r;
}

template <std::floating_point T, bool partial, FpOptions OPT,
          std::sentinel_for<const char*> S>
constexpr auto str_to_fp(const char* s, S e) noexcept {
  auto make_ret = [&](std::optional<T> v) constexpr noexcept {
    if constexpr (partial)
      return StrToNumberPartialResult<T>{v, s};
    else
      return v;
  };
  auto make_bad_ret = [&] constexpr noexcept { return make_ret(std::nullopt); };

  if (s == e) [[unlikely]]
    return make_bad_ret();

  FpSignApply<T> sign;
  if (s != e && (*s == '+' || *s == '-')) sign = (*s++ == '-');

  // Handle inf and nan
  if constexpr (OPT.parse_inf_nan && !OPT.finite_only) {
    if (s != e && distance_at_least<3>(s, e)) {
      std::uint32_t vl = pick3(s) | 0x20202020;
      const std::uint32_t kInf = pick3("inf") | 0x20202020;
      const std::uint32_t kNaN = pick3("nan") | 0x20202020;
      if (vl == kInf || vl == kNaN) [[unlikely]] {
        T val = sign((vl == kInf) ? INFINITY : NAN);
        if (vl == kInf && distance_at_least<8>(s, e) &&
            (pick8(s) | 0x2020202020202020) == pick8("infinity")) {
          s += 8;
        } else {
          s += 3;
        }
        if (s != e) {
          if constexpr (partial) {
            if (std::uint8_t(*s - '0') < 10 ||
                std::uint8_t((*s | 0x20) - 'a') < 26)
              return make_bad_ret();
          } else {
            return make_bad_ret();
          }
        }
        return make_ret(val);
      }
    }
  }

  using UT = std::conditional_t<(std::numeric_limits<T>::digits < 31),
                                std::uint32_t, std::uint64_t>;
  using ST = std::make_signed_t<UT>;
  constexpr UT kDecMulLimit = (std::numeric_limits<ST>::max() - 9) / 10;
  constexpr UT kHexMulLimit = (std::numeric_limits<ST>::max() - 15) / 16;

  // Handle hex
  if constexpr (OPT.supports_hex) {
    if (s != e && s + 1 != e && *s == '0' && (s[1] == 'x' || s[1] == 'X')) {
      s += 2;

      // The next character should either be an xdigit, or a "." followed by an
      // xdigit.
      if (s != e && isxdigit(*s))
        ;
      else if (s != e && *s == '.' && (s + 1 != e) && isxdigit(s[1]))
        ;
      else
        return make_bad_ret();

      UT u = 0;
      std::optional<unsigned> xdigit_opt;
      while (u <= kHexMulLimit && s != e &&
             (xdigit_opt = parse_digit<16>(*s))) {
        ++s;
        u = u * 16 + *xdigit_opt;
      }

      int xpow2 = 0;
      while (s != e && isxdigit(*s)) {
        xpow2 += 4;
        ++s;
      }

      if (s != e && *s == '.') {
        ++s;
        while (u <= kHexMulLimit && s != e &&
               (xdigit_opt = parse_digit<16>(*s))) {
          u = u * 16 + *xdigit_opt;
          ++s;
          xpow2 -= 4;
        }

        while (s != e && isxdigit(*s)) ++s;
      }

      // For hexadecimal floating-point values, the exponential part is
      // mandatory.
      if (s == e || (*s != 'p' && *s != 'P')) [[unlikely]]
        return make_bad_ret();
      ++s;

      unsigned exp_sign = 0;
      if (s != e && (*s == '+' || *s == '-')) exp_sign = (*s++ == '-') ? -1u : 0;
      if (s == e || !isdigit(*s)) [[unlikely]]
        return make_bad_ret();

      unsigned uexp = (*s++ - '0');
      while (uexp <= std::numeric_limits<unsigned>::max() / 32 && s != e &&
             isdigit(*s)) {
        uexp = uexp * 10 + (*s++ - '0');
      }
      while (s != e && isdigit(*s)) ++s;

      if constexpr (!partial) {
        if (s != e) return make_bad_ret();
      }

      // Check for 0 early. It also avoids 0 * inf = nan
      if (u == 0) return make_ret(sign());

      xpow2 += (uexp ^ exp_sign) - exp_sign;

      T res = sign(ST(u));

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
      if constexpr (OPT.finite_only) {
        if (!std::isfinite(res)) return make_bad_ret();
      }
      return make_ret(res);
    }
  }

  // Normal path: decimal
  // The next character should either be a digit, or a "." followed by a digit.
  if (s != e && isdigit(*s))
    ;
  else if (s != e && *s == '.' && (s + 1 != e) && isdigit(s[1]))
    ;
  else
    return make_bad_ret();

  UT u = 0;

  while (u <= kDecMulLimit && s != e && isdigit(*s)) u = u * 10 + (*s++ - '0');

  int xpow10 = 0;
  while (s != e && isdigit(*s)) {
    ++xpow10;
    ++s;
  }

  if (s != e && *s == '.') {
    ++s;

    while (u <= kDecMulLimit && s != e && isdigit(*s)) {
      u = u * 10 + (*s++ - '0');
      --xpow10;
    }

    while (s != e && isdigit(*s)) ++s;
  }

  if constexpr (OPT.scientific) {
    if (s != e && (*s == 'e' || *s == 'E')) {
      ++s;

      unsigned exp_sign = 0;
      if (s != e && (*s == '+' || *s == '-'))
        exp_sign = (*s++ == '-') ? -1u : 0;
      if (s == e || !isdigit(*s)) [[unlikely]]
        return make_bad_ret();

      unsigned uexp = (*s++ - '0');
      while (uexp <= std::numeric_limits<unsigned>::max() / 32 && s != e &&
             isdigit(*s)) {
        uexp = uexp * 10 + (*s++ - '0');
      }
      while (s != e && isdigit(*s)) ++s;

      xpow10 += (uexp ^ exp_sign) - exp_sign;
    }
  }

  if constexpr (!partial) {
    if (s != e) return make_bad_ret();
  }

  // Check for 0 early, so that we don't get performance regressions in inputs
  // like "0e-1999999999999999999999999999".
  // It also avoids 0 * inf = nan
  if (u == 0) return make_ret(sign());

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

  res *= pow2_daz<T>(xpow2);

  if (xpow5 != 0) {
    unsigned xp = (xpow5 >= 0 ? xpow5 : -xpow5);
    T r = powu(T(5), xp);
    if (xpow5 >= 0)
      res *= r;
    else
      res /= r;
  }

  if constexpr (OPT.finite_only) {
    if (!std::isfinite(res)) return make_bad_ret();
  }
  return make_ret(res);
}

}  // namespace str_to_number_detail
}  // namespace cbu
