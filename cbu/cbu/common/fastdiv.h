/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019, chys <admin@CHYS.INFO>
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

namespace cbu {
inline namespace cbu_fastdiv {
namespace fastdiv_detail {

// It's strange that there's no feature test macro for consteval, so
// we resort to GCC versions
#ifndef cbu_consteval
# if defined __GNUC__ && __GNUC__ >= 10
#  define cbu_consteval consteval
# else
#  define cbu_consteval constexpr
# endif
#endif

struct Magic {
  // (v / D) == ((v >> S) * M) >> N;
  unsigned int S;
  std::uint32_t M;
  unsigned int N;
};


template <typename Type>
inline cbu_consteval bool check(Type K, Type D, Type M, Type N) noexcept {
  return (K / D == K * M >> N);
}

template <typename Type>
inline cbu_consteval Type get_k(Type D, Type M, Type N) noexcept {
  Type lo = 0; // Satisfies
  Type hi = std::numeric_limits<Type>::max(); // Dissatisfies
  do {
    while (hi - lo > 1) {
      Type mid = lo + (hi - lo) / 2;
      if (check(mid, D, M, N)) {
        lo = mid;
      } else {
        hi = mid;
      }
    }
    Type CM = (D < 65536) ? 65536 : D;
    Type K = (lo / CM > D) ? (lo - CM * D) : 0;
    for (;;) {
      // We know (K/D == (K*M)>>N).
      // Find the next one that may differ.
      Type Ja = K - K % D + D;
      Type U = (K * M + (Type(1) << N)) & ~((Type(1) << N) - 1);
      Type Jb = (U + M - 1) / M;
      if ((Ja < K) || (Jb < K)) // Overflow
        break;
      if (Ja != Jb) {
        Type newhi = (Ja < Jb) ? Ja : Jb;
        if (newhi != hi) {
          hi = newhi;
          lo = 0;
        }
        break;
      }
      K = Ja;
    }
  } while (hi - lo > 1);
  return hi;
}

inline cbu_consteval Magic magic_base(std::uint32_t D,
                                      std::uint32_t UB) noexcept {
  for (unsigned int N = 1; N < 32; ++N) {
    // Find proper M, use this:
    // (D-1) / D == ((D - 1) * M) >> N == 0
    // D / D == (D * M) >> N == 1
    std::uint32_t M = (std::uint64_t(1u << N) + D - 1) / D;
    if (M % 2 == 0) {
      continue;
    }
    if ((D - 1) * M < (1u << N) && (1u << N) <= std::uint64_t(D) * M) {
      // Find proper K
      unsigned K = get_k<std::uint32_t>(D, M, N);
      if (K >= UB) {
        return {0, M, N};
      }
    }
  }
  return {};
}

inline cbu_consteval Magic magic(std::uint32_t D, std::uint32_t UB) noexcept {
  for (unsigned int S = 0; S < 32 && (D & ((1u << S) - 1)) == 0; ++S) {
    Magic mag = magic_base(D >> S, UB);
    if (mag.M != 0) {
      mag.S = S;
      return mag;
    }
  }

  return {};
}

} // namespace fastdiv_detail

// Compute (v / D) (where v is unknown to be < UB)
// We use tricks to have the compiler generate faster and/or smaller code
template <std::uint32_t D, std::uint32_t UB>
inline constexpr std::uint32_t fastdiv(std::uint32_t v) noexcept {
  constexpr auto mag = fastdiv_detail::magic(D, UB);
  constexpr auto S = mag.S;
  constexpr auto M = mag.M;
  constexpr auto N = mag.N;
  if constexpr (M != 0) {
    return (v >> S) * M >> N;
  } else if constexpr (D % 7 == 0 && ((D / 7) & ((D / 7) - 1)) == 0 &&
                       UB / (D / 7) <= 65536) {
    // Special case
    v /= D / 7;
    std::uint32_t a = 9363 * v >> 16;
    return ((a + ((v - a) >> 1)) >> 2);
  } else {
    return (v / D);
  }
}

template <std::uint32_t D, std::uint32_t UB>
inline constexpr std::uint32_t fastmod(std::uint32_t v) noexcept {
  return (v - fastdiv<D, UB>(v) * D);
}

template <std::uint32_t D, std::uint32_t UB>
inline constexpr std::pair<std::uint32_t, std::uint32_t>
    fastdivmod(std::uint32_t v) noexcept {
  auto quo = fastdiv<D, UB>(v);
  return {quo, v - quo * D};
}

} // namespace cbu_fastdiv
} // namespace cbu
