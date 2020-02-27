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

#include <memory>

namespace cbu {
inline namespace cbu_memory {

inline void sized_delete(void* p , std::size_t n) {
#ifdef __cpp_sized_deallocation
  ::operator delete[](p, n);
#else
  ::operator delete[](p);
#endif
}

struct SizedArrayDeleter {
  size_t bytes;
  void operator()(void* p) noexcept {
    sized_delete(p, bytes);
  }
};

template <typename T>
requires std::is_unbounded_array_v<T>
using sized_unique_ptr = std::unique_ptr<T, SizedArrayDeleter>;

template <typename T, typename... Args>
requires (!std::is_array_v<T>)
inline std::unique_ptr<T> make_unique(Args&&... args) {
  return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T>
requires std::is_unbounded_array_v<T>
inline sized_unique_ptr<T> make_unique(std::size_t n) {
  using V = std::remove_extent_t<T>;
  V* p = static_cast<V*>(::operator new(n * sizeof(V)));
  try {
    std::uninitialized_value_construct_n(p, n);
  } catch (...) {
    ::operator delete(p, n * sizeof(V));
    throw;
  }
  return sized_unique_ptr<T>(p, SizedArrayDeleter{n * sizeof(V)});
}

template <typename T>
requires (!std::is_array_v<T>)
inline std::unique_ptr<T> make_unique_for_overwrite() {
  // GCC 9 doesn't have std::make_unique_for_overwrite yet.
  // return std::make_unique_for_overwrite<T>();
  return std::unique_ptr<T>(new T);
}

template <typename T>
requires std::is_unbounded_array_v<T>
inline sized_unique_ptr<T> make_unique_for_overwrite(std::size_t n) {
  using V = std::remove_extent_t<T>;
  V* p = static_cast<V*>(::operator new(n * sizeof(V)));
  try {
    std::uninitialized_default_construct_n(p, n);
  } catch (...) {
    ::operator delete(p, n * sizeof(V));
    throw;
  }
  return sized_unique_ptr<T>(p, SizedArrayDeleter{n * sizeof(V)});
}

} // namespace cbu_memory
} // namespace cbu
