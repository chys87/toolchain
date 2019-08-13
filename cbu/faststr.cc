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

#include "faststr.h"
#include <string.h>

namespace cbu {
inline namespace cbu_faststr {

void *memdrop(void *dst, uint64_t v, size_t n) noexcept {
  if (std::endian::native == std::endian::little) {

    char *d = static_cast<char *>(dst);
    char *e = d + n;

    switch (n) {
      default:
        __builtin_unreachable();
      case 8:
        memdrop8(d, v);
        break;
      case 5:
        memdrop4(d, uint32_t(v));
        d[4] = uint8_t(v >> 32);
        break;
      case 6:
        memdrop4(d, uint32_t(v));
        memdrop2(d + 4, uint16_t(v >> 32));
        break;
      case 7:
        memdrop4(d, uint32_t(v));
        memdrop4(d + 3, uint32_t(v >> 24));
        break;
      case 4:
        memdrop4(d, v);
        break;
      case 3:
        memdrop2(d, v);
        d[2] = uint32_t(v) >> 16;
        break;
      case 2:
        memdrop2(d, v);
        break;
      case 1:
        d[0] = v;
        break;
      case 0:
        break;
    }
    return e;

  } else {
    return mempcpy(dst, &v, n);
  }
}

} // namespace cbu_faststr
} // namespace cbu
