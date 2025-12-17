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

#include <compare>
#include <cstdint>

namespace cbu {

#ifdef __SIZEOF_INT128__
#define CBU_HAS_NATIVE_INT128
using uint128_t = unsigned __int128;
#else
#undef CBU_HAS_NATIVE_INT128
#endif

class uint128 {
 public:
  constexpr uint128() noexcept = default;
  constexpr uint128(std::uint64_t l) noexcept : l_(l) {}
  constexpr uint128(std::uint64_t h, std::uint64_t l) noexcept : l_(l), h_(h) {}
#ifdef CBU_HAS_NATIVE_INT128
  constexpr uint128(uint128_t v) noexcept : l_(v), h_(v >> 64) {}
#endif
  constexpr uint128(const uint128&) noexcept = default;

  constexpr uint128& operator=(const uint128&) noexcept = default;

  constexpr std::uint64_t lo() const noexcept { return l_; }
  constexpr std::uint64_t hi() const noexcept { return h_; }
  constexpr void set_lo(std::uint64_t l) noexcept { l_ = l; }
  constexpr void set_hi(std::uint64_t h) noexcept { h_ = h; }

  explicit operator bool() const noexcept { return l_ | h_; }
#ifdef CBU_HAS_NATIVE_INT128
  explicit operator uint128_t() const noexcept { return (static_cast<uint128_t>(h_) << 64) | l_; }
#endif

  friend constexpr bool operator==(const uint128& a, const uint128& b) noexcept {
#ifdef CBU_HAS_NATIVE_INT128
    return uint128_t(a) == uint128_t(b);
#else
    return a.l_ == b.l_ && a.h_ == b.h_;
#endif
  }
  friend constexpr std::strong_ordering operator<=>(const uint128& a,
                                                    const uint128& b) noexcept {
#ifdef CBU_HAS_NATIVE_INT128
    return uint128_t(a) <=> uint128_t(b);
#else
    auto r = a.h_ <=> b.h_;
    if (r == 0) r = a.l_ <=> b.l_;
    return r;
#endif
  }

 private:
  std::uint64_t l_ = 0;
  std::uint64_t h_ = 0;
};

}  // namespace cbu
