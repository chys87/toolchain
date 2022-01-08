/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2022, chys <admin@CHYS.INFO>
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

#include <type_traits>
#include <utility>

#include "cbu/common/concepts.h"
#include "cbu/math/super_integer.h"

namespace cbu {

template <Raw_integral A, Raw_integral B, Raw_integral C>
inline constexpr bool mul_overflow(A a, B b, C* c) noexcept {
  if (std::is_constant_evaluated()) {
    SuperInteger<std::make_unsigned_t<std::common_type_t<A, B>>> si(a);
    bool overflow = si.mul_overflow(b);
    return !si.cast(c) || overflow;
  }
  return __builtin_mul_overflow(a, b, c);
}

template <Raw_integral A, Raw_integral B, Raw_integral C>
inline constexpr bool add_overflow(A a, B b, C* c) noexcept {
  if (std::is_constant_evaluated()) {
    SuperInteger<std::make_unsigned_t<std::common_type_t<A, B>>> si(a);
    bool overflow = si.add_overflow(b);
    return !si.cast(c) || overflow;
  }
  return __builtin_add_overflow(a, b, c);
}

template <typename T, typename U>
  requires std::is_convertible_v<T*, U*>
inline bool add_overflow(T* a, std::size_t b, U** c) noexcept {
  std::uintptr_t res;
  if (__builtin_add_overflow(std::uintptr_t(a), b, &res)) {
    return true;
  } else {
    *c = reinterpret_cast<U*>(res);
    return false;
  }
}

template <Raw_integral A, Raw_integral B, Raw_integral C>
inline constexpr bool sub_overflow(A a, B b, C* c) noexcept {
  if (std::is_constant_evaluated()) {
    SuperInteger<std::make_unsigned_t<std::common_type_t<A, B>>> si(a);
    bool overflow = si.sub_overflow(b);
    return !si.cast(c) || overflow;
  }
  return __builtin_sub_overflow(a, b, c);
}

template <Raw_integral A>
inline constexpr A addc(A x, A y, A carryin, A* carryout) noexcept {
  if (!std::is_constant_evaluated()) {
    if constexpr (std::is_unsigned_v<A>) {
      // Currently Clang has these addc builtins
#define CBU_ADDC(Type, Builtin)              \
  if constexpr (sizeof(A) == sizeof(Type)) { \
    Type o;                                  \
    A ret = Builtin(x, y, carryin, &o);      \
    *carryout = o;                           \
    return ret;                              \
  }
#if __has_builtin(__builtin_addcb)
      CBU_ADDC(unsigned char, __builtin_addcb)
#endif
#if __has_builtin(__builtin_addcs)
      CBU_ADDC(unsigned short, __builtin_addcs)
#endif
#if __has_builtin(__builtin_addc)
      CBU_ADDC(unsigned, __builtin_addc)
#endif
#if __has_builtin(__builtin_addcl)
      CBU_ADDC(unsigned long, __builtin_addcl)
#endif
#if __has_builtin(__builtin_addcll)
      CBU_ADDC(unsigned long long, __builtin_addcll)
#endif
#undef CBU_ADDC
    }
  }
  A tmp;
  bool c1 = add_overflow(x, y, &tmp);
  A res;
  bool c2 = add_overflow(tmp, carryin, &res);
  *carryout = c1 || c2;
  return res;
}

template <std::integral U, std::integral V>
inline bool sub_overflow(U* a, V b) noexcept {
  return sub_overflow(*a, b, a);
}

}  // namespace cbu
