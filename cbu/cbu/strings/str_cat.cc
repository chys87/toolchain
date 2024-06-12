/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2023, chys <admin@CHYS.INFO>
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

#include "cbu/strings/str_cat.h"

#include <string_view>

#include "cbu/common/stdhack.h"

namespace cbu {
namespace {

inline char* Copy(char* w, const std::string_view* array, std::size_t cnt) {
  for (std::size_t i = 0; i < cnt; ++i) {
    std::string_view sv = array[i];
    w = static_cast<char*>(__builtin_mempcpy(w, sv.data(), sv.size()));
  }
  return w;
}

}  // namespace

void str_cat_detail::Append(std::string* dst, const std::string_view* array,
                            std::size_t cnt,
                            std::size_t total_size) CBU_MEMORY_NOEXCEPT {
#if 0
  std::size_t old_size = dst->size();
  dst->resize_and_overwrite(
      old_size + total_size,
      [array, cnt, total_size, old_size](char* w, std::size_t) noexcept {
        Copy(w + old_size, array, cnt);
        return old_size + total_size;
      });
#else
  auto w = extend(dst, total_size);
  Copy(w, array, cnt);
#endif
}

void str_cat_detail::Append(std::string* dst, std::string_view a,
                            std::string_view b) CBU_MEMORY_NOEXCEPT {
  auto w = extend(dst, a.size() + b.size());
  w = static_cast<char*>(__builtin_mempcpy(w, a.data(), a.size()));
  __builtin_mempcpy(w, b.data(), b.size());
}

std::string str_cat_detail::Cat(const std::string_view* array, std::size_t cnt,
                                std::size_t total_size) CBU_MEMORY_NOEXCEPT {
  std::string res;
  res.resize_and_overwrite(
      total_size, [array, cnt, total_size](char* w, std::size_t) noexcept {
        Copy(w, array, cnt);
        return total_size;
      });
  return res;
}

std::string str_cat_detail::Cat(std::string_view a,
                                std::string_view b) CBU_MEMORY_NOEXCEPT {
  std::string res;
  std::size_t sz = a.size() + b.size();
  res.resize_and_overwrite(sz, [a, b, sz](char* w, std::size_t) noexcept {
    w = static_cast<char*>(__builtin_mempcpy(w, a.data(), a.size()));
    __builtin_mempcpy(w, b.data(), b.size());
    return sz;
  });
  return res;
}

}  // namespace cbu
