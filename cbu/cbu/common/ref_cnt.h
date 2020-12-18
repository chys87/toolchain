/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020, chys <admin@CHYS.INFO>
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

#include <atomic>
#include <utility>

#include "cbu/tweak/tweak.h"

namespace cbu {
inline namespace cbu_ref_cnt {

using ref_cnt_t = unsigned;
// Assume single-threaded
constexpr int REF_CNT_SINGLE_THREADED = 1;
// Allow ref_cnt_dec to neglect writing 0 to the destination if it returns true
constexpr int REF_CNT_MAY_NOT_WRITE_ZERO = 2;

// Increase a ref_cnt_t
inline void ref_cnt_inc(ref_cnt_t *p, int options = 0) noexcept {
  if (!(options & REF_CNT_SINGLE_THREADED) &&
      !cbu::tweak::SINGLE_THREADED &&
      std::atomic_ref(*p).load(std::memory_order_relaxed) >= 2) {
    std::atomic_ref(*p).fetch_add(1, std::memory_order_acquire);
  } else {
    ++*p;
  }
}

// Decrease a ref_cnt_t, and returns whether it's been decreased to zero
inline bool ref_cnt_dec(ref_cnt_t* p, int options) noexcept {
  if ((options & REF_CNT_SINGLE_THREADED) || cbu::tweak::SINGLE_THREADED) {
    return --*p == 0;
  }
  if (std::atomic_ref(*p).load(std::memory_order_relaxed) >= 2) {
    return std::atomic_ref(*p).fetch_sub(1, std::memory_order_release) - 1 == 0;
  }
  if (!(options & REF_CNT_MAY_NOT_WRITE_ZERO))
    std::atomic_ref(*p).store(0, std::memory_order_release);
  return true;
}

// Helper type for RefCntPtr and intrusive_ptr constructors
template <typename T = void>  // void is interpreted as default
struct Construct {
  template <typename DefaultType> using type = T;
};

template <>
struct Construct<void> {
  template <typename DefaultType> using type = DefaultType;
};

// A shared_ptr-like but simpler.
// It doesn't support consturcting a derived type.
// Nor does it support other advanced features of shared_ptr.
template <typename T, int OPTIONS = 0>
class RefCntPtr {
 public:
  constexpr RefCntPtr(decltype(nullptr) = nullptr) noexcept
      : p_(nullptr) {}

  // Can only use Construct<> (aka Construct<void>) or Construct<T>
  // in RefCntPtr.
  template <typename... Args>
  RefCntPtr(Construct<>, Args&&... args)
      : p_(new Node{T(std::forward<Args>(args)...), 1}) {}

  // The same.
  template <typename... Args>
  RefCntPtr(Construct<T>, Args&&... args)
      : p_(new Node{T(std::forward<Args>(args)...), 1}) {}

  // Copy etc.
  RefCntPtr(const RefCntPtr& other) noexcept
      : p_(other.p_) {
    if (p_)
      ref_cnt_inc(&p_->cnt, OPTIONS);
  }
  constexpr RefCntPtr(RefCntPtr&& other) noexcept
      : p_(std::exchange(other.p_, nullptr)) {}

  RefCntPtr& operator=(const RefCntPtr& other) noexcept {
    if (this != &other) {
      dec();
      p_ = other.p_;
      if (p_)
        ref_cnt_inc(&p_->cnt, OPTIONS);
    }
    return *this;
  }
  RefCntPtr& operator=(RefCntPtr&& other) noexcept {
    swap(other);
    return *this;
  }
  void swap(RefCntPtr& other) noexcept { std::swap(p_, other.p_); }
  ~RefCntPtr() noexcept { dec(); }

  explicit constexpr operator bool() const noexcept { return p_; }

  T* get() noexcept { return p_ ? &p_->v : nullptr; }
  const T* get() const noexcept { return p_ ? &p_->v : nullptr; }

  T& operator*() noexcept { return p_->v; }
  const T& operator*() const noexcept { return p_->v; }

  T* operator->() noexcept { return &p_->v; }
  const T* operator->() const noexcept { return &p_->v; }

 private:
  void dec() noexcept {
    if (p_ && ref_cnt_dec(&p_->cnt, OPTIONS | REF_CNT_MAY_NOT_WRITE_ZERO))
      delete p_;
  }

 private:
  struct Node {
    T v;
    ref_cnt_t cnt;
  };
  Node* p_;
};

} // inline namespace cbu_ref_cnt
} // namespace cbu
