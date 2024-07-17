#pragma once

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

#include <iterator>

namespace cbu {

// Sentinel for null-terminated strings
struct CStringSentinel {};

inline constexpr bool operator==(const char* s,
                                 const CStringSentinel&) noexcept {
  return *s == '\0';
}

inline constexpr bool operator==(const wchar_t* s,
                                 const CStringSentinel&) noexcept {
  return *s == L'\0';
}

inline constexpr bool operator==(const char8_t* s,
                                 const CStringSentinel&) noexcept {
  return *s == u8'\0';
}

inline constexpr bool operator==(const char16_t* s,
                                 const CStringSentinel&) noexcept {
  return *s == u'\0';
}

inline constexpr bool operator==(const char32_t* s,
                                 const CStringSentinel&) noexcept {
  return *s == U'\0';
}

static_assert(std::ranges::distance("hello", CStringSentinel()) == 5);

}  // namespace cbu
