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
#include <string_view>

#include "cbu/common/bit.h"

namespace cbu {

inline constexpr char kUnitPrefixes[]{'\0', 'K', 'M', 'G', 'T',
                                      'P',  'E', 'Z', 'Y'};

inline constexpr char kIecUnitPrefixes[][2]{{},         {'K', 'i'}, {'M', 'i'},
                                            {'G', 'i'}, {'T', 'i'}, {'P', 'i'},
                                            {'E', 'i'}, {'Z', 'i'}, {'Y', 'i'}};

inline constexpr unsigned kUnitPrefixCount =
    static_cast<unsigned>(sizeof(kUnitPrefixes) / sizeof(kUnitPrefixes[0]));

// K, M, G, ...
constexpr std::string_view get_si_prefix(unsigned idx) noexcept {
  return {kUnitPrefixes + idx, std::size_t(idx ? 1 : 0)};
}

// Ki, Mi, Gi, ...
constexpr std::string_view get_iec_prefix(unsigned idx) noexcept {
  return {kIecUnitPrefixes[idx], std::size_t(idx ? 2 : 0)};
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

// For example, 1026 bytes ==> {1260.f / 1024, 1}
// 1050627 bytes ==> {1050627.f / 1024 / 1024, 2}
struct ScaleUnitPrefix {
  float value;
  unsigned prefix_idx;
};

// Input value should be a non-negative value in range [0, 2**63]
// We don't detect out-of-range values for sake of performance.
[[gnu::const]] ScaleUnitPrefix scale_unit_prefix_1000(float value) noexcept;
[[gnu::const]] ScaleUnitPrefix scale_unit_prefix_1024(float value) noexcept;

}  // namespace cbu
