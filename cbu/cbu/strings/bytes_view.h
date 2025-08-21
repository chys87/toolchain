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

#include <concepts>  // std::convertible_to
#include <cstddef>  // std::byte
#include <iterator>  // std::data, std::size
#include <string_view>
#include <type_traits>

#include "cbu/common/concepts.h"

// This header provides a "bytes view" class, which behaves somewhat like
// std::string_view, but provides additional interfaces for convenient
// pointer cast to and from other character types
// (signed char, unsigned char, char8_t, std::byte).
// It also accepts explicit const void*

namespace cbu {

namespace cbu_bytes_view_types {

template <typename T> struct is_implicit_type : std::false_type {};
template <> struct is_implicit_type<void> : std::true_type {};
template <> struct is_implicit_type<char> : std::true_type {};
template <> struct is_implicit_type<signed char> : std::true_type {};
template <> struct is_implicit_type<unsigned char> :
    std::true_type {};
template <> struct is_implicit_type<char8_t> : std::true_type {};
template <> struct is_implicit_type<std::byte> : std::true_type {};
template <typename T>
inline constexpr bool is_implicit_type_v = is_implicit_type<T>::value;

template <typename T>
inline constexpr bool is_valid_type_v =
    (std::is_same_v<std::remove_cvref_t<T>, T> && !std::is_array_v<T> &&
     (std::is_void_v<T> ||
      (std::is_trivially_default_constructible_v<T> && std::is_trivially_copyable_v<T>)));

template <typename T>
inline constexpr bool is_explicit_type_v = (
    is_valid_type_v<T> && !is_implicit_type_v<T>);

template <typename T> concept Valid_type = is_valid_type_v<T>;
template <typename T> concept Implicit_type = is_implicit_type_v<T>;
template <typename T> concept Explicit_type = is_explicit_type_v<T>;

template <typename T>
concept Valid_obj = requires (T&& obj) {
  { std::data(std::forward<T>(obj)) } -> std::convertible_to<const void*>;
  { std::size(std::forward<T>(obj)) } -> std::convertible_to<std::size_t>;
  requires Valid_type<std::remove_const_t<std::remove_reference_t<
      decltype(*std::data(std::forward<T>(obj)))>>>;
};

template <typename T> struct is_valid_obj : std::false_type {};
template <Valid_obj T>
struct is_valid_obj<T> : std::true_type {};

template <typename T> struct is_implicit_obj : std::false_type {};
template <typename T>
  requires (
      is_valid_obj<T>::value &&
      is_implicit_type_v<std::remove_const_t<std::remove_pointer_t<
          decltype(std::data(std::declval<T&&>()))>>>)
struct is_implicit_obj<T> : std::true_type {};

template <typename T>
inline constexpr bool is_valid_obj_v = is_valid_obj<T>::value;

template <typename T>
inline constexpr bool is_implicit_obj_v = is_implicit_obj<T>::value;

template <typename T>
inline constexpr bool is_explicit_obj_v = (is_valid_obj_v<T> &&
                                           !is_implicit_obj_v<T>);

template <typename T> concept Explicit_obj = is_explicit_obj_v<T>;
template <typename T> concept Implicit_obj = is_implicit_obj_v<T>;

class pointer {
 public:
  constexpr pointer() noexcept : p_(nullptr) {}
  constexpr pointer(decltype(nullptr)) noexcept : pointer() {}
  template <Valid_type T>
  explicit(Explicit_type<T>) constexpr pointer(const T* p) noexcept : p_(p) {}
  constexpr pointer(const pointer&) noexcept = default;
  constexpr pointer& operator=(const pointer&) noexcept = default;

  template <Valid_type T = void>
  constexpr const T* get() const noexcept {
    return static_cast<const T*>(p_);
  }

  constexpr operator const void* () const noexcept { return get<void>(); }
  constexpr operator const char* () const noexcept { return get<char>(); }
  constexpr operator const unsigned char* () const noexcept {
    return get<unsigned char>();
  }
  constexpr operator const signed char* () const noexcept {
    return get<signed char>();
  }
  constexpr operator const char8_t* () const noexcept {
    return get<char8_t>();
  }
  constexpr operator const std::byte* () const noexcept {
    return get<std::byte>();
  }

 private:
  const void* p_;
};

class BytesViewImpl {
 public:
  using value_type = const std::byte;

 public:
  constexpr BytesViewImpl() noexcept : p_(nullptr), l_(0) {}
  constexpr BytesViewImpl(decltype(nullptr)) noexcept : BytesViewImpl() {}

  template <Valid_type T, Raw_integral L>
    requires (sizeof(L) > sizeof(char))
  explicit(Explicit_type<T>) constexpr BytesViewImpl(const T* p, L l) noexcept :
      p_(p), l_(l) {}

  template <Valid_obj T>
  explicit(Explicit_obj<T>) constexpr BytesViewImpl(T&& obj) noexcept :
      p_(std::data(std::forward<T>(obj))),
      l_(sizeof(*std::data(std::forward<T>(obj))) *
         std::size(std::forward<T>(obj))) {}

  constexpr BytesViewImpl(const BytesViewImpl&) noexcept = default;

  constexpr BytesViewImpl& operator=(const BytesViewImpl&) noexcept = default;

  constexpr pointer data() const noexcept { return pointer(p_); }
  constexpr std::size_t size() const noexcept { return l_; }

  // Don't explicitly cast to other string view types
  constexpr operator std::string_view() const noexcept {
    return {data(), size()};
  }

 private:
  const void* p_;
  std::size_t l_;
};

}  // namespace cbu_bytes_view_types

class BytesView : public cbu_bytes_view_types::BytesViewImpl {
 public:
  using cbu_bytes_view_types::BytesViewImpl::BytesViewImpl;
  constexpr BytesView(const BytesView&) noexcept = default;
};

}  // namespace cbu
