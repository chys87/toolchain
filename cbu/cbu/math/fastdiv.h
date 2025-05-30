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

#include <cstdint>
#include <limits>
#include <utility>

#include "cbu/common/hint.h"

namespace cbu {
namespace fastdiv_detail {

template <typename Type>
struct Magic {
  // (v / D) == ((v >> S) * M) >> N;
  unsigned int S;
  Type M;
  unsigned int N;
};


// Verify that for all k, 0 <= k < UB, k / D == k * M >> N
template <typename Type>
inline constexpr bool verify_magic(Type D, Type UB, Type M, Type N) noexcept {
  if (UB == 0) return false;
  Type t = UB - 1;
  if (t / D != t * M >> N) return false;
  t = t / D * D;
  if (t / D != t * M >> N) return false;
  if (t == 0) return true;
  if ((t - 1) / D != (t - 1) * M >> N) return false;
  if ((t - D) / D != (t - D) * M >> N) return false;
  t -= D;
  if (t == 0) return true;
  if ((t - 1) / D != (t - 1) * M >> N) return false;
  return true;
}

template <typename Type>
inline constexpr Magic<Type> magic_base(Type D, Type UB) noexcept {
  for (unsigned int N = 1; N < 8 * sizeof(Type); ++N) {
    // Find proper M, use this:
    // (D-1) / D == ((D - 1) * M) >> N == 0
    // D / D == (D * M) >> N == 1
    // Type M = (DoubleOf<Type>(Type(1) << N) + D - 1) / D;
    Type M = ((Type(1) << N) - 1) / D + 1;  // This won't overflow Type
    if (M % 2 == 0) {
      continue;
    }
    if (verify_magic<Type>(D, UB, M, N)) {
      return {0, M, N};
    }
  }
  return {};
}

template <typename Type>
inline consteval Magic<Type> magic(Type D, Type UB) noexcept {
  for (unsigned int S = 0;
       S < 8 * sizeof(Type) && (D & ((Type(1) << S) - 1)) == 0; ++S) {
    Magic mag = magic_base(D >> S, UB);
    if (mag.M != 0) {
      mag.S = S;
      return mag;
    }
  }

  return {};
}

// Work around a bug in some veresions of GCC, which insists on
// evaluating UB / (D / 7) even if it should be short-circuited.
template <std::uint32_t D, std::uint32_t UB> requires (D / 7 != 0)
inline constexpr bool use_div7_special_case() {
  return (D % 7 == 0 && ((D / 7) & ((D / 7) - 1)) == 0 &&
          UB / (D / 7) <= 65536);
}

template <std::uint32_t D, std::uint32_t UB> requires (D / 7 == 0)
inline constexpr bool use_div7_special_case() { return false; }

template <std::uint64_t D, std::uint64_t UB>
using FastDivType =
    std::conditional_t<D <= std::numeric_limits<std::uint32_t>::max() &&
                           UB <= std::numeric_limits<std::uint32_t>::max(),
                       std::uint32_t, std::uint64_t>;

// Compute (v / D) (where v is unknown to be < UB)
// We use tricks to have the compiler generate faster and/or smaller code
template <typename Type, Type D, Type UB, Magic<Type> Mag = magic(D, UB)>
inline constexpr Type fastdiv(Type v) noexcept {
  constexpr auto S = Mag.S;
  constexpr auto M = Mag.M;
  constexpr auto N = Mag.N;
  if constexpr (UB <= D) {
    // Special case
    return 0;
  } else if constexpr (sizeof(Type) == 4) {
    if constexpr (M != 0) {
      return (v >> S) * M >> N;
    } else if constexpr (fastdiv_detail::use_div7_special_case<D, UB>()) {
      // Special case
      v /= D / 7;
      std::uint32_t a = 9363 * v >> 16;
      return ((a + ((v - a) >> 1)) >> 2);
    } else {
      return (v / D);
    }
  } else {
    if constexpr (UB - 1 == std::numeric_limits<std::uint32_t>::max()) {
      // Special case for exact uint32_t
      return (std::uint32_t(v) / D);
    } else if constexpr (M != 0) {
      return (v >> S) * M >> N;
    } else {
      return (v / D);
    }
  }
}

}  // namespace fastdiv_detail

template <std::uint64_t D, std::uint64_t UB>
inline constexpr fastdiv_detail::FastDivType<D, UB> fastdiv(
    fastdiv_detail::FastDivType<D, UB> v) noexcept {
  CBU_HINT_ASSUME(v < UB);
  using Type = fastdiv_detail::FastDivType<D, UB>;
  if constexpr (UB <= D) {
    return 0;
  } else if constexpr (D == 1) {
    return v;
  } else if constexpr (UB == 256) {
    return std::uint8_t(v) / D;
  } else if constexpr (UB == 65536) {
    return std::uint16_t(v) / D;
  } else if constexpr (UB == 4294967296) {
    return std::uint32_t(v) / D;
  } else {
    Type r = fastdiv_detail::fastdiv<Type, static_cast<Type>(D),
                                     static_cast<Type>(UB)>(v);
    CBU_HINT_ASSUME(r <= (UB - 1) / D);
    return r;
  }
}

template <std::uint64_t D, std::uint64_t UB>
inline constexpr fastdiv_detail::FastDivType<D, UB> fastmod(
    fastdiv_detail::FastDivType<D, UB> v) noexcept {
  CBU_HINT_ASSUME(v < UB);
  using Type = fastdiv_detail::FastDivType<D, UB>;
  if constexpr (UB <= 1 || D == 1) {
    return 0;
  } else if constexpr (UB <= D) {
    return v;
  } else {
    auto r = v - Type(D) * fastdiv<D, UB>(v);
    CBU_HINT_ASSUME(r < D);
    return r;
  }
}

template <std::uint64_t D, std::uint64_t UB>
inline constexpr std::pair<fastdiv_detail::FastDivType<D, UB>,
                           fastdiv_detail::FastDivType<D, UB>>
fastdivmod(fastdiv_detail::FastDivType<D, UB> v) noexcept {
  CBU_HINT_ASSUME(v < UB);
  using Type = fastdiv_detail::FastDivType<D, UB>;
  if constexpr (UB <= 1) {
    return {0, 0};
  } else if constexpr (D == 1) {
    return {v, 0};
  } else if constexpr (UB <= D) {
    return {0, Type(v)};
  } else {
    Type quo = fastdiv<D, UB>(v);
    Type rem = v - quo * Type(D);
    CBU_HINT_ASSUME(rem < D);
    return {quo, rem};
  }
}

}  // namespace cbu
