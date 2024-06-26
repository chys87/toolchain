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

#if __has_include(<bits/c++config.h>)
# include <bits/c++config.h>
#endif

#if defined __GLIBCXX__ && defined _GLIBCXX_USE_CXX11_ABI
# define private public
# define protected public
#endif

#include <cassert>
#include <string>

#include "cbu/common/stdhack.h"

// Hacks standard containers to add more functionality

namespace cbu {
namespace {

template <typename T>
concept HasResizeDefaultInit = requires(std::basic_string<T>* s,
                                        std::size_t sz) {
  {s->__resize_default_init(sz)};
};

template <typename T>
T* extend_impl(std::basic_string<T>* buf, std::size_t n) CBU_MEMORY_NOEXCEPT {
#if defined __GLIBCXX__ && defined _GLIBCXX_USE_CXX11_ABI
  buf->reserve(buf->size() + n);
  T* w = buf->data() + buf->size();
  buf->_M_set_length(buf->size() + n);
  return w;
#else
  if constexpr (HasResizeDefaultInit<T>) {
    buf->__resize_default_init(buf->size() + n);
  } else {
#  if defined __cpp_lib_string_resize_and_overwrite
    size_t target_len = buf->size() + n;
    buf->resize_and_overwrite(
        target_len,
        [target_len](T*, std::size_t) noexcept { return target_len; });
#  else
    buf->resize(buf->size() + n);
#  endif
  }
  return (buf->data() + buf->size() - n);
#endif
}

template <typename T>
void truncate_unsafe_impl(std::basic_string<T>* buf, std::size_t n) noexcept {
  assert(n <= buf->size());
#if defined __GLIBCXX__ && defined _GLIBCXX_USE_CXX11_ABI
  buf->_M_set_length(n);
#else
  // No difference, but __resize_default_init generates shorter code
  if constexpr (HasResizeDefaultInit<T>) {
    buf->__resize_default_init(n);
  } else {
    buf->resize(n);
  }
#endif
}

template <typename T>
void truncate_unsafer_impl(std::basic_string<T>* buf, std::size_t n) noexcept {
  assert(n <= buf->size());
  assert((*buf)[n] == '\0');
#if defined __GLIBCXX__ && defined _GLIBCXX_USE_CXX11_ABI
  buf->_M_length(n);
#else
  // No difference, but __resize_default_init generates shorter code
  if constexpr (HasResizeDefaultInit<T>) {
    buf->__resize_default_init(n);
  } else {
    buf->resize(n);
  }
#endif
}

} // namespace

char *extend(std::string* buf, std::size_t n) CBU_MEMORY_NOEXCEPT {
  return extend_impl(buf, n);
}

wchar_t *extend(std::wstring* buf, std::size_t n) CBU_MEMORY_NOEXCEPT {
  return extend_impl(buf, n);
}

char16_t *extend(std::u16string* buf, std::size_t n) CBU_MEMORY_NOEXCEPT {
  return extend_impl(buf, n);
}

char32_t *extend(std::u32string* buf, std::size_t n) CBU_MEMORY_NOEXCEPT {
  return extend_impl(buf, n);
}

char8_t *extend(std::u8string* buf, std::size_t n) CBU_MEMORY_NOEXCEPT {
  return extend_impl(buf, n);
}

void truncate_unsafe(std::string* buf, std::size_t n) noexcept {
  truncate_unsafe_impl(buf, n);
}
void truncate_unsafe(std::wstring* buf, std::size_t n) noexcept {
  truncate_unsafe_impl(buf, n);
}
void truncate_unsafe(std::u16string* buf, std::size_t n) noexcept {
  truncate_unsafe_impl(buf, n);
}
void truncate_unsafe(std::u32string* buf, std::size_t n) noexcept {
  truncate_unsafe_impl(buf, n);
}
void truncate_unsafe(std::u8string* buf, std::size_t n) noexcept {
  truncate_unsafe_impl(buf, n);
}

void truncate_unsafer(std::string* buf, std::size_t n) noexcept {
  truncate_unsafer_impl(buf, n);
}
void truncate_unsafer(std::wstring* buf, std::size_t n) noexcept {
  truncate_unsafer_impl(buf, n);
}
void truncate_unsafer(std::u16string* buf, std::size_t n) noexcept {
  truncate_unsafer_impl(buf, n);
}
void truncate_unsafer(std::u32string* buf, std::size_t n) noexcept {
  truncate_unsafer_impl(buf, n);
}
void truncate_unsafer(std::u8string* buf, std::size_t n) noexcept {
  truncate_unsafer_impl(buf, n);
}

}  // namespace cbu
