/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019, chys <admin@CHYS.INFO>
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

#include <cstdarg>
#include <cstddef>
#include <string_view>

namespace cbu {
inline namespace cbu_strutil {

// Always return the number of characters actually filled.
// Function name taken from Linux kernel.
std::size_t vscnprintf(char *, std::size_t, const char *, std::va_list) noexcept
  __attribute__((__format__(__printf__, 3, 0)));

std::size_t scnprintf(char *, std::size_t, const char *, ...) noexcept
  __attribute__((__format__(__printf__, 3, 4)));

char *strdup_new(const char *)
  __attribute__((__malloc__, __nonnull__(1)));

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


// Same as std::reverse, but more optimized (at least for x86)
char *reverse(char *, char *) noexcept;

// c_str: Convert anything to a C-style string
inline constexpr const char *c_str(const char *s) noexcept {
  return s;
}

template<typename T>
requires requires(T s) { {s.c_str()} -> const char *; }
inline constexpr const char *c_str(const T &s) noexcept(noexcept(s.c_str())) {
  return s.c_str();
}

template<typename T>
requires requires(T s) { {s->c_str()} -> const char *; }
inline constexpr const char *c_str(const T &s) noexcept(noexcept(s->c_str())) {
  return s->c_str();
}

} // namespace cbu_strutil
} // namespace cbu
