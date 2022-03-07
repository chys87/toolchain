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

#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

#include "cbu/common/tags.h"

namespace cbu {

template <typename T>
struct StrToIntegerPartialResult {
  std::optional<T> value_opt;
  const char* endptr;
};

namespace str_to_integer_detail {

template <int base_, bool check_overflow_, bool halt_on_overflow_,
          unsigned long long overflow_threshold_>
struct OptionTag {
  static constexpr int base = base_;
  static constexpr bool check_overflow = check_overflow_;
  static constexpr bool halt_on_overflow = halt_on_overflow_;
  static constexpr unsigned long long overflow_threshold = overflow_threshold_;

  template <int new_base>
    requires(new_base == 0 || (new_base >= 2 && new_base <= 36))
  auto operator+(RadixTag<new_base>)
      -> OptionTag<new_base, check_overflow, halt_on_overflow,
                   overflow_threshold>;

  auto operator+(IgnoreOverflowTag)
      -> OptionTag<base, false, halt_on_overflow, overflow_threshold>;

  auto operator+(HaltScanOnOverflowTag)
      -> OptionTag<base, check_overflow, true, overflow_threshold>;

  template <unsigned long long threshold>
  auto operator+(OverflowThresholdTag<threshold>)
      -> OptionTag<base, check_overflow, halt_on_overflow, threshold>;
};

using DefaultOptionTag = OptionTag<10, true, false, ~0ull>;

template <typename... Options>
concept ValidOptions = requires(Options&&... options) {
  {(DefaultOptionTag() + ... + options)};
};

template <typename... Options>
  requires ValidOptions<Options...>
struct OptionParser {
  using Tag = decltype((DefaultOptionTag() + ... + Options()));
};

template <typename T, typename... Options>
concept Supported =
    std::integral<T> && sizeof(T) <= 8 && ValidOptions<Options...>;

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
inline constexpr auto extract_value(StrToIntegerPartialResult<T> v) noexcept {
  return v.value_opt;
}

template <typename U, typename T>
inline constexpr auto replace_value(std::optional<U>, T v) noexcept {
  return v;
}

template <typename U, typename T>
inline constexpr StrToIntegerPartialResult<T> replace_value(
    StrToIntegerPartialResult<U> r, std::optional<T> v) noexcept {
  return {v, r.endptr};
}

template <int base>
inline constexpr std::optional<unsigned> parse_one_digit(
    std::uint8_t c) noexcept {
  if constexpr (base <= 10) {
    if (c >= '0' && c < '0' + base) return c - '0';
  } else {
    if (c >= '0' && c <= '9')
      return c - '0';
    if ((c | 0x20) >= 'a' && (c | 0x20) < 'a' + (base - 10))
      return (((c | 0x20) - 'a') + 10);
  }
  return std::nullopt;
}

// Essentially the same as add_overflow in fastarith.h, but
// only supports unsigned integers, and we choose not to include fastarith.h
template <std::unsigned_integral T>
inline constexpr bool add_overflow(T a, T b, T* c) noexcept {
  if (std::is_constant_evaluated()) {
    *c = a + b;
    return (*c < a || *c < b);
  }
  return __builtin_add_overflow(a, b, c);
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
  auto make_ret = [&](std::optional<T> v) {
    if constexpr (partial)
      return StrToIntegerPartialResult<T>{v, s};
    else
      return v;
  };
  auto make_bad_ret = [&] { return make_ret(std::nullopt); };

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
      ++s;
      if ((*s | 0x20) == 'x')
        return str_to_integer<T, partial, decltype(Tag() + HexTag())>(s + 1, e);
      else
        return str_to_integer<T, partial, decltype(Tag() + OctTag())>(
            s + ((*s | 0x20) == 'o'), e);
    } else {
      return str_to_integer<T, partial, decltype(Tag() + DecTag())>(s, e);
    }
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

}  // namespace str_to_integer_detail
}  // namespace cbu
