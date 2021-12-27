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

#include "cbu/common/faststr.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "cbu/common/bit.h"
#include "cbu/common/stdhack.h"
#include "cbu/compat/string.h"
#include "cbu/math/common.h"
#include "cbu/math/strict_overflow.h"

namespace cbu {

alignas(64) const char arch_linear_bytes64[64] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
  0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
  0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
};

void* memdrop_var64(void* dst, uint64_t v, size_t n) noexcept {
  if (std::endian::native == std::endian::little) {

    char *d = static_cast<char *>(dst);
    char *e = d + n;

    switch (n) {
      default:
        __builtin_unreachable();
      case 8:
        memdrop8(d, v);
        break;
      case 5:
        memdrop4(d, uint32_t(v));
        d[4] = uint8_t(v >> 32);
        break;
      case 6:
        memdrop4(d, uint32_t(v));
        memdrop2(d + 4, uint16_t(v >> 32));
        break;
      case 7:
        memdrop4(d, uint32_t(v));
        memdrop4(d + 3, uint32_t(v >> 24));
        break;
      case 4:
        memdrop4(d, v);
        break;
      case 3:
        memdrop2(d, v);
        d[2] = uint32_t(v) >> 16;
        break;
      case 2:
        memdrop2(d, v);
        break;
      case 1:
        d[0] = v;
        break;
      case 0:
        break;
    }
    return e;

  } else {
    return compat::mempcpy(dst, &v, n);
  }
}

#if __WORDSIZE >= 64
void* memdrop_var128(void* dst, unsigned __int128 v, std::size_t n) noexcept {
  if (std::endian::native == std::endian::little) {
    if (n <= 8) {
      return memdrop_var64(dst, uint64_t(v), n);
    } else {
      memdrop8(static_cast<char*>(dst), uint64_t(v));
      memdrop8(static_cast<char*>(dst) + n - 8,
               uint64_t(v >> ((n - 8) * 8 % 64)));
      return static_cast<char*>(dst) + n;
    }
  } else {
    return compat::mempcpy(dst, &v, n);
  }
}
#endif

#ifdef __SSE2__
void* memdrop(void* dst, __m128i v, std::size_t n) noexcept {
  char *d = static_cast<char *>(dst);
  char *e = d + n;

  if (n > 8) {
    __m128i u = _mm_shuffle_epi8(
        v, *(const __m128i_u *)(arch_linear_bytes64 + n - 8));
    memdrop8(d, __v2di(v)[0]);
    memdrop8(e - 8, __v2di(u)[0]);
    return e;
  } else {
    return memdrop_var64(d, __v2di(v)[0], n);
  }
}
#endif

#ifdef __AVX2__
void* memdrop(void* dst, __m256i v, std::size_t n) noexcept {
  char *d = static_cast<char *>(dst);

  if (n > 16) {
    *(__m128i_u *)d = _mm256_extracti128_si256(v, 0);
    __m128i vv = _mm256_extracti128_si256(v, 1);
    return memdrop(d + 16, vv, n - 16);
  } else {
    return memdrop(d, _mm256_extracti128_si256(v, 0), n);
  }
}
#endif

namespace {

// We assume allocating 2**n bytes is optimal
inline std::size_t roundup_cap_nonzero(std::size_t n) {
  return (std::size_t(2) << bsr(n)) - 1;
}

} // namespace

std::size_t append_vnprintf(std::string* res, std::size_t n,
                            const char* format, std::va_list ap) {
  std::va_list ap_copy;
  va_copy(ap_copy, ap);

  // Argument n includes terminator.  We now don't count it, however.
  // Now we interpret n only as a hint.
  // It is automatically increased if too small.
  if (sub_overflow(&n, 2)) {
    n = 0;
  }
  ++n;

  std::size_t oldlen = res->size();

  n = roundup_cap_nonzero(oldlen + n) - oldlen;

  char *w = extend(res, n);
  int save_errno = errno;
  int extra_len = std::vsnprintf(w, n + 1, format, ap);
  errno = save_errno; // For %m
  std::size_t el = unsigned(extra_len);
  if (extra_len < 0) {
    // Something is really wrong. Leave original string intact
    truncate_unsafer(res, oldlen);
    return 0;
  } else if (el <= n) {
    // n was a good guess
    truncate_unsafer(res, oldlen + el);
    return extra_len;
  } else {
    // n was a bad guess. Increase it
    w = extend(res, el - n) - n;
    int new_extra_len = std::vsnprintf(w, el + 1, format, ap_copy);
    if (unsigned(new_extra_len) == el) {
      // The expected situation. Nothing to do
      return el;
    } else {
      // Something is really wrong. Leave original string intact
      truncate_unsafer(res, oldlen);
      return 0;
    }
  }
}

std::size_t append_nprintf(std::string* res, std::size_t hint_size,
                           const char *format, ...) {
  std::va_list ap;
  va_start(ap, format);
  return append_vnprintf(res, hint_size, format, ap);
  va_end(ap);
}

std::string vnprintf(std::size_t hint_size,
                     const char* format, std::va_list ap) {
  std::string res;
  append_vnprintf(&res, hint_size, format, ap);
  return res;
}

std::string nprintf(std::size_t hint_size,
                    const char* format, ...) {
  std::va_list ap;
  va_start(ap, format);
  return vnprintf(hint_size, format, ap);
  va_end(ap);
}

template void append<char>(std::string*, std::span<const std::string_view>);

}  // namespace cbu
