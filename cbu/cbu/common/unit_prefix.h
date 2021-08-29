/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2021, chys <admin@CHYS.INFO>
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

#include "cbu/common/bit.h"
#include "cbu/common/short_string.h"

namespace cbu {

inline constexpr char kUnitPrefixes[]{'\0', 'K', 'M', 'G', 'T',
                                      'P',  'E', 'Z', 'Y'};
inline constexpr unsigned kUnitPrefixCount =
    static_cast<unsigned>(sizeof(kUnitPrefixes) / sizeof(kUnitPrefixes[0]));

// K, M, G, ...
constexpr cbu::short_string<1> get_si_prefix(unsigned idx) noexcept {
  cbu::short_string<1> res;
  res.buffer()[0] = kUnitPrefixes[idx];
  res.buffer()[1] = '\0';
  res.set_length(idx ? 1 : 0);
  return res;
}

// Ki, Mi, Gi, ...
constexpr cbu::short_string<2> get_iec_prefix(unsigned idx) noexcept {
  cbu::short_string<2> res;
  res.buffer()[0] = kUnitPrefixes[idx];
  res.buffer()[1] = 'i';
  res.buffer()[2] = '\0';
  res.set_length(idx ? 2 : 0);
  return res;
}

// For example, 1026 bytes ==> 1 KiB + 2 bytes ==> {1, 2, 1}
// 1050627 bytes ==> 1 MiB + 2 KiB + 3 bytes ==> {1, 2, 2}
struct SplitUnitPrefix {
  unsigned quot;
  unsigned rem;
  unsigned prefix_idx;
};

constexpr SplitUnitPrefix split_unit_prefix_1000(std::uint64_t value) noexcept {
  unsigned prefix_idx = 0;
  unsigned rem = 0;
  while (value >= 1000) {
    rem = value % 1000;
    value /= 1000;
    ++prefix_idx;
  }
  return {static_cast<unsigned>(value), static_cast<unsigned>(rem), prefix_idx};
}

constexpr SplitUnitPrefix split_unit_prefix_1024(std::uint64_t value) noexcept {
  if (value < 1024) return {static_cast<unsigned>(value), 0, 0};
  // a / 10 == a * 13 >> 7 for all a < 69. avoid including fastarith.h
  unsigned prefix_idx = (bsr(value) * 13) >> 7;
  return {static_cast<unsigned>(value >> (10 * prefix_idx)),
          static_cast<unsigned>(value >> (10 * prefix_idx - 10)) & 1023,
          prefix_idx};
}

}  // namespace cbu
