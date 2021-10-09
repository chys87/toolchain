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

#include "cbu/storage/vlq.h"

#include <gtest/gtest.h>

#include <string_view>

namespace cbu {
namespace {

using std::operator""sv;

TEST(VlqTest, Encode) {
  EXPECT_EQ(vlq_encode(0).as_string_view(), "\x00"sv);
  EXPECT_EQ(vlq_encode(127).as_string_view(), "\x7f"sv);
  EXPECT_EQ(vlq_encode(128).as_string_view(), "\x81\x00"sv);
  EXPECT_EQ(vlq_encode(8192).as_string_view(), "\xc0\x00"sv);
  EXPECT_EQ(vlq_encode(16383).as_string_view(), "\xff\x7f"sv);
  EXPECT_EQ(vlq_encode(16384).as_string_view(), "\x81\x80\x00"sv);
  EXPECT_EQ(vlq_encode(2097151).as_string_view(), "\xff\xff\x7f"sv);
  EXPECT_EQ(vlq_encode(2097152).as_string_view(), "\x81\x80\x80\x00"sv);
  EXPECT_EQ(vlq_encode(268435455).as_string_view(), "\xff\xff\xff\x7f"sv);
  EXPECT_EQ(vlq_encode((std::uint64_t(1) << 56) - 1).as_string_view(),
            "\xff\xff\xff\xff\xff\xff\xff\x7f"sv);
  EXPECT_EQ(vlq_encode(std::uint64_t(1) << 56).as_string_view(),
            "\x81\x80\x80\x80\x80\x80\x80\x80\x00"sv);
  EXPECT_EQ(vlq_encode(~std::uint64_t(0)).as_string_view(),
            "\x81\xff\xff\xff\xff\xff\xff\xff\xff\x7f"sv);
}

TEST(VlqTest, EncodeSmall) {
  auto encode_constexpr = [](std::uint64_t v) consteval noexcept {
    return vlq_encode_small(v);
  };
  auto encode_non_constexpr = [](std::uint64_t v) noexcept {
    volatile std::uint64_t V = v;
    return vlq_encode_small(V);
  };
  EXPECT_EQ(encode_constexpr(0).as_string_view(), "\x00"sv);
  EXPECT_EQ(encode_constexpr(127).as_string_view(), "\x7f"sv);
  EXPECT_EQ(encode_constexpr(128).as_string_view(), "\x81\x00"sv);
  EXPECT_EQ(encode_constexpr(8192).as_string_view(), "\xc0\x00"sv);
  EXPECT_EQ(encode_constexpr(16383).as_string_view(), "\xff\x7f"sv);
  EXPECT_EQ(encode_constexpr(16384).as_string_view(), "\x81\x80\x00"sv);
  EXPECT_EQ(encode_constexpr(2097151).as_string_view(), "\xff\xff\x7f"sv);
  EXPECT_EQ(encode_constexpr(2097152).as_string_view(), "\x81\x80\x80\x00"sv);
  EXPECT_EQ(encode_constexpr(268435455).as_string_view(), "\xff\xff\xff\x7f"sv);
  EXPECT_EQ(encode_constexpr((std::uint64_t(1) << 56) - 1).as_string_view(),
            "\xff\xff\xff\xff\xff\xff\xff\x7f"sv);

  EXPECT_EQ(encode_non_constexpr(0).as_string_view(), "\x00"sv);
  EXPECT_EQ(encode_non_constexpr(127).as_string_view(), "\x7f"sv);
  EXPECT_EQ(encode_non_constexpr(128).as_string_view(), "\x81\x00"sv);
  EXPECT_EQ(encode_non_constexpr(8192).as_string_view(), "\xc0\x00"sv);
  EXPECT_EQ(encode_non_constexpr(16383).as_string_view(), "\xff\x7f"sv);
  EXPECT_EQ(encode_non_constexpr(16384).as_string_view(), "\x81\x80\x00"sv);
  EXPECT_EQ(encode_non_constexpr(2097151).as_string_view(), "\xff\xff\x7f"sv);
  EXPECT_EQ(encode_non_constexpr(2097152).as_string_view(),
            "\x81\x80\x80\x00"sv);
  EXPECT_EQ(encode_non_constexpr(268435455).as_string_view(),
            "\xff\xff\xff\x7f"sv);
  EXPECT_EQ(encode_non_constexpr((std::uint64_t(1) << 56) - 1).as_string_view(),
            "\xff\xff\xff\xff\xff\xff\xff\x7f"sv);
}

TEST(VlqTest, EncodeAndDecode) {
  std::uint64_t v = 0;
  do {
    EncodedVlq ev = vlq_encode(v);
    EXPECT_EQ(ev.l, vlq_encode_length(v)) << v;

    auto decoded = vlq_decode(ev.buffer + ev.from, ev.buffer + ev.from + ev.l);
    EXPECT_TRUE(decoded.has_value()) << v;
    EXPECT_EQ(decoded->value, v);
    EXPECT_EQ(decoded->consumed_bytes, ev.l) << v;

    auto unsafe_decoded = vlq_decode_unsafe(ev.buffer + ev.from);
    EXPECT_EQ(unsafe_decoded.value, v);
    EXPECT_EQ(unsafe_decoded.consumed_bytes, ev.l) << v;

    std::uint64_t next = v | (v >> 1);
    if (v == next)
      ++v;
    else
      v = next;
  } while (v != 0);
}

TEST(VlqTest, DecodeAbnormal) {
  // Not terminated.
  EXPECT_FALSE(vlq_decode("\xff\xff"sv).has_value());
  // Overflow
  EXPECT_TRUE(
      vlq_decode("\x81\xff\xff\xff\xff\xff\xff\xff\xff\x7f"sv).has_value());
  EXPECT_FALSE(
      vlq_decode("\x82\xff\xff\xff\xff\xff\xff\xff\xff\x7f"sv).has_value());
}

}  // namespace
}  // namespace cbu
