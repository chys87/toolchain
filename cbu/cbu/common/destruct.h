/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020-2021, chys <admin@CHYS.INFO>
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

#include <cstddef>
#include <type_traits>

namespace cbu {

struct DestructNoDelete {
  void operator()(void *) const noexcept {}
};

template <std::size_t alignment = 0>
struct DestructScalarDelete {
  void operator()(void *p) const noexcept {
    if constexpr (alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
      ::operator delete(p, std::align_val_t(alignment));
    else
      ::operator delete(p);
  }
};

template <std::size_t alignment = 0>
struct DestructArrayDelete {
  void operator()(void *p) const noexcept {
    if constexpr (alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
      ::operator delete[](p, std::align_val_t(alignment));
    else
      ::operator delete[](p);
  }
};

template <std::size_t alignment = 0>
struct DestructSizedScalarDelete {
  std::size_t size;

  void operator()(void *p) const noexcept {
    if constexpr (alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
      ::operator delete(p, size, std::align_val_t(alignment));
    else
      ::operator delete(p, size);
  }
};

template <std::size_t alignment = 0>
struct DestructSizedArrayDelete {
  std::size_t size;

  void operator()(void *p) const noexcept {
    if constexpr (alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
      ::operator delete[](p, size, std::align_val_t(alignment));
    else
      ::operator delete[](p, size);
  }
};

template <typename T, typename Deleter = DestructNoDelete>
  requires std::is_trivially_destructible<T>::value
void destruct(T *begin, T *end, Deleter deleter = Deleter()) noexcept {
  (void)end;
  deleter(begin);
}

template <typename T, typename Deleter = DestructNoDelete>
  requires (!std::is_trivially_destructible<T>::value)
void destruct(T *begin, T *end, Deleter deleter = Deleter()) noexcept {
  T *p = end;
  while (p > begin)
    (--p)->~T();
  deleter(begin);
}

} // namesapce cbu
