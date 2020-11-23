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

#include <pthread.h>
#include <type_traits>
#include "cbu/malloc/private.h"

namespace cbu {
inline namespace cbu_malloc {

struct ThreadCache {
  enum struct Status : unsigned char {
    INITIAL = 0,
    NORMAL = 1,
    FINAL = 2,
  };
  Status status {Status::INITIAL};

  DescriptionThreadCache description {};
  PageThreadCache page {};
  ThreadSmallCache small {};

  void destroy() noexcept {
    small.destroy();
    page.destroy();
    description.destroy();
  }

  bool is_active() const noexcept { return status == Status::NORMAL; }

  // The caller must react properly when prepare returns false.
  // On the one hand, single-threaded applications don't need to use prepare;
  // On the other hand, the order in which pthread_key destructors are called
  // is unspecified, so we can't expect that prepare will never be called after
  // its destruction.
  bool prepare() noexcept {
    if (status == Status::NORMAL)
      return true;
    do_prepare();
    return (status == Status::NORMAL);
  }

  void do_prepare() noexcept;
};

extern __thread ThreadCache thread_cache;

} // namespace cbu_malloc
} // namespace cbu
