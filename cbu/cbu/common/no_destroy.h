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

#include <type_traits>
#include <utility>

#include "cbu/compat/compilers.h"

namespace cbu {

// A union whose destructor does not destroy its active member.
//
// The caller is responsible for ending the lifetime of ``value`` if needed.
template <typename T>
union NoDestroy {
  T value;

  template <typename... Args>
  constexpr explicit NoDestroy(Args&&... args) noexcept(
      std::is_nothrow_constructible_v<T, Args...>)
      : value{std::forward<Args>(args)...} {}

  constexpr ~NoDestroy() noexcept {}
};

}  // namespace cbu

#ifdef CBU_COMPILER_HAS_NO_DESTROY

// Define a namespace-scope object whose destructor is not run. The arguments
// are the elements of a braced initializer list. For example,
// ``CBU_DEFINE_NO_DESTROY(Pair, pair, 1, 2)`` defines ``Pair pair{1, 2}``.
#define CBU_DEFINE_NO_DESTROY(Type, Name, ...) \
  CBU_NO_DESTROY Type Name { __VA_ARGS__ }

#define CBU_CONSTINIT_NO_DESTROY(Type, Name, ...) \
  CBU_NO_DESTROY constinit Type Name { __VA_ARGS__ }

#else

#define CBU_DEFINE_NO_DESTROY(Type, Name, ...)                       \
  ::cbu::NoDestroy<Type> Name##_cbu_no_destroy_storage{__VA_ARGS__}; \
  Type& Name = Name##_cbu_no_destroy_storage.value

#define CBU_CONSTINIT_NO_DESTROY(Type, Name, ...)                              \
  constinit ::cbu::NoDestroy<Type> Name##_cbu_no_destroy_storage{__VA_ARGS__}; \
  constinit Type& Name = Name##_cbu_no_destroy_storage.value

#endif
