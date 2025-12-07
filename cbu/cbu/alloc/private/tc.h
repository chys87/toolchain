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

#include <pthread.h>
#include <sched.h>

#include <type_traits>

#include "cbu/alloc/pagesize.h"
#include "cbu/alloc/private/page.h"
#include "cbu/alloc/private/small.h"

namespace cbu {
namespace alloc {

enum class TcStatus : unsigned {
  kInitial = 0,
  kReady = 1,
  kSettingUp = 2,
};

struct ThreadCache {
  struct Dummy {};

  DescriptionCache description_cache;

#ifndef CBU_NO_BRK
  PageCategoryCache page_category_cache_brk;
#endif
  PageCategoryCache page_category_cache;
  [[no_unique_address]] std::conditional_t<(kTHPSize > 0), PageCategoryCache,
                                           Dummy>
      page_category_cache_no_thp;

  SmallCache small_cache;

  TcStatus status = TcStatus::kInitial;
};

#ifdef CBU_SINGLE_THREADED

constexpr ThreadCache* get_or_create_thread_cache() noexcept { return nullptr; }
constexpr ThreadCache* get_thread_cache() noexcept { return nullptr; }

#else

ThreadCache* get_or_create_thread_cache() noexcept;
ThreadCache* get_thread_cache() noexcept;

#endif

}  // namespace alloc
}  // namespace cbu
