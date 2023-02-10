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

#if defined __x86_64__ && !defined __LP64__

#include <dlfcn.h>
#include <poll.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "cbu/common/fixed_string.h"

namespace cbu {
namespace coroutine {

// Accessors to non-prempted functions in libc
template <fixed_string NAME, typename Prototype>
class RawFuncAccessor;

template <fixed_string NAME, typename R, typename... Params>
class RawFuncAccessor<NAME, R(Params...)> {
 private:
  constexpr RawFuncAccessor() = default;
  RawFuncAccessor(const RawFuncAccessor&) = delete;
  RawFuncAccessor& operator = (const RawFuncAccessor&) = delete;

 public:
  using Prototype = R(Params...);
  static inline RawFuncAccessor instance;

  R operator()(Params... params) { return f_(params...); }

  static R ifunc_loader(Params... params) {
    Prototype* func =
        reinterpret_cast<Prototype*>(dlsym(RTLD_NEXT, NAME.c_str()));
    instance.f_ = func;
    return func(params...);
  }

 private:
  Prototype* f_ = &ifunc_loader;
};

// Access to non-prempted functions from libc
inline auto& sys_poll =
    RawFuncAccessor<"poll", int(pollfd*, nfds_t, int)>::instance;
inline auto& sys_read =
    RawFuncAccessor<"read", ssize_t(int, void*, size_t)>::instance;
inline auto& sys_write =
    RawFuncAccessor<"write", ssize_t(int, const void*, size_t)>::instance;
inline auto& sys_send =
    RawFuncAccessor<"send", ssize_t(int, const void*, size_t, int)>::instance;
inline auto& sys_sendto =
    RawFuncAccessor<"sendto", ssize_t(int, const void*, size_t, int,
                                      const sockaddr*, socklen_t)>::instance;
inline auto& sys_recv =
    RawFuncAccessor<"recv", ssize_t(int, void*, size_t, int)>::instance;
inline auto& sys_recvfrom =
    RawFuncAccessor<"recvfrom", ssize_t(int, void*, size_t, int, sockaddr*,
                                        socklen_t)>::instance;
inline auto& sys_usleep = RawFuncAccessor<"usleep", int(useconds_t)>::instance;
inline auto& sys_epoll_wait =
    RawFuncAccessor<"epoll_wait", int(int, epoll_event*, int, int)>::instance;

} // namespace coroutine
} // namespace cbu

#endif
