/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020-2025, chys <admin@CHYS.INFO>
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

#include <algorithm>
#include <optional>
#include <string_view>
#include <variant>

#include "cbu/common/concepts.h"
#include "cbu/common/stdhack.h"
#include "cbu/common/tags.h"
#include "cbu/strings/faststr.h"

namespace cbu::sb {

template <typename Builder>
concept HasSize = requires(const Builder& bldr) {
  // Add size() only if it's cheap to derive the actual size, and that
  // the actual writing never uses additional space
  { bldr.size() } -> std::convertible_to<std::size_t>;
};

template <typename Builder>
concept HasMaxSize = requires(const Builder& bldr) {
  // Add max_size() _only_ if write() may use additional space.
  { bldr.max_size() } -> std::convertible_to<std::size_t>;
};

template <typename Builder>
concept HasMinSize = requires(const Builder& bldr) {
  { bldr.min_size() } -> std::convertible_to<std::size_t>;
};

template <typename Builder>
concept HasStaticMaxSize = requires() {
  requires(sizeof(char[Builder::static_max_size()]) < 0xffffffff);
};

template <typename Builder>
concept HasStaticMinSize = requires() {
  requires(sizeof(char[Builder::static_min_size()]) < 0xffffffff);
};

template <typename Builder>
concept BaseBuilder = requires(const Builder& bldr) {
  requires (HasMaxSize<Builder> || HasSize<Builder>);
  { bldr.write(std::declval<char*>()) } -> std::convertible_to<char*>;
};

template <typename Builder>
constexpr std::size_t get_static_min_size() noexcept {
  if constexpr (HasStaticMinSize<Builder>)
    return Builder::static_min_size();
  else
    return 0;
}

template <typename Builder>
constexpr std::size_t get_min_size(const Builder& builder) noexcept {
  if constexpr (HasMinSize<Builder>)
    return builder.min_size();
  else if constexpr (HasSize<Builder>)
    return builder.size();
  else if constexpr (HasStaticMinSize<Builder>)
    return Builder::static_min_size();
  else
    return 0;
}

template <typename Builder>
constexpr std::size_t get_max_size(const Builder& builder) noexcept {
  if constexpr (HasMaxSize<Builder>)
    return builder.max_size();
  else if constexpr (HasStaticMaxSize<Builder>)
    return Builder::static_max_size();
  else
    return builder.size();
}

template <typename Builder, std::size_t MaxLen = 1024 * 1024>
concept BuilderForBuffer = requires(const Builder& bldr) {
  requires(sizeof(char[Builder::static_max_size()]) <= MaxLen);
  { bldr.write(std::declval<char*>()) } -> std::same_as<char*>;
};

template <std::size_t MaxLen>
  requires(MaxLen <= 1024 * 1024)  // Avoid stack overflow
class Buffer {
 public:
  explicit Buffer(UninitializedTag) noexcept {}

  template <BuilderForBuffer<MaxLen> Builder, std::integral R>
  explicit Buffer(R& rl, const Builder& builder) noexcept {
    if consteval {
      std::fill_n(s_, sizeof(s_), 0);
    }
    rl = builder.write(s_) - s_;
  }

  constexpr Buffer(const Buffer&) noexcept = default;

  constexpr char* data() noexcept { return s_; }
  constexpr const char* data() const noexcept { return s_; }

  // Don't change the param type to pass by value.
  // Passing by reference so that the following idiom is valid:
  //    size_t l;
  //    cbu::sb::Concat( /* ... */ ).as_buffer(l).view(l)
  constexpr std::string_view view(const std::integral auto& l) const noexcept {
    return std::string_view(s_, l);
  }

 private:
  char s_[MaxLen];
};

template <BuilderForBuffer<> Builder, std::integral R>
Buffer(R&, const Builder&) -> Buffer<Builder::static_max_size()>;

// We are not using "deducing this" because adding a base class means we have to
// manually add many constructors
#define CBU_STR_BUILDER_MIXIN                                     \
  void append_to(std::string* dst) {                              \
    std::size_t M = get_max_size(*this), m = get_min_size(*this); \
    char* w = extend(dst, M);                                     \
    w = write(w);                                                 \
    if (m != M) truncate_unsafe(dst, w - dst->data());            \
  }                                                               \
  void append_to(std::string& dst) { append_to(&dst); }           \
  std::string as_string() {                                       \
    std::string r;                                                \
    append_to(&r);                                                \
    return r;                                                     \
  }                                                               \
  constexpr auto as_buffer(std::integral auto& rl) noexcept {     \
    return Buffer(rl, *this);                                     \
  }

template <std::size_t MaxLen = std::size_t(-1), std::size_t MinLen = 0>
struct View {
  std::string_view sv;
  static constexpr std::size_t static_min_size() noexcept { return MinLen; }
  static constexpr std::size_t static_max_size() noexcept
    requires(MaxLen < 0xffffffff)
  {
    return MaxLen;
  }
  constexpr std::size_t min_size() const noexcept { return sv.size(); }
  constexpr std::size_t size() const noexcept { return sv.size(); }
  constexpr char* write(char* w) const noexcept {
    return cbu::Mempcpy(w, sv.data(), sv.size());
  }
  CBU_STR_BUILDER_MIXIN
};

template <std::size_t L>
  requires(L < 0xffffffff)
struct ConstLengthView {
  const char* s;
  static constexpr std::size_t static_min_size() noexcept { return L; }
  static constexpr std::size_t static_max_size() noexcept { return L; }
  static constexpr std::size_t min_size() noexcept { return L; }
  static constexpr std::size_t size() noexcept { return L; }
  constexpr char* write(char* w) const noexcept {
    return cbu::Mempcpy(w, s, L);
  }
  CBU_STR_BUILDER_MIXIN
};

// Difference with View is that we always copy MaxSize bytes for better code
// generation
template <std::size_t MaxSize, std::size_t MinSize = 0>
  requires(MaxSize >= MinSize && MaxSize < 0xffffffff)
struct FromBuffer {
  const char* s;
  std::size_t l;
  constexpr FromBuffer(const char* s, std::size_t l) noexcept : s(s), l(l) {}
  static constexpr std::size_t static_max_size() noexcept { return MaxSize; }
  static constexpr std::size_t static_min_size() noexcept { return MinSize; }
  constexpr std::size_t max_size() const noexcept { return MaxSize; }
  constexpr std::size_t min_size() const noexcept { return l; }
  constexpr char* write(char* w) const noexcept {
    Memcpy(w, s, MaxSize);
    return w + l;
  }
  CBU_STR_BUILDER_MIXIN
};

template <std::size_t MaxSize>
FromBuffer(const char (&)[MaxSize], std::size_t) -> FromBuffer<MaxSize>;

// Difference with View is that VarLen forcefully generates actual mempcpy call
// unless the compiler knows length is a compile-time constant.
template <std::size_t MaxLen = std::size_t(-1), std::size_t MinLen = 0>
  requires(MaxLen >= MinLen)
struct VarLen {
  const char* s;
  std::size_t l;
  constexpr VarLen(const char* s, std::size_t l) noexcept : s(s), l(l) {}
  constexpr VarLen(std::string_view sv) noexcept : s(sv.data()), l(sv.size()) {}
  static constexpr std::size_t static_min_size() noexcept { return MinLen; }
  static constexpr std::size_t static_max_size() noexcept
    requires(MaxLen < 0xffffffff)
  {
    return MaxLen;
  }
  constexpr std::size_t min_size() const noexcept { return l; }
  constexpr std::size_t size() const noexcept { return l; }
  constexpr char* write(char* w) const noexcept {
    if (__builtin_constant_p(l)) {
      // It may still be constant propagated
      return cbu::Mempcpy(w, s, l);
    } else {
      return cbu::mempcpy_no_builtin(w, s, l);
    }
  }
  CBU_STR_BUILDER_MIXIN
};

struct Char {
  char c;
  static constexpr std::size_t static_min_size() noexcept { return 1; }
  static constexpr std::size_t static_max_size() noexcept { return 1; }
  static constexpr std::size_t min_size() noexcept { return 1; }
  static constexpr std::size_t size() noexcept { return 1; }
  constexpr char* write(char* w) const noexcept {
    *w++ = c;
    return w;
  }
  CBU_STR_BUILDER_MIXIN
};

template <std::size_t MaxLen = std::size_t(-1), std::size_t MinLen = 0>
  requires(MaxLen >= MinLen)
struct Chars {
  std::size_t n;
  char c;
  static constexpr std::size_t static_min_size() noexcept { return MinLen; }
  static constexpr std::size_t static_max_size() noexcept
    requires(MaxLen < 0xffffffff)
  {
    return MaxLen;
  }
  constexpr std::size_t min_size() const noexcept { return n; }
  constexpr std::size_t size() const noexcept { return n; }
  constexpr char* write(char* w) const noexcept {
    for (std::size_t i = 0; i != n; ++i) *w++ = c;
    return w;
  }
  CBU_STR_BUILDER_MIXIN
};

// For types customized str builders
template <typename Type>
concept TypeWithCustomizedStrBuilder = requires(const Type& v) {
  { v.str_builder() } -> BaseBuilder;
};

template <BaseBuilder Builder>
struct OptionalImpl {
  std::optional<Builder> builder;

  static constexpr std::size_t static_min_size() noexcept { return 0; }
  static constexpr std::size_t static_max_size() noexcept
    requires HasStaticMaxSize<Builder>
  {
    return Builder::static_max_size();
  }
  constexpr std::size_t max_size() const noexcept
    requires HasMaxSize<Builder>
  {
    return builder ? builder->max_size() : 0;
  }
  constexpr std::size_t min_size() const noexcept {
    return builder ? get_min_size(*builder) : 0;
  }
  constexpr std::size_t size() const noexcept {
    return builder ? builder->size() : 0;
  }
  constexpr char* write(char* w) const noexcept {
    if (builder) w = builder->write(w);
    return w;
  }
  CBU_STR_BUILDER_MIXIN
};

template <BaseBuilder... Builders>
struct VariantImpl {
  std::variant<Builders...> builders;

  static constexpr std::size_t static_min_size() noexcept {
    return std::min({get_static_min_size<Builders>()...});
  }
  static constexpr std::size_t static_max_size() noexcept
    requires((... && HasStaticMaxSize<Builders>))
  {
    return std::max({Builders::static_max_size()...});
  }
  constexpr std::size_t max_size() const noexcept
    requires((... || HasMaxSize<Builders>))
  {
    return std::visit(
        [](const auto& builder) constexpr noexcept {
          return get_max_size(builder);
        },
        builders);
  }
  constexpr std::size_t min_size() const noexcept {
    return std::visit(
        [](const auto& builder) constexpr noexcept {
          return get_min_size(builder);
        },
        builders);
  }
  constexpr std::size_t size() const noexcept
    requires((... && HasSize<Builders>))
  {
    return std::visit(
        [](const auto& builder) constexpr noexcept { return builder.size(); },
        builders);
  }
  constexpr char* write(char* w) const noexcept {
    return std::visit(
        [w](const auto& builder) constexpr noexcept {
          return builder.write(w);
        },
        builders);
  }
  CBU_STR_BUILDER_MIXIN
};

constexpr auto Adapt(const BaseBuilder auto& bldr) noexcept { return bldr; }
template <std::size_t N>
constexpr auto Adapt(const char (&s)[N]) noexcept {
  return ConstLengthView<N - 1>{s};
}
constexpr auto Adapt(std::string_view sv) noexcept { return View{sv}; }
template <typename T>
  requires(String_view_compat<T> && !TypeWithCustomizedStrBuilder<T>)
constexpr auto Adapt(const T& sv) noexcept {
  return View{std::string_view(sv)};
}
constexpr auto Adapt(char c) noexcept { return Char{c}; }
constexpr auto Adapt(char8_t c) noexcept { return Char{char(c)}; }
constexpr auto Adapt(std::integral auto) noexcept = delete;
constexpr auto Adapt(const TypeWithCustomizedStrBuilder auto& v) noexcept {
  return v.str_builder();
}
template <BaseBuilder Builder>
constexpr auto Adapt(const std::optional<Builder>& bldr) noexcept {
  return OptionalImpl{bldr};
}

template <typename T>
concept ExBuilder = requires(const T& ex) {
  { Adapt(ex) } -> BaseBuilder;
};

template <ExBuilder T>
constexpr auto Optional(std::optional<T> bldr) noexcept {
  if constexpr (BaseBuilder<T>) {
    return OptionalImpl{bldr};
  } else {
    return OptionalImpl{
        bldr.transform([](const T& b) constexpr noexcept { return Adapt(b); })};
  }
}

template <ExBuilder T>
constexpr auto Optional(bool cond, const T& bldr) noexcept {
  return OptionalImpl{cond ? std::optional(Adapt(bldr)) : std::nullopt};
}

template <ExBuilder... Ts>
constexpr auto Variant(const std::variant<Ts...>& bldrs) noexcept {
  using R = std::variant<decltype(Adapt(std::declval<Ts>()))...>;
  return VariantImpl{std::visit(
      [](const auto& b) constexpr noexcept { return R(Adapt(b)); }, bldrs)};
}

template <ExBuilder T, ExBuilder U>
constexpr auto Conditional(bool cond, const T& a, const U& b) noexcept {
  using BT = decltype(Adapt(a));
  using BU = decltype(Adapt(b));
  if constexpr (std::is_same_v<BT, BU>) {
    return cond ? Adapt(a) : Adapt(b);
  } else {
    using V = std::variant<BT, BU>;
    return VariantImpl{cond ? V(Adapt(a)) : V(Adapt(b))};
  }
}

template <BaseBuilder... Builders>
struct ConcatImpl {
  std::tuple<Builders...> builders;

  static constexpr std::size_t static_min_size() noexcept {
    return (0zu + ... + get_static_min_size<Builders>());
  }
  static constexpr std::size_t static_max_size() noexcept
    requires(true && ... && HasStaticMaxSize<Builders>)
  {
    return (0zu + ... + Builders::static_max_size());
  }
  constexpr std::size_t max_size() const noexcept
    requires((... || HasMaxSize<Builders>))
  {
    return std::apply(
        [](const Builders&... bldrs) constexpr noexcept {
          return (0zu + ... + get_max_size(bldrs));
        },
        builders);
  }
  constexpr std::size_t min_size() const noexcept {
    return std::apply(
        [](const Builders&... bldrs) constexpr noexcept {
          return (0zu + ... + get_min_size(bldrs));
        },
        builders);
  }
  constexpr std::size_t size() const noexcept
    requires((true && ... && HasSize<Builders>))
  {
    return std::apply(
        [](const Builders&... bldrs) constexpr noexcept {
          return (0zu + ... + bldrs.size());
        },
        builders);
  }
  constexpr char* write(char* w) const noexcept {
    return std::apply(
        [w](const Builders&... bldrs) constexpr noexcept {
          char* p = w;
          ((p = bldrs.write(p)), ...);
          return p;
        },
        builders);
  }
  CBU_STR_BUILDER_MIXIN
};

template <ExBuilder... Builders>
constexpr auto Concat(const Builders&... builders) noexcept {
  return ConcatImpl{std::tuple{Adapt(builders)...}};
}

#undef CBU_STR_BUILDER_MIXIN

}  // namespace cbu::sb
