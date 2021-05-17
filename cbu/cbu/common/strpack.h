/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2020, chys <admin@CHYS.INFO>
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

#ifdef __clang__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wgnu-string-literal-operator-template"
#endif

namespace cbu {
inline namespace cbu_strpack {

// STRPACK: Convert a string literal to a template argument pack
template <typename T, T... chars>
struct strpack {
  enum : std::size_t { n = sizeof...(chars) };
  static inline constexpr T s[n + 1] = {chars..., T()};
  static constexpr T* fill(T* p) noexcept {
    ((*p++ = chars), ...);
    return p;
  }
};

template <typename T, T... chars>
constexpr strpack<T, chars...> operator""_strpack() {
  return {};
}

// This can be used as template argument!!
template <typename T, T... chars>
constexpr const T (&operator""_str())[sizeof...(chars) + 1] {
  return strpack<T, chars...>::s;
}

} // namespace cbu_strpack
} // namespace cbu

#ifdef __clang__
# pragma GCC diagnostic pop
#endif
