/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019, 2020, chys <admin@CHYS.INFO>
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

#include "syscall_hook.h"
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "coroutine.h"

#define VISIBLE __attribute__((externally_visible, visibility("default")))

namespace cbu {
namespace coroutine {
namespace {

inline bool is_non_blocking(int fd) {
  int flags = fcntl(fd, F_GETFL);
  return (flags >= 0 && (flags & O_NONBLOCK));
}

void single_poll(int fd, short events, int timeout = -1) {
  pollfd fds[] = {{fd, events, 0}};
  active_container->Poll(fds, 1, timeout);
}

} // namespace

extern "C" {

VISIBLE int poll(pollfd* fds, nfds_t nfds, int timeout) {
  if (active_container == nullptr)
    return sys_poll(fds, nfds, timeout);
  return active_container->Poll(fds, nfds, timeout);
}

VISIBLE ssize_t read(int fd, void* buffer, size_t n) {
  if (active_container == nullptr || is_non_blocking(fd))
    return sys_read(fd, buffer, n);
  single_poll(fd, POLLIN);
  return sys_read(fd, buffer, n);
}

VISIBLE ssize_t write(int fd, const void* buffer, size_t n) {
  if (active_container == nullptr || is_non_blocking(fd))
    return sys_write(fd, buffer, n);
  single_poll(fd, POLLOUT);
  return sys_write(fd, buffer, n);
}

VISIBLE ssize_t send(int fd, const void* buffer, size_t n, int flags) {
  return sendto(fd, buffer, n, flags, nullptr, 0);
}

VISIBLE ssize_t sendto(int fd, const void* buffer, size_t n, int flags,
                       const sockaddr* addr, socklen_t addrlen) {
  if (active_container == nullptr || is_non_blocking(fd))
    return sys_sendto(fd, buffer, n, flags, addr, addrlen);
  single_poll(fd, POLLOUT);
  return sys_sendto(fd, buffer, n, flags, addr, addrlen);
}

VISIBLE ssize_t recv(int fd, void* buffer, size_t n, int flags) {
  return recvfrom(fd, buffer, n, flags, nullptr, 0);
}

VISIBLE ssize_t recvfrom(int fd, void* buffer, size_t n, int flags,
                         sockaddr* addr, socklen_t addrlen) {
  if (active_container == nullptr || is_non_blocking(fd))
    return sys_sendto(fd, buffer, n, flags, addr, addrlen);
  single_poll(fd, POLLIN);
  return sys_recvfrom(fd, buffer, n, flags, addr, addrlen);
}

VISIBLE unsigned int sleep(unsigned int seconds) {
  poll(nullptr, 0, seconds * 1000);
  return 0;
}

VISIBLE int usleep(useconds_t seconds) {
  poll(nullptr, 0, seconds / 1000);
  if (auto remain = seconds % 1000)
    sys_usleep(remain);
  return 0;
}

VISIBLE int epoll_wait(int epfd, epoll_event* events, int maxevents,
                       int timeout) {
  if (active_container == nullptr)
    return sys_epoll_wait(epfd, events, maxevents, timeout);
  single_poll(epfd, POLLIN, timeout);
  return sys_epoll_wait(epfd, events, maxevents, 0);
}

} // extern "C"
} // namespace coroutine
} // namespace cbu
