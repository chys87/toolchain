/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2022, chys <admin@CHYS.INFO>
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
#include <new>

#include "cbu/common/byte_size.h"
#include "cbu/common/type_traits.h"
#include "cbu/compat/compilers.h"

namespace cbu {

// Convenient low-level new and delete
// These functions only allocates and deallocates raw memory, without invoking
// constructor or destructor
template <typename T>
constexpr T* raw_scalar_new() {
  void* p;
  if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
    p = ::operator new(sizeof(T), std::align_val_t(alignof(T)));
  else
    p = ::operator new(sizeof(T));
  return static_cast<T*>(p);
}

template <typename T>
constexpr T* raw_array_new(
    ByteSize<T> n, std::align_val_t align = std::align_val_t(alignof(T))) {
  void* p;
  if (std::size_t(align) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
    p = ::operator new[](n.bytes(), align);
  else
    p = ::operator new[](n.bytes());
  return static_cast<T*>(p);
}

template <typename T>
constexpr void raw_scalar_delete(T* p) noexcept {
#ifdef __cpp_sized_deallocation
  if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
    ::operator delete(p, sizeof(T), std::align_val_t(alignof(T)));
  else
    ::operator delete(p, sizeof(T));
#else
  if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
    ::operator delete(p, std::align_val_t(alignof(T)));
  else
    ::operator delete(p);
#endif
}

template <typename T>
constexpr void raw_array_delete(
    T* p, ByteSize<std::type_identity_t<T>> n [[maybe_unused]],
    std::align_val_t align = std::align_val_t(alignof(T))) noexcept {
#ifdef __cpp_sized_deallocation
  if (std::size_t(align) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
    ::operator delete[](p, n.bytes(), align);
  else
    ::operator delete[](p, n.bytes());
#else
  if (std::size_t(align) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
    ::operator delete[](p, align);
  else
    ::operator delete[](p);
#endif
}

// Same as std::destroy_n, but does destructin in reverse order
template <typename T, typename Size>
constexpr void destroy_backward_n(T* begin, Size size) {
  if constexpr (std::is_trivially_destructible_v<T>) return;
  T* p = begin + size;
  while (p != begin) std::destroy_at(--p);
}

// Deleters
template <typename T>
using ScalarDeleter = std::default_delete<T>;

// Note that this class can only be used with arrays allocated with
// new_and_default_init_array/new_and_value_init_array, not with new T[n]
template <typename T, std::align_val_t align = std::align_val_t(alignof(T))>
struct ArrayDeleter {
  ByteSize<T> size;
  constexpr void operator()(T* p) const noexcept {
    destroy_backward_n(p, size);
    raw_array_delete(p, size, align);
  }
};

template <typename T>
struct RawScalarDeleter {
  CBU_STATIC_CALL constexpr void operator()(T* p)
      CBU_STATIC_CALL_CONST noexcept {
    raw_scalar_delete(p);
  }
};

template <typename T, std::align_val_t align = std::align_val_t(alignof(T))>
struct RawArrayDeleter {
  ByteSize<T> size;
  constexpr void operator()(T* p) const noexcept {
    raw_array_delete(p, size, align);
  }
};

// Destruct, and calls RawDeleter to free up memory
template <typename T, typename Size, typename RawDeleter>
constexpr void destroy_backward_and_delete_n(
    T* begin, Size n, RawDeleter deleter = RawDeleter()) noexcept {
  destroy_backward_n(begin, n);
  deleter(begin);
}

// new_and_default_init_array, new_and_value_init_array:
// Please note that the allocated memory can only be deleted by our
// ArrayDeleter, not by delete[]
template <typename T>
inline T* new_and_default_init_array(
    std::size_t n, std::align_val_t align = std::align_val_t(alignof(T))) {
  T* p = raw_array_new<T>(n, align);
  try {
    std::uninitialized_default_construct_n(p, n);
  } catch (...) {
    raw_array_delete(p, n, align);
    throw;
  }
  return p;
}

template <typename T>
inline T* new_and_value_init_array(
    std::size_t n, std::align_val_t align = std::align_val_t(alignof(T))) {
  T* p = raw_array_new<T>(n);
  try {
    std::uninitialized_value_construct_n(p, n);
  } catch (...) {
    raw_array_delete(p, n, align);
    throw;
  }
  return p;
}

// uninitialized_move_and_destroy: Move construct and destroy old object
// It's the caller's responsibility to guarantee that old_obj and new_obj
// are different pointers.
template <typename T>
constexpr void uninitialized_move_and_destroy(T* old_obj, T* new_obj) noexcept {
  static_assert(std::is_nothrow_destructible_v<T>,
                "uninitialized_move_and_destroy is safe only if the type's "
                "destructor never throws.");
  if constexpr (bitwise_movable_v<T>) {
    if !consteval {
      __builtin_memcpy(static_cast<void*>(new_obj),
                       static_cast<const void*>(old_obj), sizeof(T));
      return;
    }
  }
  std::construct_at(new_obj, std::move(*old_obj));
  std::destroy_at(old_obj);
}

// It's the caller's responsibility to guarantee that memory blocks don't
// overlap.
template <typename T>
constexpr T* uninitialized_move_and_destroy_n(
    T* old_ptr, cbu::ByteSize<std::type_identity_t<T>> size,
    T* new_ptr) noexcept {
  static_assert(std::is_nothrow_move_constructible_v<T> &&
                    std::is_nothrow_destructible_v<T>,
                "uninitialized_move_and_destroy_n is safe only if the type's "
                "move constructor and destructor never throw.");
  if constexpr (bitwise_movable_v<T>) {
    if !consteval {
      __builtin_memcpy(static_cast<void*>(new_ptr),
                       static_cast<const void*>(old_ptr), size.bytes());
      return new_ptr + size;
    }
  }
  while (size) {
    uninitialized_move_and_destroy(old_ptr, new_ptr);
    ++old_ptr;
    ++new_ptr;
    --size;
  }
  return new_ptr;
}

template <typename T>
requires std::is_unbounded_array_v<T>
using sized_unique_ptr =
    std::unique_ptr<T, ArrayDeleter<std::remove_extent_t<T>>>;

template <typename T, typename... Args>
requires (!std::is_array_v<T>)
inline std::unique_ptr<T> make_unique(Args&&... args) {
  return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T>
requires std::is_unbounded_array_v<T>
inline sized_unique_ptr<T> make_unique(std::size_t n) {
  using V = std::remove_extent_t<T>;
  return sized_unique_ptr<T>(new_and_value_init_array<V>(n),
                             ArrayDeleter<V>{n});
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
  return sized_unique_ptr<T>(new_and_default_init_array<V>(n),
                             ArrayDeleter<V>{n});
}

template <typename T, typename... Args>
requires (!std::is_array_v<T>)
inline std::shared_ptr<T> make_shared(Args&&... args) {
  return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename T>
requires std::is_unbounded_array_v<T>
inline std::shared_ptr<T> make_shared(std::size_t n) {
  using V = std::remove_extent_t<T>;
  return std::shared_ptr<T>(new_and_value_init_array<V>(n),
                            ArrayDeleter<V>{n});
}

template <typename T>
requires (!std::is_array_v<T>)
inline std::shared_ptr<T> make_shared_for_overwrite() {
  // GCC 9 doesn't have std::make_shared_for_overwrite yet.
  // return std::make_shared_for_overwrite<T>();
  return std::shared_ptr<T>(new T);
}

template <typename T>
requires std::is_unbounded_array_v<T>
inline std::shared_ptr<T> make_shared_for_overwrite(std::size_t n) {
  using V = std::remove_extent_t<T>;
  return std::shared_ptr<T>(new_and_default_init_array<V>(n),
                            ArrayDeleter<V>{n});
}

// OutlinableArray<T> is like array<T> or unique_ptr<T[]>, but allocates
// either on stack or heap depending on size.  It is somewhat a simplified
// version of absl::InlinedVector.
//
// Usage:
//  OutlinableArrayBuffer<T, N> buffer;
//  OutlinableArray<T> array(&buffer, n);
//  (or, OutlinableArray<T> array(&buffer, n,
//                                OutlinableArray<T>::for_overwrite))
//
// The purpose of not embedding buffer in array is for better compiler
// optimization.
template <typename T> class OutlinableArray;

template <typename T, std::size_t N>
struct OutlinableArrayBuffer {
  alignas(T) char buffer[sizeof(T) * N];
};

template <typename T>
class OutlinableArray {
 public:
  struct ForOverwriteTag {};
  static constexpr ForOverwriteTag for_overwrite{};

  using value_type = T;
  using reference = T&;
  using pointer = T*;
  using iterator = T*;

  template <std::size_t N>
  constexpr OutlinableArray(OutlinableArrayBuffer<T, N>* buffer, std::size_t n)
      : ptr_(new (get_pointer(buffer, n)) T[n]()),
        size_(n),
        allocated_(n <= N ? 0 : n) {}

  template <std::size_t N>
  constexpr OutlinableArray(OutlinableArrayBuffer<T, N>* buffer, std::size_t n,
                            ForOverwriteTag)
      : ptr_(new (get_pointer(buffer, n)) T[n]), allocated_(n <= N ? 0 : n) {}

  OutlinableArray(const OutlinableArray&) = delete;
  OutlinableArray& operator=(const OutlinableArray&) = delete;

  constexpr ~OutlinableArray() noexcept {
    if constexpr (!std::is_trivially_destructible_v<T>)
      destuct_n(ptr_, size_);
    if (allocated_) raw_array_delete(ptr_, size_);
  }

  constexpr T* get() const noexcept { return ptr_; }
  constexpr T* data() const noexcept { return ptr_; }
  constexpr std::size_t size() const noexcept { return size_; }
  constexpr T* begin() const noexcept { return ptr_; }
  constexpr T* end() const noexcept { return ptr_ + size_; }
  constexpr T* cbegin() const noexcept { return ptr_; }
  constexpr T* cend() const noexcept { return ptr_ + size_; }
  constexpr T& operator[](std::size_t k) const noexcept { return ptr_[k]; }

 private:
  template <std::size_t N>
  constexpr void* get_pointer(OutlinableArrayBuffer<T, N>* buffer,
                              std::size_t n) {
    if (n <= N)
      return buffer->buffer;
    else
      return raw_array_new<T>(n);
  }

 private:
  T* ptr_;
  std::size_t size_;
  bool allocated_;
};

}  // namespace cbu
