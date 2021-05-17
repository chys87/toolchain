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

#include "cbu/common/encoding.h"

#if __has_include(<x86intrin.h>)
# include <x86intrin.h>
#endif

#include "cbu/common/faststr.h"

namespace cbu {
inline namespace cbu_encoding {

char8_t* char32_to_utf8(char8_t* w, char32_t u) noexcept {
  if (u < 0x800) {
    if (u < 0x80) {
      *w++ = u;
    } else {
#if defined __BMI2__
      w = memdrop_be<uint16_t>(w, _pdep_u32(u, 0x1f3f) | 0xc080u);
#else
      *w++ = (u >> 6) + 0xc0u;
      *w++ = (u & 0x3fu) + 0x80u;
#endif
    }
  } else if (u < 0x10000) {
    if (u >= 0xd800 && u <= 0xdfff) {
      // UTF-16 surrogate pairs
#ifndef __clang__
      [[unlikely]]
#endif
      return nullptr;
    } else {
      // Most Chinese characters fall here
#if defined __BMI2__
      memdrop_be<uint32_t>(w, _pdep_u32(u, 0x0f3f3f00) | 0xe0808000);
      w += 3;
#else
      *w++ = (u >> 12) + 0xe0u;
      *w++ = ((u >> 6) & 0x3fu) + 0x80u;
      *w++ = (u & 0x3fu) + 0x80u;
#endif
    }
  } else {
#ifndef __clang__
    [[unlikely]]
#endif
    if (u < 0x110000) {
#if defined __BMI2__
      w = memdrop_be<uint32_t>(w, _pdep_u32(u, 0x073f3f3f) | 0xf0808080u);
#else
      *w++ = (u >> 18) + 0xf0u;
      *w++ = ((u >> 12) & 0x3fu) + 0x80u;
      *w++ = ((u >> 6) & 0x3fu) + 0x80u;
      *w++ = (u & 0x3fu) + 0x80u;
#endif
    } else {
      return nullptr;
    }
  }
  return w;
}

char* char32_to_utf8(char* dst, char32_t c) noexcept {
  return reinterpret_cast<char*>(
      char32_to_utf8(reinterpret_cast<char8_t*>(dst), c));
}

} // namepsace cbu_encoding
} // namespace cbu
