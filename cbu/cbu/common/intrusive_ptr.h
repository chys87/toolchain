/*
 * cbu - chys's basic utilities
 * Copyright (c) 2026, chys <admin@CHYS.INFO>
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
 * (INCLUDING BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <type_traits>
#include <utility>

#include "cbu/common/ref_cnt.h"  // IWYU pragma: keep
#include "cbu/common/tags.h"
#include "cbu/compat/compilers.h"

namespace cbu {

// Our intrusive_ptr is a bit different than boost::intrusive_ptr
// We define intrusive_ptr_add_ref and intrusive_ptr_release as member functions

namespace detail {

template <typename T>
concept HasAddRefNoRace = requires(T* ptr) {
  { ptr->intrusive_ptr_add_ref_no_race() };
};

inline void add_ref_no_race(HasAddRefNoRace auto* ptr) noexcept {
  ptr->intrusive_ptr_add_ref_no_race();
}

template <typename T>
  requires(!HasAddRefNoRace<T>)
inline void add_ref_no_race(T* ptr) noexcept {
  ptr->intrusive_ptr_add_ref();
}

}  // namespace detail

template <typename T>
class CBU_TRIVIAL_ABI intrusive_ptr {
 public:
  constexpr intrusive_ptr() noexcept : ptr_(nullptr) {}
  constexpr intrusive_ptr(decltype(nullptr)) noexcept : ptr_(nullptr) {}
  intrusive_ptr(T* ptr) noexcept : ptr_(ptr) {
    if (ptr) ptr->intrusive_ptr_add_ref();
  }

  constexpr intrusive_ptr(UnsafeTag, T* ptr) noexcept : ptr_(ptr) {}

  template <std::derived_from<T> SubType, typename... Args>
  intrusive_ptr(std::in_place_type_t<SubType>, Args&&... args)
      : ptr_(new SubType(std::forward<Args>(args)...)) {
    detail::add_ref_no_race(ptr_);
  }

  template <typename... Args>
  intrusive_ptr(std::in_place_t, Args&&... args)
      : ptr_(new T(std::forward<Args>(args)...)) {
    detail::add_ref_no_race(ptr_);
  }

  intrusive_ptr(const intrusive_ptr& other) noexcept : ptr_(other.ptr_) {
    if (ptr_) ptr_->intrusive_ptr_add_ref();
  }

  intrusive_ptr(intrusive_ptr&& other) noexcept
      : ptr_(std::exchange(other.ptr_, nullptr)) {}

  // Copy/move from a derived type
  template <typename SubType>
    requires(!std::is_same_v<T, SubType> && std::is_base_of_v<T, SubType>)
  intrusive_ptr(const intrusive_ptr<SubType>& other) noexcept
      : ptr_(other.ptr_) {
    if (ptr_) ptr_->intrusive_ptr_add_ref();
  }

  template <typename SubType>
    requires(!std::is_same_v<T, SubType> && std::is_base_of_v<T, SubType>)
  intrusive_ptr(intrusive_ptr<SubType>&& other) noexcept
      : ptr_(std::exchange(other.ptr_, nullptr)) {}

  [[gnu::always_inline]] constexpr ~intrusive_ptr() noexcept {
    if (ptr_) ptr_->intrusive_ptr_release();
  }

  intrusive_ptr& operator=(const intrusive_ptr& other) noexcept {
    intrusive_ptr(other).swap(*this);
    return *this;
  }
  intrusive_ptr& operator=(intrusive_ptr&& other) noexcept {
    other.swap(*this);
    return *this;
  }

  template <typename SubType>
    requires(!std::is_same_v<T, SubType> && std::is_base_of_v<T, SubType>)
  intrusive_ptr& operator=(const intrusive_ptr<SubType>& other) noexcept {
    intrusive_ptr(other).swap(*this);
    return *this;
  }

  template <typename SubType>
    requires(!std::is_same_v<T, SubType> && std::is_base_of_v<T, SubType>)
  intrusive_ptr& operator=(intrusive_ptr<SubType>&& other) noexcept {
    intrusive_ptr(std::move(other)).swap(*this);
    return *this;
  }

  void swap(intrusive_ptr& other) noexcept { std::swap(ptr_, other.ptr_); }

  void reset() noexcept {
    if (ptr_) std::exchange(ptr_, nullptr)->intrusive_ptr_release();
  }

  template <typename... Args>
  void renew(Args&&... args) {
    reset();
    ptr_ = new T(std::forward<Args>(args)...);
    detail::add_ref_no_race(ptr_);
  }

  void reset_unsafe(T* ptr) noexcept { ptr_ = ptr; }

  void reset_unsafe(intrusive_ptr&& other) noexcept {
    ptr_ = std::exchange(other.ptr_, nullptr);
  }

  explicit constexpr operator bool() const noexcept { return ptr_; }

  constexpr T* get() const noexcept { return ptr_; }

  T* release_unsafe() noexcept { return std::exchange(ptr_, nullptr); }

  T& operator*() const noexcept { return *ptr_; }

  constexpr T* operator->() const noexcept { return ptr_; }

 public:
  static constexpr bool bitwise_movable(intrusive_ptr*) noexcept {
    return true;
  }

  template <typename>
  friend class intrusive_ptr;

 private:
  T* ptr_;
};

#define CBU_DECLARE_INTRUSIVE_PTR_STRUCT_EX(ClassName, DeleteStmt)       \
  void intrusive_ptr_add_ref() noexcept { cbu::ref_cnt_inc(&ref_cnt_); } \
  void intrusive_ptr_add_ref_no_race() noexcept { ++ref_cnt_; }          \
  void intrusive_ptr_release() noexcept {                                \
    if (cbu::ref_cnt_dec<cbu::REF_CNT_MAY_NOT_WRITE_ZERO>(&ref_cnt_)) {  \
      DeleteStmt;                                                        \
    }                                                                    \
  }                                                                      \
  cbu::ref_cnt_t ref_cnt_{0}

#define CBU_DECLARE_INTRUSIVE_PTR_CLASS_EX(ClassName, DeleteStmt)        \
 public:                                                                 \
  ClassName(const ClassName&) = delete;                                  \
  ClassName& operator=(const ClassName&) = delete;                       \
                                                                         \
  void intrusive_ptr_add_ref() noexcept { cbu::ref_cnt_inc(&ref_cnt_); } \
  void intrusive_ptr_add_ref_no_race() noexcept { ++ref_cnt_; }          \
                                                                         \
  void intrusive_ptr_release() noexcept {                                \
    if (cbu::ref_cnt_dec<cbu::REF_CNT_MAY_NOT_WRITE_ZERO>(&ref_cnt_)) {  \
      DeleteStmt;                                                        \
    }                                                                    \
  }                                                                      \
                                                                         \
 private:                                                                \
  cbu::ref_cnt_t ref_cnt_{0}

#define CBU_DECLARE_INTRUSIVE_PTR_STRUCT(ClassName) \
  CBU_DECLARE_INTRUSIVE_PTR_STRUCT_EX(ClassName, delete this)
#define CBU_DECLARE_INTRUSIVE_PTR_CLASS(ClassName) \
  CBU_DECLARE_INTRUSIVE_PTR_CLASS_EX(ClassName, delete this)

}  // namespace cbu
