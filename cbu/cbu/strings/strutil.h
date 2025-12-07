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

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <cwchar>
#include <string_view>

namespace cbu {

std::size_t strcnt(const char *, char) noexcept
  __attribute__((__nonnull__(1), __pure__));

std::size_t memcnt(const char *, char, size_t) noexcept
  __attribute__((__nonnull__(1), __pure__));

inline std::size_t memcnt(std::string_view sv, char c) noexcept {
  return memcnt(sv.data(), c, sv.length());
}

// Same as strcmp, but compares contiguous digits as numbers
int strnumcmp(const char *, const char *) noexcept
  __attribute__((__nonnull__(1, 2), __pure__));

int strnumcmp(const char*, size_t, const char*, size_t) noexcept
    __attribute__((__pure__));

inline int strnumcmp(std::string_view a, std::string_view b) noexcept {
  return strnumcmp(a.data(), a.size(), b.data(), b.size());
}

// Same as std::reverse, but more optimized (at least for x86)
char *reverse(char *, char *) noexcept;

// Returns the prefix of s, but always cut on UTF-8 character boundaries
[[gnu::pure]] size_t truncate_string_utf8_impl(const void* s, size_t n) noexcept;
inline std::string_view truncate_string_utf8(std::string_view s,
                                             size_t n) noexcept {
  if (n >= s.size()) return s;
  return {s.data(), truncate_string_utf8_impl(s.data(), n)};
}
inline std::u8string_view truncate_string_utf8(std::u8string_view s,
                                               size_t n) noexcept {
  if (n >= s.size()) return s;
  return {s.data(), truncate_string_utf8_impl(s.data(), n)};
}


// Return the length of common prefix.
// If utf8 is true, always cut on UTF-8 character boundaries.
[[gnu::pure]] size_t common_prefix_ex(const void*, const void*, size_t) noexcept;
[[gnu::pure]] size_t common_suffix_ex(const void*, const void*, size_t) noexcept;
[[gnu::pure]] size_t common_suffix_fix_for_utf8(const void*, size_t) noexcept;

inline size_t common_prefix(std::string_view sa, std::string_view sb) noexcept {
  size_t maxl = std::min(sa.length(), sb.length());
  return common_prefix_ex(sa.data(), sb.data(), maxl);
}

inline size_t common_prefix_utf8(std::string_view sa,
                                 std::string_view sb) noexcept {
  size_t maxl = std::min(sa.length(), sb.length());
  size_t l = common_prefix_ex(sa.data(), sb.data(), maxl);
  if (l != maxl) l = truncate_string_utf8_impl(sa.data(), l);
  return l;
}

inline size_t common_prefix(std::u8string_view sa,
                            std::u8string_view sb) noexcept {
  size_t maxl = std::min(sa.length(), sb.length());
  size_t l = common_prefix_ex(sa.data(), sb.data(), maxl);
  if (l != maxl) l = truncate_string_utf8_impl(sa.data(), l);
  return l;
}

inline size_t common_suffix(std::string_view sa, std::string_view sb) noexcept {
  size_t maxl = std::min(sa.length(), sb.length());
  return common_suffix_ex(sa.end(), sb.end(), maxl);
}

inline size_t common_suffix_utf8(std::string_view sa,
                                 std::string_view sb) noexcept {
  size_t maxl = std::min(sa.length(), sb.length());
  size_t l = common_suffix_ex(sa.end(), sb.end(), maxl);
  return common_suffix_fix_for_utf8(sa.end(), l);
}

inline size_t common_suffix(std::u8string_view sa,
                            std::u8string_view sb) noexcept {
  size_t maxl = std::min(sa.length(), sb.length());
  size_t l = common_suffix_ex(sa.end(), sb.end(), maxl);
  return common_suffix_fix_for_utf8(sa.end(), l);
}

// Return the length of prefix composed only of character c
[[gnu::pure]] size_t char_span_length(const void* buffer, size_t len, char c) noexcept;

inline size_t char_span_length(std::string_view s, char c) noexcept {
  return char_span_length(s.data(), s.size(), c);
}

}  // namespace cbu
