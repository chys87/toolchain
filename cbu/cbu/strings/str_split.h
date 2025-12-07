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

// This file implements StrSplit with different API from absl::StrSplit
//
// It provides no additional feature to absl::StrSplit, but the generated code
// can be much smaller in size and is equally or more efficient.
//
// (1) Callback API
//
//     cbu::StrSplit(text, delimiter, [predicate,] callback);
//
//   In general, it's equivalent to
//
//     for (auto part : absl::StrSplit(text, delimiter, predicate)) {
//       if constexpr (std::is_same_v<decltype(callback(part)), void>) {
//         callback(part);
//       } else {
//         if (!callback(part)) break;
//       }
//     }
//
//   Like absl::StrSplit, delimiter can be:
//
//      - cbu::ByChar(c)  (default for char arguments)
//      - cbu::ByStringView(s)  (default for string arguments)
//      - A delimiter provided or accpeted by absl::StrSplit
//
//   Like absl::StrSplit, predicate can be:
//
//     - cbu::AllowEmpty (the default)
//     - cbu::SkipEmpty (same as absl::SkipEmpty)
//     - A delimiter provided or accepted by absl::StrSplit
//
//   callback should take a std::string_view argument, and return either void
//   or a type convertible to bool.  A falsy return value breaks the loop.
//
//   cbu::Split returns true by default, and returns false if the callback
//   returns falsy value.
//
// (2) Split and append/insert to an existing container
//
//     cbu::StrSplitTo(text, delim, [predicate,] &container);
//
//   container should support at least one of the following operations
//   (in order of preference):
//
//     container.push_back(string_view);
//     container.push(string_view);
//     container.insert(string_view);
//     container.insert(container.end(), string_view);
//
// (3) Split and construct a new container
//
//     cbu::StrSplitTo<ContainerType>(text, delim [, predicate]);

#pragma once

#include <concepts>
#include <string_view>
#include <utility>

namespace cbu {

class ByChar {
 public:
  explicit constexpr ByChar(char c) noexcept : c_(c) {}

  constexpr std::string_view Find(std::string_view text,
                                  std::size_t pos) const noexcept {
    text = std::string_view(text.data() + pos, text.size() - pos);
    const char* p = static_cast<const char*>(
        __builtin_memchr(text.data(), c_, text.size()));
    if (p == nullptr)
      return {text.data() + text.size(), 0};
    else
      return {p, 1};
  }

 private:
  char c_;
};

class ByStringView {
 public:
  explicit constexpr ByStringView(std::string_view s) noexcept : s_(s) {}

  constexpr std::string_view Find(std::string_view text,
                                  std::size_t pos) const noexcept {
    if (s_.empty()) [[unlikely]]
      if (!text.empty()) return {text.data() + 1, 0};
    auto p = text.find(s_, pos);
    if (p == text.npos)
      return {text.data() + text.size(), 0};
    else
      return {text.data() + p, s_.size()};
  }

 private:
  std::string_view s_;
};

struct AllowEmpty {
  constexpr bool operator()(std::string_view) const noexcept { return true; }
};

struct SkipEmpty {
  constexpr bool operator()(std::string_view s) const noexcept {
    return !s.empty();
  }
};

namespace str_split_detail {

template <typename T>
concept PredicateConcept = std::is_invocable_r_v<bool, T, std::string_view>;

template <typename T>
concept CallbackConcept = std::is_invocable_r_v<void, T, std::string_view> ||
    std::is_invocable_r_v<bool, T, std::string_view>;

template <typename T>
using WrapDelimiter = std::conditional_t<
    std::is_convertible_v<T, char>, ByChar,
    std::conditional_t<std::is_convertible_v<T, std::string_view>, ByStringView,
                       const T&>>;

template <typename Delimiter, typename Predicate, typename Callback>
inline constexpr bool Nothrow =
    noexcept(std::declval<WrapDelimiter<Delimiter>>().Find(std::string_view(),
                                                           std::size_t())) &&
    std::is_nothrow_invocable_v<Predicate, std::string_view>&& std::
        is_nothrow_invocable_v<Callback, std::string_view>;

template <typename Container>
concept PushBackable = requires(Container& container) {
  {container.push_back(std::string_view())};
};

template <typename Container>
concept Pushable = requires(Container& container) {
  {container.push(std::string_view())};
};

template <typename Container>
concept Insertable = requires(Container& container) {
  {container.insert(std::string_view())};
};

template <typename Container>
concept InsertEndable = requires(Container& container) {
  {container.insert(container.end(), std::string_view())};
};

template <typename Container>
concept ContainerConcept = PushBackable<Container> || Pushable<Container> ||
    Insertable<Container> || InsertEndable<Container>;

template <ContainerConcept Container>
struct AppendToContainer {
  Container* container;

  constexpr void operator()(std::string_view part) const {
    if constexpr (PushBackable<Container>)
      container->push_back(part);
    else if constexpr (Pushable<Container>)
      container->push(part);
    else if constexpr (Insertable<Container>)
      container->insert(part);
    else
      container->insert(container->end(), part);
  }
};

template <typename Callback>
constexpr bool
wrap_call(const Callback& callback, std::string_view param) noexcept(
    std::is_nothrow_invocable_v<Callback, std::string_view>) {
  using RT = decltype(callback(param));
  if constexpr (std::is_same_v<RT, void>) {
    callback(param);
    return true;
  } else {
    return static_cast<bool>(callback(param));
  }
}

template <typename Delimiter, PredicateConcept Predicate,
          CallbackConcept Callback>
constexpr bool StrSplit(
    std::string_view text, Delimiter delim, Predicate predicate,
    Callback callback) noexcept(Nothrow<Delimiter, Predicate, Callback>) {
  if (text.empty()) return true;
  while (true) {
    auto sep = delim.Find(text, 0);
    std::string_view part(text.data(), sep.data() - text.data());

    text =
        std::string_view(sep.data() + sep.size(),
                         text.data() + text.size() - (sep.data() + sep.size()));

    if (predicate(part)) {
      if constexpr (std::is_same_v<decltype(callback(part)), void>) {
        callback(part);
      } else {
        if (!callback(part)) return false;
      }
    }
    if constexpr (std::is_same_v<Delimiter, ByChar>) {
      // Any delimiter which guarantees a normal delimiter is non-empty
      // can use this branch.
      if (sep.empty()) break;
    } else {
      // This is generic implementation
      if (sep.data() == text.data() + text.size()) break;
    }
  }
  return true;
}

}  // namespace str_split_detail

template <typename Delimiter, typename Callback>
  requires std::invocable<Callback, std::string_view>
constexpr bool
StrSplit(std::string_view text, Delimiter delim, Callback callback) noexcept(
    str_split_detail::Nothrow<Delimiter, AllowEmpty, Callback>) {
  return str_split_detail::StrSplit(
      text, str_split_detail::WrapDelimiter<Delimiter>(delim), AllowEmpty(),
      std::move(callback));
}

template <typename Delimiter, str_split_detail::PredicateConcept Predicate,
          str_split_detail::CallbackConcept Callback>
constexpr bool StrSplit(
    std::string_view text, Delimiter delim, Predicate predicate,
    Callback callback) noexcept(str_split_detail::Nothrow<Delimiter, Predicate,
                                                          Callback>) {
  return str_split_detail::StrSplit(
      text, str_split_detail::WrapDelimiter<Delimiter>(delim),
      std::move(predicate), std::move(callback));
}

template <typename Delimiter, str_split_detail::ContainerConcept Container>
constexpr void StrSplitTo(std::string_view text, Delimiter delim,
                          Container* container) {
  str_split_detail::StrSplit(
      text, str_split_detail::WrapDelimiter<Delimiter>(delim), AllowEmpty(),
      str_split_detail::AppendToContainer<Container>{container});
}

template <typename Delimiter, str_split_detail::PredicateConcept Predicate,
          str_split_detail::ContainerConcept Container>
constexpr void StrSplitTo(std::string_view text, Delimiter delim,
                          Predicate predicate, Container* container) {
  str_split_detail::StrSplit(
      text, str_split_detail::WrapDelimiter<Delimiter>(delim),
      std::move(predicate),
      str_split_detail::AppendToContainer<Container>{container});
}

template <typename Container, typename Delimiter,
          str_split_detail::PredicateConcept Predicate = AllowEmpty>
constexpr Container StrSplitTo(std::string_view text, Delimiter delim,
                               Predicate predicate = Predicate()) {
  Container container;
  str_split_detail::StrSplit(
      text, str_split_detail::WrapDelimiter<Delimiter>(delim),
      std::move(predicate),
      str_split_detail::AppendToContainer<Container>{&container});
  return container;
}

}  // namespace cbu
