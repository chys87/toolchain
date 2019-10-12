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

#include "strutil.h"
#include <cstdio>
#include <cstring>
#include <type_traits>

namespace cbu {
inline namespace cbu_strutil {

using ssize_t = std::make_signed_t<size_t>;

std::size_t vscnprintf(char *buf, std::size_t bufsiz, const char *format,
                       std::va_list ap) noexcept {
  // Be careful we should be able to handle bufsiz == 0
  ssize_t len = std::vsnprintf(buf, bufsiz, format, ap);
  if (len >= ssize_t(bufsiz)) {
    len = bufsiz - 1;
  }
  if (len < 0) {
    len = 0;
  }
  return len;
}

std::size_t scnprintf(char *buf, std::size_t bufsiz,
                      const char *format, ...) noexcept {
  std::size_t r;
  std::va_list ap;
  va_start(ap, format);
  r = vscnprintf(buf, bufsiz, format, ap);
  va_end(ap);
  return r;
}

char *strdup_new(const char *s) {
  std::size_t l = std::strlen(s) + 1;
  return static_cast<char *>(std::memcpy(new char[l], s, l));
}

} // namespace cbu_strutil
} // namespace cbu
