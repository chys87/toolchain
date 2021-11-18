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

#include <ostream>
#include <string_view>

#include "cbu/common/byte_size.h"
#include "cbu/common/zstring_view.h"

using std::operator""sv;

namespace std {

// STL doesn't provide proper operator<< overload for char8_t, char16_t or
// char32_t
void PrintTo(std::u8string_view s, std::ostream* os);
void PrintTo(std::u16string_view s, std::ostream* os);
void PrintTo(std::u32string_view s, std::ostream* os);

}  // namespace std

namespace cbu {

void PrintTo(zstring_view, std::ostream*);
void PrintTo(u8zstring_view, std::ostream*);
void PrintTo(u16zstring_view, std::ostream*);
void PrintTo(u32zstring_view, std::ostream*);

template <std::size_t N>
void PrintTo(cbu::ByteSize<N> size, std::ostream* os) {
  *os << "ByteSize<"sv << N << ">("sv << std::size_t(size) << ") /* "sv
      << size.bytes() << " bytes */sv";
}

}  // namespace cbu
