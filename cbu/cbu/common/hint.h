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

#pragma once

#include <assert.h>

namespace cbu {

#if __has_builtin(__builtin_assume)
#  define CBU_HINT_ASSUME(expr) __builtin_assume(expr)
#elif __has_builtin(__builtin_unreachable)
#  define CBU_HINT_ASSUME(expr)             \
    do {                                    \
      if (!(expr)) __builtin_unreachable(); \
    } while (0)
#else
#  define CBU_HINT_ASSUME(expr) ((void)0)
#endif

#define CBU_HINT_ASSERT(cond) \
  do {                        \
    assert(cond);             \
    CBU_HINT_ASSUME(cond);    \
  } while (0)

// Give the compiler a hint that the pointer is guaranteed to be non-null
template <typename T> inline T* hint_nonnull(T *ptr) noexcept {
  CBU_HINT_ASSUME(ptr != nullptr);
  return ptr;
}

#define CBU_HINT_NONNULL ::cbu::hint_nonnull

} // namespace cbu
