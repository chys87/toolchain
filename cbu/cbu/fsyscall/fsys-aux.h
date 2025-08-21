/*
 * cbu - chys's basic utilities
 * Copyright (c) 2013-2025, chys <admin@CHYS.INFO>
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

#include "fsyscall.h"

#if !defined __cplusplus
# error "fsys-aux supports only C++."
#endif
#if __cplusplus < 202002L
# error "C++20 is required."
#endif

#include <cerrno>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace fsys_aux {
inline namespace fsys_aux_readdir {

struct ReadDirOpt {
  bool close = false;
  bool skip_dots = true;     // Skip "." and ".."
  bool skip_hidden = false;  // Skip all files starting with "."
  bool early_use_reclen = false;
  size_t buf_size = 4096;
};

namespace readdir_detail {

using DirEnt = fsys_linux_dirent64;

template <typename Callback>
using RawResultOf = std::decay_t<std::invoke_result_t<Callback, DirEnt *>>;

template <typename Callback>
inline std::enable_if_t<std::is_void_v<RawResultOf<Callback>>, std::nullptr_t>
call(const Callback &cb, DirEnt *ent) {
  cb(ent);
  return nullptr;
}

template <typename Callback>
inline std::enable_if_t<!std::is_void_v<RawResultOf<Callback>>,
                        RawResultOf<Callback>>
call(const Callback &cb, DirEnt *ent) {
  return cb(ent);
}

template <typename Callback>
using ret_t = decltype(call(std::declval<Callback>(),
                            static_cast<DirEnt *>(nullptr)));

template <bool>
struct DeferClose {
  constexpr DeferClose(int) noexcept {}
};
template <> struct DeferClose<true> {
  int fd;
  ~DeferClose() noexcept { fsys_close(fd); }
};

} // namespace readdir_detail


// Inline is important for optimizing the callback, mostly likely a lambda
template <ReadDirOpt Opt = {}, typename Callback>
[[gnu::always_inline]]
inline auto readdir(int fd, const Callback &callback,
                    const readdir_detail::ret_t<Callback> &default_value = {}) {
  using readdir_detail::DirEnt;

  readdir_detail::DeferClose<Opt.close> close_obj {fd};
  alignas(DirEnt) char buf[Opt.buf_size];
  ssize_t l;
  while ((l = fsys_getdents64(fd, buf, sizeof(buf))) > 0) {
    char *ptr = buf;
    char *ptrend = buf + l;
    do {
      DirEnt *ent = reinterpret_cast<DirEnt *>(ptr);
      if constexpr (Opt.early_use_reclen) {
        ptr += ent->d_reclen;
      }
      bool skipped = false;
      if constexpr (Opt.skip_hidden) {
        skipped = (ent->d_name[0] == '.');
      } else if constexpr (Opt.skip_dots) {
        skipped = ent->skip_dot();
      }
      if (!skipped) {
        auto res = readdir_detail::call(callback, ent);
        if (res != default_value)
          return res;
      }
      if constexpr (!Opt.early_use_reclen) {
        ptr += ent->d_reclen;
      }
    } while (ptr < ptrend);
  }
  return default_value;
}

} // namespace fsys_aux_readdir

template <typename Callback>
  requires requires(const Callback& cb) {
    { cb() } -> std::signed_integral;
  }
inline auto eintr(const Callback& cb) {
  decltype(cb()) r;
  do {
    r = cb();
  } while (r < 0 && fsys_errno(r, EINTR));
  return r;
}

} // namespace fsys_aux

#define fsys_eintr(...) ::fsys_aux::eintr([&]() { return __VA_ARGS__; })

#if defined __cplusplus && __cplusplus >= 201703L
#include <tuple>
#include <type_traits>

template <unsigned n, typename... Args>
fsys_inline const auto& fsys_nth(const Args&... args) {
#if defined __cpp_pack_indexing && __cpp_pack_indexing >= 202311L
  return args...[n];
#else
  return std::get<n>(std::tie(args...));
#endif
}

template <typename RetType, typename... Args>
  requires(sizeof...(Args) <= 6)
fsys_inline RetType fsyscall(unsigned sysno, const Args&... args) {
  constexpr int n = sizeof...(Args);
  if constexpr (n == 0) {
    return fsys_generic(sysno, RetType, 0);
  } else if constexpr (n == 1) {
    return fsys_generic(sysno, RetType, 1, fsys_nth<0>(args...));
  } else if constexpr (n == 2) {
    return fsys_generic(sysno, RetType, 2, fsys_nth<0>(args...), fsys_nth<1>(args...));
  } else if constexpr (n == 3) {
    return fsys_generic(sysno, RetType, 3, fsys_nth<0>(args...), fsys_nth<1>(args...),
                        fsys_nth<2>(args...));
  } else if constexpr (n == 4) {
    return fsys_generic(sysno, RetType, 4, fsys_nth<0>(args...), fsys_nth<1>(args...),
                        fsys_nth<2>(args...), fsys_nth<3>(args...));
  } else if constexpr (n == 5) {
    return fsys_generic(sysno, RetType, 5, fsys_nth<0>(args...), fsys_nth<1>(args...),
                        fsys_nth<2>(args...), fsys_nth<3>(args...), fsys_nth<4>(args...));
  } else if constexpr (n == 6) {
    return fsys_generic(sysno, RetType, 6, fsys_nth<0>(args...), fsys_nth<1>(args...),
                        fsys_nth<2>(args...), fsys_nth<3>(args...), fsys_nth<4>(args...),
                        fsys_nth<5>(args...));
  } else {
    static_assert(false);
  }
}

template <typename RetType, typename... Args>
  requires(sizeof...(Args) <= 6)
fsys_inline RetType fsyscall_nomem(unsigned sysno, const Args&... args) {
  constexpr int n = sizeof...(Args);
  if constexpr (n == 0) {
    return fsys_generic_nomem(sysno, RetType, 0);
  } else if constexpr (n == 1) {
    return fsys_generic_nomem(sysno, RetType, 1, fsys_nth<0>(args...));
  } else if constexpr (n == 2) {
    return fsys_generic_nomem(sysno, RetType, 2, fsys_nth<0>(args...), fsys_nth<1>(args...));
  } else if constexpr (n == 3) {
    return fsys_generic_nomem(sysno, RetType, 3, fsys_nth<0>(args...), fsys_nth<1>(args...),
                        fsys_nth<2>(args...));
  } else if constexpr (n == 4) {
    return fsys_generic_nomem(sysno, RetType, 4, fsys_nth<0>(args...), fsys_nth<1>(args...),
                        fsys_nth<2>(args...), fsys_nth<3>(args...));
  } else if constexpr (n == 5) {
    return fsys_generic_nomem(sysno, RetType, 5, fsys_nth<0>(args...), fsys_nth<1>(args...),
                        fsys_nth<2>(args...), fsys_nth<3>(args...), fsys_nth<4>(args...));
  } else if constexpr (n == 6) {
    return fsys_generic_nomem(sysno, RetType, 6, fsys_nth<0>(args...), fsys_nth<1>(args...),
                        fsys_nth<2>(args...), fsys_nth<3>(args...), fsys_nth<4>(args...),
                        fsys_nth<5>(args...));
  } else {
    static_assert(false);
  }
}
#endif
