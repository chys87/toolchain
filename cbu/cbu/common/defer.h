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

#pragma once

#include <utility>

namespace cbu {

template <typename F>
class Defer {
 public:
  explicit Defer(F f) noexcept : f_(std::move(f)) {}

  Defer(const Defer&) = delete;
  void operator=(const Defer&) = delete;

  [[gnu::always_inline]] ~Defer() { f_(); }

 private:
  F f_;
};

namespace defer_detail {

class Maker {
 public:
  template <typename T>
  Defer<T> operator|(T func) noexcept(noexcept(std::move(func))) {
    return Defer<T>(std::move(func));
  }
};

}  // namespace defer_detail

}  // namespace cbu

#define CBU_DEFER CBU_DEFER_(__COUNTER__)
#define CBU_DEFER_(cnt) CBU_DEFER__(cnt)
#define CBU_DEFER__(cnt)                       \
  [[maybe_unused]] auto cbu_RAII_defer_##cnt = \
      ::cbu::defer_detail::Maker() | [&]() noexcept -> void
