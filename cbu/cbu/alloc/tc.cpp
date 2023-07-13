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

#ifndef CBU_SINGLE_THREADED

#include "cbu/alloc/private/tc.h"

#include <pthread.h>

#include <atomic>

#include "cbu/common/procutil.h"

namespace cbu::alloc {
namespace {

std::atomic<TcStatus> g_tc_status{TcStatus::kInitial};
pthread_key_t g_tc_key;
constinit thread_local ThreadCache t_tc;

static_assert(std::is_trivially_destructible_v<ThreadCache>);

void TcDestroy(void* arg) {
  ThreadCache* tc = static_cast<ThreadCache*>(arg);
  if (!tc || tc->status != TcStatus::kReady) return;

  tc->small_cache.clear();

#ifndef CBU_NO_BRK
  tc->page_category_cache_brk.clear(&arena_brk);
#endif
  tc->page_category_cache.clear(&arena_mmap);
  if constexpr (kTHPSize > 0)
    tc->page_category_cache_no_thp.clear(&arena_mmap_no_thp);

  tc->description_cache.clear();
}

bool TcSetUp() {
  TcStatus tc_status = g_tc_status.load(std::memory_order_acquire);
  switch (tc_status) {
    case TcStatus::kInitial:
      if (g_tc_status.compare_exchange_strong(tc_status, TcStatus::kSettingUp,
                                              std::memory_order_relaxed,
                                              std::memory_order_acquire)) {
        int ret = pthread_key_create(&g_tc_key, &TcDestroy);
        if (ret != 0) [[unlikely]]
          cbu::fatal<"pthread_key_create failed">();
        g_tc_status.store(TcStatus::kReady, std::memory_order_release);
        return true;
      } else {
        return tc_status == TcStatus::kReady;
      }
    case TcStatus::kSettingUp:
    default:
      // It may happen if pthread_key_create calls malloc.
      // Also possible is that another thread is initializing, but that's not
      // important, since most likely malloc is called before a thread is ever
      // created.
      return false;
    case TcStatus::kReady:
      return true;
  }
}

}  // namespace

ThreadCache* get_or_create_thread_cache() noexcept {
  ThreadCache* tc = &t_tc;
  switch (tc->status) {
    case TcStatus::kInitial: {
      if (!TcSetUp()) return nullptr;
      tc->status = TcStatus::kSettingUp;
      int ret = pthread_setspecific(g_tc_key, tc);
      if (ret != 0) [[unlikely]]
        cbu::fatal<"pthread_setspecific failed">();
      tc->status = TcStatus::kReady;
      break;
      }
    case TcStatus::kSettingUp:
    default:
      // In case pthread_setspecific calls malloc (it happens!)
      return nullptr;
    case TcStatus::kReady:
      break;
  }
  return tc;
}

ThreadCache* get_thread_cache() noexcept {
  ThreadCache* tc = &t_tc;
  if (tc->status != TcStatus::kReady) return nullptr;
  return tc;
}

}  // namespace cbu::alloc

#endif  // !CBU_SINGLE_THREADED
