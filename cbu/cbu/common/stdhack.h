/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2025, chys <admin@CHYS.INFO>
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

#include <string>
#include <type_traits>
#include <vector>

#include "cbu/common/concepts.h"
#include "cbu/compat/compilers.h"
#include "cbu/tweak/tweak.h"

// Hacks standard containers to add more functionality

namespace cbu {
namespace stdhack_detail {

template <typename T>
concept HasResizeDefaultInit = requires(std::basic_string<T>* s,
                                        std::size_t sz) {
  {s->__resize_default_init(sz)};
};

// We can test for macro __cpp_lib_string_resize_and_overwrite, but using
// concept is more direct.
template <typename T>
concept HasResizeAndOverwrite =
    requires(std::basic_string<T>* s, std::size_t sz) {
      {
        s->resize_and_overwrite(sz, [sz](T* w) { return 0; })
      };
    };

#if defined __GLIBCXX__ && defined _GLIBCXX_USE_CXX11_ABI
template <typename C, void (std::basic_string<C>::*foo)(std::size_t)>
struct MLengthAccessor {
  friend constexpr void _M_length(std::basic_string<C>* ptr,
                                  std::size_t l) noexcept {
    (ptr->*foo)(l);
  }
};

template <typename C, void (std::basic_string<C>::*foo)(std::size_t)>
struct MSetLengthAccessor {
  friend constexpr void _M_set_length(std::basic_string<C>* ptr,
                                      std::size_t l) noexcept {
    (ptr->*foo)(l);
  }
};

template struct MLengthAccessor<char, &std::string::_M_length>;
template struct MLengthAccessor<wchar_t, &std::wstring::_M_length>;
template struct MLengthAccessor<char8_t, &std::u8string::_M_length>;
template struct MLengthAccessor<char16_t, &std::u16string::_M_length>;
template struct MLengthAccessor<char32_t, &std::u32string::_M_length>;
template struct MSetLengthAccessor<char, &std::string::_M_set_length>;
template struct MSetLengthAccessor<wchar_t, &std::wstring::_M_set_length>;
template struct MSetLengthAccessor<char8_t, &std::u8string::_M_set_length>;
template struct MSetLengthAccessor<char16_t, &std::u16string::_M_set_length>;
template struct MSetLengthAccessor<char32_t, &std::u32string::_M_set_length>;

constexpr void _M_length(std::string*, std::size_t) noexcept;
constexpr void _M_length(std::wstring*, std::size_t) noexcept;
constexpr void _M_length(std::u8string*, std::size_t) noexcept;
constexpr void _M_length(std::u16string*, std::size_t) noexcept;
constexpr void _M_length(std::u32string*, std::size_t) noexcept;
constexpr void _M_set_length(std::string*, std::size_t) noexcept;
constexpr void _M_set_length(std::wstring*, std::size_t) noexcept;
constexpr void _M_set_length(std::u8string*, std::size_t) noexcept;
constexpr void _M_set_length(std::u16string*, std::size_t) noexcept;
constexpr void _M_set_length(std::u32string*, std::size_t) noexcept;
#endif

}  // namespace stdhack_detail

template <Std_string_char T>
constexpr T* extend(std::basic_string<T>* buf,
                    std::size_t n) CBU_MEMORY_NOEXCEPT {
#if defined __GLIBCXX__ && defined _GLIBCXX_USE_CXX11_ABI
  buf->reserve(buf->size() + n);
  T* w = buf->data() + buf->size();
  stdhack_detail::_M_set_length(buf, buf->size() + n);
  return w;
#else
  if constexpr (stdhack_detail::HasResizeDefaultInit<T>) {
    buf->__resize_default_init(buf->size() + n);
  } else if constexpr (stdhack_detail::HasResizeAndOverwrite<T>) {
    size_t target_len = buf->size() + n;
    buf->resize_and_overwrite(target_len,
                              [target_len](T*, std::size_t) constexpr noexcept {
                                return target_len;
                              });
  } else {
    buf->resize(buf->size() + n);
  }
  return (buf->data() + buf->size() - n);
#endif
}

// Same as resize, but caller should guarantee n <= buf->size()
template <Std_string_char T>
constexpr void truncate_unsafe(std::basic_string<T>* buf,
                               std::size_t n) noexcept {
  // assert(n <= buf->size());
#if defined __GLIBCXX__ && defined _GLIBCXX_USE_CXX11_ABI
  stdhack_detail::_M_set_length(buf, n);
#else
  // No difference, but __resize_default_init generates shorter code
  if constexpr (stdhack_detail::HasResizeDefaultInit<T>) {
    buf->__resize_default_init(n);
  } else {
    buf->resize(n);
  }
#endif
}

// Same as resize, but caller should guarantee n <= buf->size()
// and (*buf)[n] == '\0'
template <Std_string_char T>
constexpr void truncate_unsafer(std::basic_string<T>* buf,
                                std::size_t n) noexcept {
  // assert(n <= buf->size());
  // assert((*buf)[n] == '\0');
#if defined __GLIBCXX__ && defined _GLIBCXX_USE_CXX11_ABI
  stdhack_detail::_M_length(buf, n);
#else
  // No difference, but __resize_default_init generates shorter code
  if constexpr (stdhack_detail::HasResizeDefaultInit<T>) {
    buf->__resize_default_init(n);
  } else {
    buf->resize(n);
  }
#endif
}

// Same as resize, but only truncates
template <Std_string_char T>
constexpr void truncate(std::basic_string<T>* buf, std::size_t n) noexcept {
  if (n < buf->size()) {
    truncate_unsafe(buf, n);
  }
}

namespace stdhack_detail {

template <typename T>
class Vector : public std::vector<T> {
 public:
  T* extend(std::size_t n) {
    this->reserve(this->size() + n);
    T* r = this->_M_impl._M_finish;
    this->_M_impl._M_finish += n;
    return r;
  }
};

}  // namespace stdhack_detail

template <typename T>
  requires std::is_trivially_default_constructible_v<T>
inline T* extend(std::vector<T>* vec, std::size_t n) {
#if defined __GLIBCXX__ && !defined CBU_ADDRESS_SANITIZER
  static_assert(sizeof(stdhack_detail::Vector<T>) == sizeof(std::vector<T>));
  return static_cast<stdhack_detail::Vector<T>*>(vec)->extend(n);
#else
  vec->resize(vec->size() + n);
  return vec->data() + vec->size() - n;
#endif
}

}  // namespace cbu
