/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020, chys <admin@CHYS.INFO>
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

#include "cbu/network/ipv4.h"
#include "cbu/common/fastdiv.h"
#include "cbu/common/short_string.h"

namespace cbu {
inline namespace cbu_ipv4 {
namespace {

char* uint8_to_string(char* p, std::uint8_t v) noexcept {
  if (v >= 100) {
    if (v >= 200) {
      *p++ = '2';
      v -= 200;
    } else if (v >= 100) {
      *p++ = '1';
      v -= 100;
    }
    goto _j;
  }
  if (v >= 10) {
_j:
    *p++ = fastdiv<10, 99>(v) + '0';
    *p++ = fastmod<10, 99>(v) + '0';
  } else {
    *p++ = v + '0';
  }
  return p;
}

struct ParseResult {
  bool ok;
  std::uint8_t v;
  const char* s;
};

ParseResult parse_uint8(
    const char* s, const char* e) noexcept {
  if (s >= e) {
    return {false};
  }
  if (*s < '0' || *s > '9') {
    return {false};
  }
  if (*s == '0') {
    // 0 is a special case.  We don't recognize extra leading zeros
    return {true, 0, s + 1};
  }
  unsigned int x = (*s++ - '0');
  if (s < e && *s >= '0' && *s <= '9') {
    x = x * 10 + (*s++ - '0');
    if (s < e && ((x < 25 && (*s >= '0' && *s <= '9')) ||
                  (x == 25 && (*s >= '0' && *s <= '5')))) {
      x = x * 10 + (*s++ - '0');
    }
  }
  return {true, std::uint8_t(x), s};
}

} // namespace

char* IPv4::to_string(char* p) const noexcept {
  p = uint8_to_string(p, a());
  *p++ = '.';
  p = uint8_to_string(p, b());
  *p++ = '.';
  p = uint8_to_string(p, c());
  *p++ = '.';
  p = uint8_to_string(p, d());
  return p;
}

short_string<15> IPv4::to_string() const {
  short_string<15> res;  // The max length of an IPv4 is 15
  res.set_length(to_string(res.buffer()) - res.buffer());
  return res;
}

std::optional<IPv4> IPv4::from_string(std::string_view s) noexcept {
  const char* p = s.data();
  const char* e = p + s.size();
  auto [a_ok, a, a_s] = parse_uint8(p, e);
  if (!a_ok) return std::nullopt;
  p = a_s;

  if (*p++ != '.') return std::nullopt;
  auto [b_ok, b, b_s] = parse_uint8(p, e);
  if (!b_ok) return std::nullopt;
  p = b_s;

  if (*p++ != '.') return std::nullopt;
  auto [c_ok, c, c_s] = parse_uint8(p, e);
  if (!c_ok) return std::nullopt;
  p = c_s;

  if (*p++ != '.') return std::nullopt;
  auto [d_ok, d, d_s] = parse_uint8(p, e);
  if (!d_ok || d_s != e) return std::nullopt;

  return IPv4(a, b, c, d);
}


} // namespace cbu_ipv4
} // namespace cbu
