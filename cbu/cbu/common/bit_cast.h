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

#pragma once

#include <cstring>
#include <new>
#include <type_traits>
#include <utility>

namespace cbu {
inline namespace cbu_bit_cast {

template <typename To, typename From>
requires (sizeof(To) == sizeof(From)
  and std::is_trivially_copyable_v<From>
  and std::is_trivially_copyable_v<To>
  and std::is_trivially_default_constructible_v<To>)
inline To bit_cast(const From &src) noexcept {
  To to;
  // memcpy is standards compliant, whereas union support is GCC specific
  std::memcpy(std::addressof(to), std::addressof(src), sizeof(to));
  return to;
}

// For this function, we allow non-trivial types, and allow arbitrary size pairs
template <typename To, typename From>
inline To bit_cast_ex(const From &src) noexcept {
  union U {
    To to;
    From from;
    char to_mem[sizeof(To)];
    // Don't just use memset(std::addressof(to), 0, sizeof(to)),
    // which triggers warning:
    // clearing an object of type '...' with no trivial copy-assignment
    U() requires (sizeof(To) > sizeof(From)) {
      std::memset(to_mem, 0, sizeof(to_mem));
    }
    U() requires (sizeof(To) <= sizeof(From)) {}
  } u;
  new (std::addressof(u.from)) From(src);
  return u.to;
}

} // inline namespace cbu_bit_cast
} // namespace cbu
