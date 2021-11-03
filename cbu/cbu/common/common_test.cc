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

#include "cbu/common/common_test.h"

#include <string_view>

namespace std {

void PrintTo(const std::u8string_view& s, std::ostream* os) {
  *os << std::string_view(reinterpret_cast<const char*>(s.data()), s.size());
}

void PrintTo(const std::u16string_view& s, std::ostream* os) {
  auto buffer = make_unique_for_overwrite<char[]>(s.size() * 6);
  char* p = buffer.get();
  for (char16_t c : s) {
    // FIXME: This is actually incorrect -- treating char16_t as though it
    // were char32_t
    char* q = cbu::char32_to_utf8(p, c);
    if (q != nullptr)
      p = q;
    else
      *p++ = '?';
  }
  *os << std::string_view(buffer.get(), p - buffer.get());
}

void PrintTo(const std::u32string_view& s, std::ostream* os) {
  auto buffer = make_unique_for_overwrite<char[]>(s.size() * 6);
  char* p = buffer.get();
  for (char32_t c : s) {
    char* q = cbu::char32_to_utf8(p, c);
    if (q != nullptr)
      p = q;
    else
      *p++ = '?';
  }
  *os << std::string_view(buffer.get(), p - buffer.get());
}

}  // namespace std
