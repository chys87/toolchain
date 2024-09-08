/*
 * cbu - chys's basic utilities
 * Copyright (c) 2020-2024, chys <admin@CHYS.INFO>
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

#include <optional>
#include <string_view>

#include "cbu/common/concepts.h"
#include "cbu/common/stdhack.h"
#include "cbu/strings/faststr.h"

namespace cbu::sb {

template <typename Builder>
concept BaseBuilder = requires(const Builder& bldr) {
  { bldr.size() } -> std::convertible_to<std::size_t>;
  { bldr.write(std::declval<char*>()) } -> std::convertible_to<char*>;
};

// We are not using "deducing this" because adding a base class means we have to
// manually add many constructors
#define CBU_STR_BUILDER_MIXIN                                      \
  void append_to(std::string* dst) { write(extend(dst, size())); } \
  void append_to(std::string& dst) { append_to(&dst); }            \
  std::string as_string() {                                        \
    std::string r;                                                 \
    append_to(&r);                                                 \
    return r;                                                      \
  }

struct View {
  std::string_view sv;
  constexpr std::size_t size() const noexcept { return sv.size(); }
  constexpr char* write(char* w) const noexcept {
    return cbu::Mempcpy(w, sv.data(), sv.size());
  }
  CBU_STR_BUILDER_MIXIN
};

// Difference with View is that VarLen forcefully generates actual mempcpy call
// unless the compiler knows length is a compile-time constant.
struct VarLen {
  const char* s;
  std::size_t l;
  constexpr VarLen(const char* s, std::size_t l) noexcept : s(s), l(l) {}
  constexpr VarLen(std::string_view sv) noexcept : s(sv.data()), l(sv.size()) {}
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
  static constexpr std::size_t size() noexcept { return 1; }
  constexpr char* write(char* w) const noexcept {
    *w++ = c;
    return w;
  }
  CBU_STR_BUILDER_MIXIN
};

// For fillers (low_level_buffer_filler.h) with size()
template <typename Builder>
concept CompatibleLowLevelBufferFiller = requires(const Builder& filler) {
  { filler(std::declval<char*>()) } -> std::convertible_to<char*>;
  { filler.size() } -> std::convertible_to<std::size_t>;
};

template <CompatibleLowLevelBufferFiller Filler>
struct LowLevelBufferFillerAdapter {
  Filler filler;

  constexpr std::size_t size() const noexcept { return filler.size(); }
  constexpr char* write(char* w) const noexcept { return filler(w); }
  CBU_STR_BUILDER_MIXIN
};

template <BaseBuilder Builder>
struct OptionalImpl {
  std::optional<Builder> builder;

  constexpr std::size_t size() const noexcept {
    return builder ? builder->size() : 0;
  }
  constexpr char* write(char* w) const noexcept {
    if (builder) w = builder->write(w);
    return w;
  }
  CBU_STR_BUILDER_MIXIN
};

constexpr auto Adapt(const BaseBuilder auto& bldr) noexcept { return bldr; }
constexpr auto Adapt(std::string_view sv) noexcept { return View{sv}; }
constexpr auto Adapt(char c) noexcept { return Char{c}; }
constexpr auto Adapt(
    const CompatibleLowLevelBufferFiller auto& filler) noexcept {
  return LowLevelBufferFillerAdapter{filler};
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
auto Optional(std::optional<T> bldr) noexcept {
  if constexpr (BaseBuilder<T>) {
    return OptionalImpl{bldr};
  } else {
    return OptionalImpl{
        bldr.transform([](const T& b) constexpr noexcept { return Adapt(b); })};
  }
}

template <ExBuilder T>
auto Optional(bool cond, const T& bldr) noexcept {
  return OptionalImpl{cond ? std::optional(Adapt(bldr)) : std::nullopt};
}

template <BaseBuilder... Builders>
struct ConcatImpl {
  std::tuple<Builders...> builders;

  constexpr std::size_t size() const noexcept {
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
