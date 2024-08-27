/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2023, chys <admin@CHYS.INFO>
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

#include "cbu/common/unit_prefix.h"

#include <cstring>
#include <limits>

#include "cbu/math/fastdiv.h"

namespace cbu {

ScaleUnitPrefix scale_unit_prefix_1000(float value) noexcept {
  float scale = 1;
  unsigned prefix_idx = 0;
  while (value >= scale * 1000) {
    scale *= 1000;
    ++prefix_idx;
  }
  return {value / scale, prefix_idx};
}

ScaleUnitPrefix scale_unit_prefix_1024(float value) noexcept {
  if constexpr (std::numeric_limits<float>::is_iec559) {
    std::uint32_t u = std::bit_cast<uint32_t>(value);

    // Handle 0 and small numbers properly
    if (u < std::bit_cast<uint32_t>(1024.f)) return {value, 0};

    // Extract exponent
    // The value is known to be positive, so don't bother to clear sign bit
    unsigned orig_exponent = u >> 23;
    unsigned real_exponent = orig_exponent - 127;
    unsigned prefix_idx = cbu::fastdiv<10, 64>(real_exponent);
    u -= prefix_idx * 10 << 23;
    float res = std::bit_cast<float>(u);
    return {res, prefix_idx};
  } else {
    // Division by 1024 doesn't generate errors, so this could be further
    // optimized by avoiding division.
    // But we don't bother to, becuase mainsteam platforms all use IEEE 754
    float scale = 1;
    unsigned prefix_idx = 0;
    while (value >= scale * 1024) {
      scale *= 1024;
      ++prefix_idx;
    }
    return {value / scale, prefix_idx};
  }
}

}  // namespace cbu
