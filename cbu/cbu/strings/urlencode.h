/*
 * cbu - chys's basic utilities
 * Copyright (c) 2026, chys <admin@CHYS.INFO>
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

#include <cstddef>
#include <string_view>

namespace cbu {

// Decode all "%xx" (two hexadecimal digits, case-insensitive) sequences in a
// URI component, like JavaScript's decodeURIComponent (as opposed to
// decodeURI, which leaves reserved characters such as "%2f" untouched).
//
// Unlike JavaScript, this function is lenient and never fails:
//  - a '%' not followed by two hexadecimal digits is copied verbatim;
//  - the decoded bytes are not validated as UTF-8.
//
// Returns the end of the decoded result.  Decoding never expands the string,
// so dst must have room for at least size bytes.  dst and src must not
// overlap.
char* decode_uri_component(char* dst, const char* src,
                           std::size_t size) noexcept;

inline char* decode_uri_component(char* dst, std::string_view src) noexcept {
  return decode_uri_component(dst, src.data(), src.size());
}

}  // namespace cbu
