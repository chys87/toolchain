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

#include <new>

namespace cbu {

struct ReNew {
  void* ptr;
  ReNew() = delete;
  explicit constexpr ReNew(void* p) : ptr{p} {}
  ReNew(const ReNew &) = default;
};

void* new_realloc(void* ptr, std::size_t n);
void* new_aligned_realloc(void* ptr, std::size_t n, std::size_t alignment);

}  // namespace cbu

inline void* operator new(std::size_t n, cbu::ReNew nr) {
  return cbu::new_realloc(nr.ptr, n);
}

inline void* operator new[](std::size_t n, cbu::ReNew nr) {
  return operator new(n, nr);
}

inline void* operator new(std::size_t n, cbu::ReNew nr,
                          std::align_val_t alignment) {
  return cbu::new_aligned_realloc(nr.ptr, n, std::size_t(alignment));
}

inline void* operator new[](std::size_t n, cbu::ReNew nr,
                            std::align_val_t alignment) {
  return operator new(n, nr, alignment);
}

// FIXME: Although we write the following ones, we will still
// get errors if the constructor actually throws.
// We recommend usage with POD only.
inline void operator delete(void* ptr, cbu::ReNew) noexcept {
  operator delete(ptr);
}

inline void operator delete[](void* ptr, cbu::ReNew) noexcept {
  operator delete[](ptr);
}

inline void operator delete(void* ptr, cbu::ReNew, std::align_val_t) noexcept {
  operator delete(ptr);
}

inline void operator delete[](void* ptr, cbu::ReNew,
                              std::align_val_t) noexcept {
  operator delete[](ptr);
}
