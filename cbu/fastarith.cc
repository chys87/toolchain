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

#include "fastarith.h"
#include <cmath>
#include <limits>

namespace cbu {
inline namespace cbu_fastarith {


namespace {

float scale(float a, int n) { return std::scalbnf(a, n); }
double scale(double a, int n) { return std::scalbn(a, n); }

inline constexpr unsigned clz(unsigned x) noexcept {
  return __builtin_clz(x);
}

inline constexpr unsigned clz(unsigned long x) noexcept {
  return __builtin_clzl(x);
}

inline constexpr unsigned clz(unsigned long long x) noexcept {
  return __builtin_clzll(x);
}

template <typename F, typename U>
requires (std::is_floating_point<F>::value &&
          std::numeric_limits<F>::radix == 2 &&
          std::numeric_limits<U>::digits > std::numeric_limits<F>::digits)
inline F map_uint_to_float(U u) noexcept {
  constexpr int MANTISSA_BITS = std::numeric_limits<F>::digits;
  constexpr int UBITS = std::numeric_limits<U>::digits;

  // We want F(u) * scale(F(1), -UBITS), with the multiplication rounding
  // toward zero or minus infinity.
  // Changing rounding mode is expensive, so we clear extra bits before
  // proceeding
  //
  // NOTE: This method gives more precission (for small values) than just
  // clearing the lowest 8 bits of u.
  // Consider u = 0x1ff
  unsigned lz = clz(u);

  // If lz == UBITS, the shift has undefined behavior.
  // However, in that case we must have u == 0.
  // (It really should be implementatin-defined behavior.  Fuck the Standards!)
  u &= ~(~U(0) >> MANTISSA_BITS >> lz);

  // Now we have guarantee that both the conversion and multiplication are exact
  return F(u) * scale(F(1), -UBITS);
}

} // namespace

float map_uint32_to_float(uint32_t v) noexcept {
  return map_uint_to_float<float, uint32_t>(v);
}

double map_uint64_to_double(uint64_t v) noexcept {
  return map_uint_to_float<double, uint64_t>(v);
}

} // inline namespace cbu_fastarith
} // namespace cbu
