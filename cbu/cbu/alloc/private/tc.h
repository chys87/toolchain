/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2021, chys <admin@CHYS.INFO>
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

#include <mutex>
#include <type_traits>

#include "cbu/alloc/pagesize.h"
#include "cbu/alloc/private/common.h"
#include "cbu/fsyscall/fsyscall.h"
#include "cbu/sys/low_level_mutex.h"

namespace cbu {
namespace alloc {

// Try to make each active thread use a fixed cache, so that there is minimal
// race.
inline thread_local uint32_t g_thread_cache_idx = 0;
inline std::atomic<uint32_t> g_used_max_concurrency{0};

constexpr uint32_t kHardMaxConcurrency = 128;

template <typename CacheClass>
struct CachePool {
  struct alignas(kCacheLineSize) Node {
    CacheClass cache;
    [[no_unique_address]] LowLevelMutex mutex;
  };
#ifndef CBU_SINGLE_THREADED
  Node nodes[kHardMaxConcurrency];
#endif
};

template <typename CacheClass>
class UniqueCache {
 public:
  explicit UniqueCache(CachePool<CacheClass>* pool) noexcept
      : ptr_(grab(pool)) {}
  ~UniqueCache() noexcept;

  explicit operator bool() const noexcept { return ptr_; }
  CacheClass* get() const noexcept { return &ptr_->cache; }
  CacheClass& operator*() const noexcept { return ptr_->cache; }
  CacheClass* operator->() const noexcept { return &ptr_->cache; }

  template <typename Callback>
  static void visit_all(CachePool<CacheClass>* pool, Callback callback);

 private:
  using Node = typename CachePool<CacheClass>::Node;
  static Node* grab(CachePool<CacheClass>* pool) noexcept;

 private:
  Node* ptr_;
};

template <typename CacheClass>
UniqueCache<CacheClass>::~UniqueCache() noexcept {
  if (ptr_) ptr_->mutex.unlock();
}

template <typename CacheClass>
template <typename Callback>
void UniqueCache<CacheClass>::visit_all(CachePool<CacheClass>* pool,
                                        Callback callback) {
#ifndef CBU_SINGLE_THREADED
  uint32_t k = g_used_max_concurrency.load(std::memory_order_relaxed);
  while (k--) {
    std::lock_guard lock(pool->nodes[k].mutex);
    callback(&pool->nodes[k].cache);
  }
#endif
}

template <typename CacheClass>
typename UniqueCache<CacheClass>::Node*
UniqueCache<CacheClass>::grab(CachePool<CacheClass>* pool) noexcept {
#ifdef CBU_SINGLE_THREADED
  return nullptr;
#else
  uint32_t max_concurrency =
      g_used_max_concurrency.load(std::memory_order_relaxed);
  uint32_t k = g_thread_cache_idx;

  // Fast path
  if (k < max_concurrency && pool->nodes[k].mutex.try_lock())
    return &pool->nodes[k];

  // Slow path
  for (;;) {
    for (uint32_t remaining = max_concurrency; remaining; --remaining) {
      if (++k >= max_concurrency) k = 0;
      if (pool->nodes[k].mutex.try_lock()) {
        g_thread_cache_idx = k;
        return &pool->nodes[k];
      }
    }

    if (max_concurrency >= kHardMaxConcurrency) {
      fsys_sched_yield();
      continue;
    }

    if (g_used_max_concurrency.compare_exchange_strong(
            max_concurrency, max_concurrency + 1, std::memory_order_relaxed,
            std::memory_order_relaxed))
      ++max_concurrency;
  }
#endif
}

}  // namespace alloc
}  // namespace cbu
