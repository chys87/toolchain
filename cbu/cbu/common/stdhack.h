/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2024, chys <admin@CHYS.INFO>
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

#include <string>

// Hacks standard containers to add more functionality

namespace cbu {

char *extend(std::string* buf, std::size_t n);
wchar_t *extend(std::wstring* buf, std::size_t n);
char16_t *extend(std::u16string* buf, std::size_t n);
char32_t *extend(std::u32string* buf, std::size_t n);
#if defined __cpp_char8_t && __cpp_char8_t >= 201811
char8_t *extend(std::u8string* buf, std::size_t n);
#endif

// Same as resize, but caller should guarantee n <= buf->size()
void truncate_unsafe(std::string* buf, std::size_t n);
void truncate_unsafe(std::wstring* buf, std::size_t n);
void truncate_unsafe(std::u16string* buf, std::size_t n);
void truncate_unsafe(std::u32string* buf, std::size_t n);
#if defined __cpp_char8_t && __cpp_char8_t >= 201811
void truncate_unsafe(std::u8string* buf, std::size_t n);
#endif

// Same as resize, but caller should guarantee n <= buf->size()
// and (*buf)[n] == '\0'
void truncate_unsafer(std::string* buf, std::size_t n);
void truncate_unsafer(std::wstring* buf, std::size_t n);
void truncate_unsafer(std::u16string* buf, std::size_t n);
void truncate_unsafer(std::u32string* buf, std::size_t n);
#if defined __cpp_char8_t && __cpp_char8_t >= 201811
void truncate_unsafer(std::u8string* buf, std::size_t n);
#endif

// Same as resize, but only truncates
inline void truncate(std::string* buf, std::size_t n) {
  if (n < buf->size()) {
    truncate_unsafe(buf, n);
  }
}
inline void truncate(std::wstring* buf, std::size_t n) {
  if (n < buf->size()) {
    truncate_unsafe(buf, n);
  }
}
inline void truncate(std::u16string* buf, std::size_t n) {
  if (n < buf->size()) {
    truncate_unsafe(buf, n);
  }
}
inline void truncate(std::u32string* buf, std::size_t n) {
  if (n < buf->size()) {
    truncate_unsafe(buf, n);
  }
}
#if defined __cpp_char8_t && __cpp_char8_t >= 201811
inline void truncate(std::u8string* buf, std::size_t n) {
  if (n < buf->size()) {
    truncate_unsafe(buf, n);
  }
}
#endif

} // namespace cbu
