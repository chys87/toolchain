/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2026, chys <admin@CHYS.INFO>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <concepts>
#include <cstddef>
#include <memory>
#include <new>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

#include "cbu/common/byte_size.h"
#include "cbu/strings/str_builder.h"
#include "cbu/strings/zstring_view.h"
#include "cbu/tweak/tweak.h"

namespace cbu {

inline constexpr std::size_t kArenaMaxAlign = 64;

template <typename T>
concept Arena_supported_type =
    std::is_object_v<T> && !std::is_const_v<T> && !std::is_volatile_v<T> &&
    requires { alignof(T); } && alignof(T) <= kArenaMaxAlign &&
    (!std::is_array_v<T> || std::is_bounded_array_v<T>);

template <typename T>
concept Arena_supported_array = std::is_unbounded_array_v<T> &&
                                Arena_supported_type<std::remove_extent_t<T>>;

namespace arena_detail {

struct ArenaDeleterNode {
  ArenaDeleterNode* next;
};

struct ArenaDeleterArrayNode {
  ArenaDeleterArrayNode* next;
  std::size_t size;
};

}  // namespace arena_detail

// Destructs the non-trivially destructible objects allocated from one arena.
//
// The caller must use an ArenaDeleter only with one Arena, and must destroy
// it before that Arena.
template <typename T>
class ArenaDeleter;

template <typename T>
  requires Arena_supported_type<T>
class ArenaDeleter<T> {
 public:
  constexpr ArenaDeleter() noexcept = default;

  ArenaDeleter(const ArenaDeleter&) = delete;
  ArenaDeleter& operator=(const ArenaDeleter&) = delete;

  ~ArenaDeleter() noexcept(std::is_nothrow_destructible_v<T>) {
    while (arena_detail::ArenaDeleterNode* node = head_) {
      auto* next = node->next;
      head_ = next;
      std::byte* object = reinterpret_cast<std::byte*>(node) - sizeof(T);
      if constexpr (std::is_bounded_array_v<T>) {
        using U = std::remove_extent_t<T>;
        U* array = reinterpret_cast<U*>(object);
        U* p = array + std::extent_v<T>;
        while (p != array) std::destroy_at(--p);
      } else {
        std::destroy_at(reinterpret_cast<T*>(object));
      }
    }
  }

 private:
  void register_node(arena_detail::ArenaDeleterNode* node) noexcept {
    head_ = ::new (node) arena_detail::ArenaDeleterNode{head_};
  }

  arena_detail::ArenaDeleterNode* head_ = nullptr;

  friend class Arena;
};

template <typename T>
  requires Arena_supported_type<T>
class ArenaDeleter<T[]> {
 public:
  using element_type = T;

  constexpr ArenaDeleter() noexcept = default;

  ArenaDeleter(const ArenaDeleter&) = delete;
  ArenaDeleter& operator=(const ArenaDeleter&) = delete;

  ~ArenaDeleter() noexcept(std::is_nothrow_destructible_v<element_type>) {
    while (arena_detail::ArenaDeleterArrayNode* node = head_) {
      auto* next = node->next;
      head_ = next;
      std::size_t size = node->size;
      std::byte* p = reinterpret_cast<std::byte*>(node);
      while (size != 0) {
        p -= sizeof(element_type);
        std::destroy_at(reinterpret_cast<element_type*>(p));
        --size;
      }
    }
  }

 private:
  void register_node(arena_detail::ArenaDeleterArrayNode* node,
                     std::size_t size) noexcept {
    head_ = ::new (node) arena_detail::ArenaDeleterArrayNode{head_, size};
    head_ = node;
  }

  arena_detail::ArenaDeleterArrayNode* head_ = nullptr;

  friend class Arena;
};

// This is inspired by google::protobuf::Arena, but provides much simpler
// functionalities.  It is intended to be used where the extra functionalities
// of google::protobuf::Arena are not required.
//
// Objects with non-trivial destructors must be allocated with an
// ArenaDeleter.  The deleter owns the destruction of those objects; the arena
// owns only their storage.
//
// Note that this Arena is *not* thread-safe.
class Arena {
 public:
  constexpr Arena() noexcept = default;
  constexpr Arena(Arena&& other) noexcept
      : active_(std::exchange(other.active_, nullptr)),
        list_(std::exchange(other.list_, nullptr)) {}

  ~Arena() noexcept;

  constexpr Arena& operator=(Arena&& other) noexcept {
    swap(other);
    return *this;
  }

  constexpr void swap(Arena& other) noexcept {
    std::swap(active_, other.active_);
    std::swap(list_, other.list_);
  }

  template <Arena_supported_type T>
    requires std::is_trivially_destructible_v<T>
  auto* Create() CBU_MEMORY_NOEXCEPT {
    using U = std::remove_extent_t<T>;
    return static_cast<U*>(CreateRaw(sizeof(T), alignof(T)));
  }

  template <Arena_supported_type T>
    requires(!std::is_trivially_destructible_v<T>)
  auto* Create(ArenaDeleter<T>& deleter) CBU_MEMORY_NOEXCEPT {
    using U = std::remove_extent_t<T>;
    auto [object, node] = CreateRawWithNode(sizeof(T), alignof(T));
    deleter.register_node(node);
    return static_cast<U*>(object);
  }

  // Create<T[]>(n).  ByteSize<T> is non-deduced.
  template <Arena_supported_array Array,
            typename T = std::remove_extent_t<Array>>
    requires std::is_trivially_destructible_v<T>
  T* Create(std::type_identity_t<cbu::ByteSize<T>> n) CBU_MEMORY_NOEXCEPT {
    return static_cast<T*>(CreateRaw(n.bytes(), alignof(T)));
  }

  // Create<T[]>(n, deleter), or Create(n, deleter) with the array type
  // deduced from the deleter.  The size is an element count.
  template <Arena_supported_array Array,
            typename T = std::remove_extent_t<Array>>
    requires(!std::is_trivially_destructible_v<T>)
  T* Create(std::size_t n, ArenaDeleter<Array>& deleter) CBU_MEMORY_NOEXCEPT {
    auto [object, node] = CreateRawWithArrayNode(n * sizeof(T), alignof(T));
    deleter.register_node(node, n);
    return static_cast<T*>(object);
  }

  // Create<T[]>(n, align).  ByteSize<T> is non-deduced.
  template <Arena_supported_array Array,
            typename T = std::remove_extent_t<Array>>
    requires std::is_trivially_destructible_v<T>
  T* Create(std::type_identity_t<cbu::ByteSize<T>> n,
            std::size_t align) CBU_MEMORY_NOEXCEPT {
    return static_cast<T*>(CreateRaw(n.bytes(), align));
  }

  // Create<T[]>(n, align, deleter), or Create(n, align, deleter) with the
  // array type deduced from the deleter.  The size is an element count.
  template <Arena_supported_array Array,
            typename T = std::remove_extent_t<Array>>
    requires(!std::is_trivially_destructible_v<T>)
  T* Create(std::size_t n, std::size_t align,
            ArenaDeleter<Array>& deleter) CBU_MEMORY_NOEXCEPT {
    auto [object, node] = CreateRawWithArrayNode(n * sizeof(T), align);
    deleter.register_node(node, n);
    return static_cast<T*>(object);
  }

  template <Arena_supported_type T, typename... Args>
    requires(!std::is_array_v<T> && std::is_trivially_destructible_v<T>)
  T* emplace(Args&&... args) noexcept(
      cbu::kMemoryNoExcept && std::is_nothrow_constructible_v<T, Args&&...>) {
    return std::construct_at(Create<T>(), std::forward<Args>(args)...);
  }

  template <Arena_supported_type T, typename... Args>
    requires(!std::is_array_v<T> && !std::is_trivially_destructible_v<T>)
  T* emplace(ArenaDeleter<T>& deleter, Args&&... args) noexcept(
      cbu::kMemoryNoExcept && std::is_nothrow_constructible_v<T, Args&&...>) {
    auto [object, node] = CreateRawWithNode(sizeof(T), alignof(T));
    T* result =
        std::construct_at(static_cast<T*>(object), std::forward<Args>(args)...);
    deleter.register_node(node);
    return result;
  }

  template <typename T>
    requires(Arena_supported_array<T[]> && std::is_trivially_destructible_v<T>)
  std::span<T>
  copy_n(const T* array, std::type_identity_t<cbu::ByteSize<T>> size) noexcept(
      cbu::kMemoryNoExcept && std::is_nothrow_copy_constructible_v<T>) {
    T* p = Create<T[]>(size);
    if consteval {
      for (decltype(size) i = 0; i != size; ++i) {
        std::construct_at(p + i, *(array + i));
      }
    } else {
      if constexpr (std::is_trivially_copy_constructible_v<T>) {
        __builtin_memcpy(p, array, size.bytes());
      } else {
        for (decltype(size) i = 0; i != size; ++i) {
          std::construct_at(p + i, *(array + i));
        }
      }
    }
    return std::span(p, size);
  }

  template <typename T>
    requires(Arena_supported_array<T[]> && !std::is_trivially_destructible_v<T>)
  std::span<T>
  copy_n(const T* array, std::size_t size, ArenaDeleter<T[]>& deleter) noexcept(
      cbu::kMemoryNoExcept && std::is_nothrow_copy_constructible_v<T>) {
    auto [object, node] = CreateRawWithArrayNode(size * sizeof(T), alignof(T));
    T* p = static_cast<T*>(object);
    constexpr bool no_throw =
        cbu::kMemoryNoExcept && std::is_nothrow_copy_constructible_v<T>;
    if constexpr (no_throw) {
      for (std::size_t i = 0; i != size; ++i) {
        std::construct_at(p + i, *(array + i));
      }
    } else {
      std::size_t constructed = 0;
      try {
        for (std::size_t i = 0; i != size; ++i, ++constructed) {
          std::construct_at(p + i, *(array + i));
        }
      } catch (...) {
        while (constructed != 0) std::destroy_at(p + --constructed);
        throw;
      }
    }
    deleter.register_node(node, size);
    return std::span(p, size);
  }

  template <typename T>
    requires requires(const T& cont) {
      requires Arena_supported_array<
          std::remove_cv_t<std::remove_cvref_t<decltype(*cont.data())>>[]>;
      { std::size(cont) } -> std::convertible_to<size_t>;
      requires std::is_trivially_destructible_v<
          std::remove_cv_t<std::remove_cvref_t<decltype(*cont.data())>>>;
    }
  auto copy_n(const T& cont) noexcept(
      cbu::kMemoryNoExcept &&
      std::is_nothrow_copy_constructible_v<
          std::remove_cv_t<std::remove_cvref_t<decltype(*cont.data())>>>) {
    return copy_n(std::data(cont), std::size(cont));
  }

  template <typename T>
    requires requires(const T& cont) {
      requires Arena_supported_array<
          std::remove_cv_t<std::remove_cvref_t<decltype(*cont.data())>>[]>;
      { std::size(cont) } -> std::convertible_to<size_t>;
      requires !std::is_trivially_destructible_v<
          std::remove_cv_t<std::remove_cvref_t<decltype(*cont.data())>>>;
    }
  auto copy_n(
      const T& cont,
      ArenaDeleter<
          std::remove_cv_t<std::remove_cvref_t<decltype(*cont.data())>>[]>&
          deleter) noexcept(cbu::kMemoryNoExcept &&
                            std::is_nothrow_copy_constructible_v<
                                std::remove_cv_t<std::remove_cvref_t<
                                    decltype(*cont.data())>>>) {
    return copy_n(std::data(cont), std::size(cont), deleter);
  }

  void* CreateRaw(std::size_t size, std::size_t align) CBU_MEMORY_NOEXCEPT;

  // Return is a no-op unless the returned memory is the whole or tail part of
  // the last allocation.
  // Deleter-backed allocations have a node after the object and therefore are
  // never eligible for Return.
  void Return(void* ptr, std::size_t n) noexcept;

  std::string_view PushString(const void* data,
                              size_t size) CBU_MEMORY_NOEXCEPT {
    return {PushStringImpl(data, size), size};
  }

  std::string_view PushString(std::string_view sv) CBU_MEMORY_NOEXCEPT {
    return PushString(sv.data(), sv.size());
  }

  // Copy a string to the arena, and add a null-terminator
  cbu::zstring_view PushZString(const void* data,
                                size_t size) CBU_MEMORY_NOEXCEPT {
    return {PushZStringImpl(data, size), size};
  }

  cbu::zstring_view PushZString(std::string_view sv) CBU_MEMORY_NOEXCEPT {
    return PushZString(sv.data(), sv.size());
  }

  // For use with cbu::sb
  template <cbu::sb::BaseBuilder Builder>
  std::string_view PushString(const Builder& builder) CBU_MEMORY_NOEXCEPT {
    if constexpr (cbu::sb::HasSize<Builder>) {
      size_t l = builder.size();
      char* p = Create<char[]>(l);
      builder.write(p);
      return {p, l};
    } else {
      size_t l = builder.max_size();
      char* p = Create<char[]>(l);
      char* e = builder.write(p);
      return {p, e};
    }
  }

  template <cbu::sb::BaseBuilder Builder>
  cbu::zstring_view PushZString(const Builder& builder) CBU_MEMORY_NOEXCEPT {
    if constexpr (cbu::sb::HasSize<Builder>) {
      size_t l = builder.size();
      char* p = Create<char[]>(l + 1);
      *builder.write(p) = '\0';
      return {p, l};
    } else {
      size_t l = builder.max_size();
      char* p = Create<char[]>(l + 1);
      char* e = builder.write(p);
      *e = '\0';
      return {p, e};
    }
  }

 private:
  union Node;

  // Allocate an object and a trailing deleter node together.  The node is
  // placed exactly after the object, so the deleter can derive the object
  // address from the node address.  If necessary, padding is placed before
  // the object to align both the object and its trailing node.
  std::pair<void*, arena_detail::ArenaDeleterNode*> CreateRawWithNode(
      std::size_t size, std::size_t align) CBU_MEMORY_NOEXCEPT;
  std::pair<void*, arena_detail::ArenaDeleterArrayNode*> CreateRawWithArrayNode(
      std::size_t size, std::size_t align) CBU_MEMORY_NOEXCEPT;

  static void DestructSingle(Node* first) noexcept;
  static void DestructList(Node* first) noexcept;
  const char* PushStringImpl(const void*, size_t) CBU_MEMORY_NOEXCEPT;
  const char* PushZStringImpl(const void*, size_t) CBU_MEMORY_NOEXCEPT;

  static constexpr std::size_t kInitialAlloc = 256;
  static constexpr std::size_t kMaxAlloc = 16384;

  Node* active_ = nullptr;
  Node* list_ = nullptr;
};

}  // namespace cbu
