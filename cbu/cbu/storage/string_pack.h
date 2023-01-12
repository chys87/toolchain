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

#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include "cbu/common/zstring_view.h"

// StringPack is a simple zero-copy-serializable storage of a list of strings.
// It aims to be fast and small, and has optional support for compaction.
//
// StringPack sp;
// sp.add("Hello");
// sp << "world";  // operator<< and .add are equivalent.
//
// sp.for_each([](std::string_view s) {
//   // Iterates "Hello" and "world"
// });
//
// std::string serialized = sp.serialize();
//
// sp.deserialize(std::move(serialized));
//
// StringPackZ: Similar with StringPack, but all strings are null-terminated,
// and can be used as C strings without copy.
//
// StringPackCompactEncoder: Encode strings in a compact manner.
// string_pack_compact_decode: Decode compacted string pack
//
// Supported compaction codecs:
//   CommonPrefixSuffixCodec works best if adjacent strings likely have short
//   common prefixes and/or suffixes.  Intended use case is sorted filenames.

namespace cbu {
namespace string_pack_detail {

template<typename StringType>
struct ConsumeOneString {
  StringType sv;
  const char* p;
};

std::optional<ConsumeOneString<std::string_view>> consume_one_string(
    const char* p, const char* e) noexcept;
std::optional<ConsumeOneString<zstring_view>> consume_one_string_z(
    const char* p, const char* e) noexcept;

ConsumeOneString<std::string_view> consume_one_string_unsafe(
    const char* p) noexcept;
ConsumeOneString<zstring_view> consume_one_string_z_unsafe(
    const char* p) noexcept;

}  // namespace string_pack_detail

class StringPackBase {
 public:
  const std::string& serialize() const { return s_; }

  // Unsafe deserialization, meaning there's no protection against malicious
  // input.
  void deserialize_unsafe(std::string&& s) { s_ = std::move(s); }

  [[nodiscard]] bool empty() const noexcept { return s_.empty(); }

 protected:
  // Actual storage.
  std::string s_;
};

class StringPack : public StringPackBase {
 public:
  void add(std::string_view s);

  StringPack& operator<<(std::string_view s) {
    add(s);
    return *this;
  }

  bool deserialize(std::string&& s) {
    if (!verify(s)) return false;
    deserialize_unsafe(std::move(s));
    return true;
  }

  static bool verify(std::string_view s) noexcept;

  template <typename Callback>
  void for_each(Callback callback) const
      noexcept(noexcept(callback(std::string_view()))) {
    const char* p = s_.data();
    const char* e = p + s_.size();
    while (p < e) {
      auto [sv, new_p] = string_pack_detail::consume_one_string_unsafe(p);
      callback(sv);
      p = new_p;
    }
  }
};

class StringPackZ : public StringPackBase {
 public:
  void add(std::string_view s);

  StringPackZ& operator<<(std::string_view s) {
    add(s);
    return *this;
  }

  bool deserialize(std::string&& s) {
    if (!verify(s)) return false;
    deserialize_unsafe(std::move(s));
    return true;
  }

  static bool verify(std::string_view s) noexcept;

  template <typename Callback>
  void for_each(Callback callback) const
      noexcept(noexcept(callback(zstring_view()))) {
    const char* p = s_.data();
    const char* e = p + s_.size();
    while (p < e) {
      auto [sv, new_p] = string_pack_detail::consume_one_string_z_unsafe(p);
      callback(sv);
      p = new_p;
    }
  }
};

class StringPackCompactEncoderBase {
 public:
  const std::string& serialize() const { return s_; }
  [[nodiscard]] bool empty() const noexcept { return s_.empty(); }

 protected:
  void add_compacted(std::string_view compact_info,
                     std::string_view compacted_bytes);

 protected:
  std::string s_;
  std::string prev_item_;
  std::string scratch_;
};

struct CommonPrefixSuffixCodec {
  struct CompactInfo {
    std::uint8_t prefix_suffix;
  };

  static void compact(std::string_view prev, std::string_view cur,
                      CompactInfo* compact_info,
                      std::string_view* compacted_bytes, std::string* scratch);
  static void restore(CompactInfo compact_info, std::string_view prev,
                      std::string_view compacted_bytes, std::string* out);

  static std::size_t common_prefix_max_31(std::string_view a, std::string_view b);
  static std::size_t common_suffix_max_7(std::string_view a, std::string_view b);
};

template <typename Codec = CommonPrefixSuffixCodec>
class StringPackCompactEncoder : public StringPackCompactEncoderBase {
 public:
  constexpr StringPackCompactEncoder() noexcept(noexcept(Codec())) : codec_() {}
  constexpr explicit StringPackCompactEncoder(const Codec& codec) noexcept(
      std::is_nothrow_copy_constructible_v<Codec>)
      : codec_(codec) {}
  constexpr explicit StringPackCompactEncoder(Codec& codec) noexcept(
      std::is_nothrow_move_constructible_v<Codec>)
      : codec_(std::move(codec)) {}

  const Codec& codec() const noexcept { return codec_; }

  void add(std::string&& s);
  StringPackCompactEncoder& operator<<(std::string&& s) {
    add(std::move(s));
    return *this;
  }

 private:
  Codec codec_;
};

template <typename Codec>
void StringPackCompactEncoder<Codec>::add(std::string&& s) {
  typename Codec::CompactInfo compact_info;
  std::string_view compacted_bytes;
  codec_.compact(prev_item_, s, &compact_info, &compacted_bytes, &scratch_);
  add_compacted(
      {reinterpret_cast<const char*>(&compact_info), sizeof(compact_info)},
      compacted_bytes);
  prev_item_ = std::move(s);
}

template <typename Codec = CommonPrefixSuffixCodec, typename Callback>
bool string_pack_compact_decode(std::string_view s, Callback callback,
                                const Codec& codec = Codec()) {
  using CompactInfo = typename Codec::CompactInfo;
  static_assert(alignof(CompactInfo) == 1);

  std::string prev;
  std::string cur;

  const char* p = s.data();
  const char* e = p + s.size();
  while (p < e) {
    if (std::size_t(e - p) < sizeof(CompactInfo)) return false;
    const CompactInfo& compact_info = *reinterpret_cast<const CompactInfo*>(p);
    auto cos =
        string_pack_detail::consume_one_string(p + sizeof(CompactInfo), e);
    if (!cos) return false;
    auto [sv, new_p] = *cos;
    p = new_p;

    codec.restore(compact_info, prev, sv, &cur);
    callback(static_cast<const std::string&>(cur));
    prev = std::move(cur);
  }
  return true;
}

}  // namespace cbu
