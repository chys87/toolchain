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

#include "cbu/storage/string_pack.h"

#if __has_include(<x86intrin.h>)
#include <x86intrin.h>
#endif

#include "cbu/common/bit.h"
#include "cbu/common/faststr.h"
#include "cbu/storage/vlq.h"

namespace cbu {
namespace string_pack_detail {

std::optional<ConsumeOneString<std::string_view>> consume_one_string(
    const char* p, const char* e) noexcept {
  auto dv = vlq_decode(p, e);
  if (!dv) return std::nullopt;
  p += dv->consumed_bytes;
  if (std::size_t(e - p) < dv->value) return std::nullopt;
  return ConsumeOneString<std::string_view>{{p, std::size_t(dv->value)},
                                            p + dv->value};
}

std::optional<ConsumeOneString<zstring_view>> consume_one_string_z(
    const char* p, const char* e) noexcept {
  auto dv = vlq_decode(p, e);
  if (!dv) return std::nullopt;
  p += dv->consumed_bytes;
  if (std::size_t(e - p) <= dv->value) return std::nullopt;
  if (p[dv->value] != '\0') return std::nullopt;
  return ConsumeOneString<zstring_view>{{p, std::size_t(dv->value)},
                                        p + dv->value + 1};
}

ConsumeOneString<std::string_view> consume_one_string_unsafe(
    const char* p) noexcept {
  auto dv = vlq_decode_unsafe(p);
  p += dv.consumed_bytes;
  return {{p, std::size_t(dv.value)}, p + dv.value};
}

ConsumeOneString<zstring_view> consume_one_string_z_unsafe(
    const char* p) noexcept {
  auto dv = vlq_decode_unsafe(p);
  p += dv.consumed_bytes;
  return {{p, std::size_t(dv.value)}, p + dv.value + 1};
}

}  // namespace string_pack_detail

void StringPack::add(std::string_view s){
  std::size_t l = s.size();
  EncodedVlq ev = vlq_encode_small(l);
  append(&s_, {ev.as_string_view(), s});
}

bool StringPack::verify(std::string_view s) noexcept {
  const char* p = s.data();
  const char* e = s.data() + s.size();
  while (p < e) {
    auto cos = string_pack_detail::consume_one_string(p, e);
    if (!cos) return false;
    p = cos->p;
  }
  return true;
}

void StringPackZ::add(std::string_view s) {
  std::size_t l = s.size();
  EncodedVlq ev = vlq_encode_small(l);
  std::size_t total_l = l + ev.l + 1;

  char* w = extend(&s_, total_l);
  w = Mempcpy(w, ev.buffer + ev.from, ev.l);
  w = Mempcpy(w, s.data(), l);
  *w = '\0';
}

bool StringPackZ::verify(std::string_view s) noexcept {
  const char* p = s.data();
  const char* e = s.data() + s.size();
  while (p < e) {
    auto cos = string_pack_detail::consume_one_string_z(p, e);
    if (!cos) return false;
    p = cos->p;
  }
  return true;
}

void StringPackCompactEncoderBase::add_compacted(
    std::string_view compact_info, std::string_view compacted_bytes) {
  EncodedVlq ev = vlq_encode_small(compacted_bytes.size());
  std::size_t total_l = compact_info.size() + ev.l + compacted_bytes.size();
  char* w = extend(&s_, total_l);
  w = Mempcpy(w, compact_info.data(), compact_info.size());
  w = Mempcpy(w, ev.buffer + ev.from, ev.l);
  Mempcpy(w, compacted_bytes.data(), compacted_bytes.size());
}

namespace {

std::size_t common_prefix_max_31(std::string_view a, std::string_view b) {
  const char* pa = a.data();
  const char* pb = b.data();
  std::size_t l = std::min(a.size(), b.size());

#if defined __AVX2__
  if (l >= 32) {
    __m256i eq = _mm256_cmpeq_epi8(*reinterpret_cast<const __m256i_u*>(pa),
                                   *reinterpret_cast<const __m256i_u*>(pb));
    std::uint32_t mask = ~_mm256_movemask_epi8(eq);
    if (mask == 0) return 31;
    return ctz(mask);
  }
#endif

  std::size_t prefix = 0;
  while (prefix < 31 && prefix < l && *pa++ == *pb++) ++prefix;
  return prefix;
}

std::size_t common_suffix_max_7(std::string_view a, std::string_view b) {
  const char* pa = a.data() + a.size() - 1;
  const char* pb = b.data() + b.size() - 1;
  std::size_t l = std::min(a.size(), b.size());

#if defined __SSE4_1__
  if (l >= 8) {
    __m128i eq = _mm_cmpeq_epi8(_mm_cvtsi64x_si128(mempick8(pa - 7)),
                                _mm_cvtsi64x_si128(mempick8(pb - 7)));
    std::uint32_t mask = std::uint16_t(~_mm_movemask_epi8(eq));
    if (mask == 0) return 7;
    return 32 - clz(mask);
  }
#endif

  std::size_t suffix = 0;
  while (suffix < 31 && suffix < l && *pa-- == *pb--) ++suffix;
  return suffix;
}

}  // namespace

void CommonPrefixSuffixCodec::compact(std::string_view prev,
                                      std::string_view cur,
                                      CompactInfo* compact_info,
                                      std::string_view* compacted_bytes,
                                      std::string* scratch) {
  std::size_t prefix = common_prefix_max_31(prev, cur);;
  cur.remove_prefix(prefix);

  std::size_t suffix = common_suffix_max_7(prev, cur);
  cur.remove_suffix(suffix);

  compact_info->prefix_suffix = (prefix << 3) + suffix;
  *compacted_bytes = cur;
}

void CommonPrefixSuffixCodec::restore(CompactInfo compact_info,
                                      std::string_view prev,
                                      std::string_view compacted_bytes,
                                      std::string* out) {
  out->clear();

  if (auto prefix = compact_info.prefix_suffix >> 3)
    out->assign(prev.data(), std::min<std::size_t>(prev.size(), prefix));

  out->append(compacted_bytes.data(), compacted_bytes.size());

  if (auto suffix = compact_info.prefix_suffix & 7) {
    std::size_t l = std::min<std::size_t>(prev.size(), suffix);
    out->append(prev.data() + prev.size() - l, l);
  }
}

}  // namespace cbu
