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

// Implements VLQ (variable-length quantity).
// VLQ is the big-endian equivalent of LEB128.
//
// Currently only the unsigned version is implemented.

#if (defined __i386__ || defined __x86_64__) && __has_include(<x86intrin.h>)
#include <x86intrin.h>
#endif

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

#include "cbu/common/bit.h"
#include "cbu/common/byteorder.h"
#include "cbu/math/fastdiv.h"
#include "cbu/strings/faststr.h"

namespace cbu {

struct EncodedVlq {
  std::uint8_t from;
  std::uint8_t l;
  char buffer[sizeof(std::uint64_t) * 8 / 7 + 1];

  constexpr std::string_view as_string_view() const noexcept {
    return {buffer + from, l};
  }

  constexpr std::span<const char> as_span() const noexcept {
    return {buffer + from, l};
  }
};

constexpr unsigned vlq_encode_length(std::uint64_t v) noexcept {
  if (v == 0) return 1;
  unsigned k = 64 - cbu::clz(v);
  return fastdiv<7, 69>(k + 6);
}

constexpr EncodedVlq vlq_encode(uint64_t v) noexcept {
  EncodedVlq res{};
  std::size_t k = sizeof(res.buffer);
  res.buffer[--k] = (v & 0x7f);
  while (v >>= 7) res.buffer[--k] = (v & 0x7f) | 0x80;
  res.from = k;
  res.l = sizeof(res.buffer) - k;
  return res;
}

// Optimized version if the input is known to be smaller than 2**56
constexpr EncodedVlq vlq_encode_small(uint64_t v) noexcept {
#ifdef __BMI2__
  if !consteval {
    EncodedVlq res{};
    v = bswap_be(_pdep_u64(v, 0x7f7f7f7f7f7f7f7f));
    cbu::memdrop(res.buffer + sizeof(res.buffer) - 8,
                 v | bswap_be(std::uint64_t(0x8080808080808000)));
    unsigned k = v ? cbu::ctz(v) / 8 : 7;
    res.from = k + sizeof(res.buffer) - 8;
    res.l = 8 - k;
    return res;
  }
#endif
  return vlq_encode(v);
}

struct DecodedVlq {
  std::uint64_t value;
  std::size_t consumed_bytes;
};

constexpr std::optional<DecodedVlq> vlq_decode(const char* s,
                                               const char* e) noexcept {
  uint64_t value = 0;
  const char* p = s;

  for (;;) {
    if (p >= e) return std::nullopt;
    std::uint8_t byte = *p++;
    if ((value << 7 >> 7) != value) return std::nullopt;
    value = (value << 7) | (byte & 0x7f);
    if ((byte & 0x80) == 0) break;
  }

  return DecodedVlq{value, std::size_t(p - s)};
}

constexpr std::optional<DecodedVlq> vlq_decode(
    std::span<const char> bytes) noexcept {
  return vlq_decode(bytes.data(), bytes.data() + bytes.size());
}

constexpr DecodedVlq vlq_decode_unsafe(const char* s) noexcept {
  uint64_t value = 0;
  const char* p = s;

  for (;;) {
    std::uint8_t byte = *p++;
    value = (value << 7) | (byte & 0x7f);
    if ((byte & 0x80) == 0) break;
  }

  return {value, std::size_t(p - s)};
}

}  // namespace cbu
