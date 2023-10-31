/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2023, chys <admin@CHYS.INFO>
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

#ifdef __linux__

#include <fcntl.h>
#include <stdint.h>
#include <sys/stat.h>

#include <tuple>
#include <type_traits>

#include "cbu/fsyscall/fsyscall.h"

namespace cbu {

using struct_statx = struct statx;

struct StatxBaseSpec {
  static constexpr int kFlags = 0;
  static constexpr uint32_t kMask = 0;
  static constexpr void operator()(int, const struct_statx&) noexcept {}
};

namespace detail {

template <typename T>
concept Statx_spec = std::is_base_of_v<StatxBaseSpec, T>;

template <Statx_spec Spec>
constexpr auto StatxResultAsTuple(int ret, const struct_statx& stx,
                                  Spec spec) noexcept {
  using RT = decltype(spec(ret, stx));
  if constexpr (std::is_void_v<RT>)
    return std::tuple();
  else
    return std::tuple<RT>(spec(ret, stx));
}

}  // namespace detail

template <int flags>
struct StatxFlags : StatxBaseSpec {
  static constexpr int kFlags = flags;
};

using StatxNoFollow = StatxFlags<AT_SYMLINK_NOFOLLOW>;
using StatxNoAutoMount = StatxFlags<AT_NO_AUTOMOUNT>;
using StatxEmptyPath = StatxFlags<AT_EMPTY_PATH>;

template <uint32_t mask, auto field, typename RT = void>
struct StatxField;

template <uint32_t mask, typename T, T struct_statx::*field, typename RT>
struct StatxField<mask, field, RT> : StatxBaseSpec {
  static constexpr uint32_t kMask = mask;
  static constexpr std::conditional_t<std::is_void_v<RT>, T, RT> operator()(
      int, const struct_statx& stx) noexcept {
    return stx.*field;
  }
};

using StatxSize = StatxField<STATX_SIZE, &struct_statx::stx_size, uint64_t>;
using StatxMTime = StatxField<STATX_MTIME, &struct_statx::stx_mtime>;
using StatxCTime = StatxField<STATX_CTIME, &struct_statx::stx_ctime>;
using StatxBTime = StatxField<STATX_BTIME, &struct_statx::stx_btime>;
using StatxATime = StatxField<STATX_ATIME, &struct_statx::stx_atime>;
using StatxMode = StatxField<STATX_MODE | STATX_TYPE, &struct_statx::stx_mode>;
using StatxModeOnly = StatxField<STATX_MODE, &struct_statx::stx_mode>;
using StatxType = StatxField<STATX_TYPE, &struct_statx::stx_mode>;
using StatxUid = StatxField<STATX_UID, &struct_statx::stx_uid>;
using StatxGid = StatxField<STATX_GID, &struct_statx::stx_gid>;

template <detail::Statx_spec RealTimeSpec>
struct StatxCombineTime : RealTimeSpec {
  constexpr StatxCombineTime(RealTimeSpec rts) noexcept : RealTimeSpec(rts) {}
  static constexpr uint64_t operator()(int ret,
                                       const struct_statx& stx) noexcept {
    auto ts = RealTimeSpec::operator()(ret, stx);
    return (uint64_t(uint32_t(ts.tv_sec)) << 32) | uint32_t(ts.tv_nsec);
  }
};

template <detail::Statx_spec RealTimeSpec>
struct StatxSecond : RealTimeSpec {
  constexpr StatxSecond(RealTimeSpec rts) noexcept : RealTimeSpec(rts) {}
  static constexpr time_t operator()(int ret,
                                     const struct_statx& stx) noexcept {
    return RealTimeSpec::operator()(ret, stx).tv_sec;
  }
};

template <detail::Statx_spec... Specs>
  requires(sizeof...(Specs) > 0)
inline auto Statx(int fd, const char* path, Specs... specs) {
  struct_statx stx;
  int ret =
      fsys_statx(fd, path, (... | Specs::kFlags), (... | Specs::kMask), &stx);
  return std::tuple_cat(std::tuple(ret), detail::StatxResultAsTuple(ret, stx, specs)...);
}

template <detail::Statx_spec... Specs>
inline auto Statx(int fd, Specs... specs) {
  return Statx(fd, "", StatxEmptyPath(), specs...);
}

template <detail::Statx_spec... Specs>
inline auto Statx(const char* path, Specs... specs) {
  return Statx(AT_FDCWD, path, StatxEmptyPath(), specs...);
}

}  // namespace cbu

#endif  // __linux__
