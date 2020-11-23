/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019, 2020, chys <admin@CHYS.INFO>
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

namespace cbu {
namespace compat {

inline void* mempcpy(void* d, const void* s, std::size_t n) noexcept {
#if defined __GLIBC__ && defined __USE_GNU
  return __builtin_mempcpy(d, s, n);
#else
  return static_cast<char*>(std::memcpy(d, s, n)) + n;
#endif
}

inline const void* memrchr(const void* s, int c, std::size_t n) noexcept {
#if defined __GLIBC__ && defined __USE_GNU
  return ::memrchr(s, c, n);
#else
  const void* r = nullptr;
  while (const void* found = std::memchr(s, c, n)) {
    r = found;
    n -= static_cast<const char*>(found) - static_cast<const char*>(s) + 1;
    s = static_cast<const char*>(found) + 1;
  }
  return r;
#endif
}

inline void* memrchr(void* s, int c, std::size_t n) noexcept {
  return const_cast<void*>(memrchr(static_cast<const void*>(s), c, n));
}

} // namespace compat
} // namespace cbu
