/*
 * cbu - chys's basic utilities
 * Copyright (c) 2013-2023, chys <admin@CHYS.INFO>
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

#include <sys/types.h>

#if (!defined __GNUC__ || __GNUC__ < 4) && !defined __clang__
# error "Your compiler is not supported."
#endif

#if defined __x86_64__ || defined __aarch64__
# if !defined FSYSCALL_USE
#  define FSYSCALL_USE 1
# endif
#else
# undef FSYSCALL_USE
# define FSYSCALL_USE 0
#endif

struct fsys_linux_dirent64 {
  long long /* ino64_t */ d_ino;
  long long /* off64_t */ d_off;
  unsigned short d_reclen;
  unsigned char d_type; // In linux_dirent, it's after d_name;
                        // but in linux_dirent64, it's here.
  char d_name[1];

#ifdef __cplusplus
  // Return true if name is "." or ".."
  bool skip_dot() const {
    return (d_name[0] == '.' &&
            (d_name[1] == '\0' || (d_name[1] == '.' && d_name[2] == '\0')));
  }
#endif
};

// We cannot omit always_inline. At least vfork requires it.
#if defined __cplusplus
# define fsys_inline inline __attribute__((__always_inline__))
#elif defined __STDC__ && __STDC__ >= 199901L
# define fsys_inline static inline __attribute__((__always_inline__))
#else
# define fsys_inline static __inline__ __attribute__((__always_inline__))
#endif

#if defined __cplusplus && __cplusplus >= 201402L
# define fsys_constexpr inline constexpr __attribute__((__always_inline__))
#else
# define fsys_constexpr static inline __attribute__((__always_inline__))
#endif

#if FSYSCALL_USE

#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/syscall.h>

#ifdef __x86_64__
# include "fsyscall-macros-x86-64.h"
#elif defined __aarch64__
# include "fsyscall-macros-aarch64.h"
#endif

#define FSYS_MEMORY_1 "memory",
#define FSYS_MEMORY_0

#define fsys_errno(r,err) ((r) == (__typeof__(r))(-(err)))
#define fsys_errno_val(r) (-(int)(long long)(r))
#define fsys_failure(r) ((unsigned long long)(long long)(r) >= -4095ULL)
#define fsys_mmap_failed(r) ((unsigned long)(r) >= -4095UL)

struct rusage;
struct sockaddr;
struct stat;
struct statx;
struct timeval;
struct utsname;
struct statfs;
struct iovec;
struct msghdr;
struct mmsghdr;
struct epoll_event;
struct itimerspec;
struct rlimit;
struct pollfd;
struct utimbuf;
struct sysinfo;

// Glibc defines larger-than-necessary sigset_t.
// https://unix.stackexchange.com/questions/399342/why-is-sigset-t-in-glibc-musl-128-bytes-large-on-64-bit-linux
// We use only what the kernel wants.
typedef struct fsys_sigset_t {
  unsigned long __val[64 / sizeof(unsigned long)];
} fsys_sigset_t;

#if defined __cplusplus && __cplusplus >= 201103L
static_assert(_NSIG == 64 || _NSIG == 65, "_NSIG wrong");
#elif defined __STDC_VERSION__ && __STDC_VERSION__ >= 201112L
_Static_assert(_NSIG == 64 || _NSIG == 65, "_NSIG wrong");
#endif

// sigaction class used by Linux kernel, different from glibc's
struct fsys_sigaction {
#pragma push_macro("sa_handler")
#pragma push_macro("sa_sigaction")
#undef sa_handler
#undef sa_sigaction
  union {
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, siginfo_t *, void *);
  } __sigaction_handler;
#pragma pop_macro("sa_handler")
#pragma pop_macro("sa_sigaction")
  unsigned long sa_flags;
  void (*sa_restorer) (void);
  fsys_sigset_t sa_mask;
};

def_fsys(uname,uname,int,1,struct utsname *)
def_fsys(chdir,chdir,int,1,const char *)
def_fsys_nomem(fchdir,fchdir,int,1,int)
#ifdef __x86_64__
def_fsys(mkdir,mkdir,int,2,const char *, int)
def_fsys(rmdir,rmdir,int,1,const char *)
#else
#define fsys_mkdir(name,mode) fsys_mkdirat(AT_FDCWD,name,mode)
#define fsys_rmdir(name) fsys_unlinkat(AT_FDCWD,name,AT_REMOVEDIR)
#endif
def_fsys(mkdirat,mkdirat,int,3,int,const char *,int)
def_fsys_nomem(socket,socket,int,3,int,int,int)
def_fsys(bind,bind,int,3,int,const struct sockaddr *,unsigned long)
def_fsys_nomem(listen,listen,int,2,int,int)
def_fsys(accept,accept,int,3,int,struct sockaddr *,unsigned*)
def_fsys(accept4,accept4,int,4,int,struct sockaddr *,unsigned *,int)
def_fsys(socketpair,socketpair,int,4,int,int,int,int*)
def_fsys_nomem(shutdown,shutdown,int,2,int,int)
def_fsys(setsockopt,setsockopt,int,5,int,int,int,const void *,unsigned)
def_fsys(getsockopt,getsockopt,int,5,int,int,int,void*,unsigned*)
def_fsys(getsockname,getsockname,int,3,int,struct sockaddr*,unsigned*)
def_fsys_nomem(ioctl_long,ioctl,int,3,int,int,long)
def_fsys(ioctl_ptr,ioctl,int,3,int,int,void *)
def_fsys_nomem(fcntl_void,fcntl,int,2,int,int)
def_fsys_nomem(fcntl_long,fcntl,int,3,int,int,long)
def_fsys_nomem(fcntl_int,fcntl,int,3,int,int,int)
def_fsys(fcntl_ptr,fcntl,int,3,int,int,void *)
def_fsys(fcntl_cptr,fcntl,int,3,int,int,const void *)
def_fsys(prctl_ptr,prctl,int,2,int,void *)
def_fsys(prctl_cptr,prctl,int,2,int,const void *)
def_fsys_nomem(setpriority,setpriority,int,3,int,int,int)
// getpriority note: I removed this function.
// Its C prototype in libc is broken by design.
// I have difficulty emulating it properly.
// Use with fsys_generic_nomem if you know what you're doing.
#ifdef __x86_64__
def_fsys(symlink,symlink,int,2,const char *, const char *)
def_fsys(link,link,int,2,const char *,const char *)
def_fsys(unlink,unlink,int,1,const char *)
def_fsys(access,access,int,2,const char *,int)
def_fsys(readlink,readlink,long,3,const char *,char *, unsigned long)
#else
#define fsys_symlink(target,linkpath) fsys_symlinkat(target,AT_FDCWD,linkpath)
#define fsys_link(oldpath,newpath) fsys_linkat(AT_FDCWD,oldpath,AT_FDCWD,newpath,0)
#define fsys_unlink(path) fsys_unlinkat(AT_FDCWD,path,0)
#define fsys_access(path,mode) fsys_faccessat2(AT_FDCWD,path,mode,0)
#define fsys_readlink(name,buf,len) fsys_readlinkat(AT_FDCWD,name,buf,len)
#endif
def_fsys(readlinkat,readlinkat,long,4,int,const char *,char *, unsigned long)
#ifdef __x86_64__
def_fsys(chmod,chmod,int,2,const char *,int)
#else
#define fsys_chmod(filename,mode) fsys_fchmodat_raw(AT_FDCWD,filename,mode,0)
#endif
def_fsys_nomem(fchmod,fchmod,int,2,int,int)
def_fsys(fchmodat_raw,fchmodat,int,4,int,const char*,int,int)
def_fsys_nomem(readahead,readahead,long,3,int,long,unsigned long)
def_fsys_nomem(flock,flock,int,2,int,int)
def_fsys_nomem(umask,umask,int,1,int)
def_fsys_nomem(dup,dup,int,1,int)
#ifdef __x86_64__
def_fsys_nomem(dup2,dup2,int,2,int,int)
#else
#define fsys_dup2(a,b) fsys_dup3(a,b,0)
#endif
def_fsys_nomem(dup3,dup3,int,3,int,int,int)
#define fsys_pipe(fds) fsys_pipe2(fds,0)
def_fsys(pipe2,pipe2,int,2,int *,int)
#ifdef __x86_64__
def_fsys(stat,stat,int,2,const char *,struct stat*)
def_fsys(lstat,lstat,int,2,const char *,struct stat*)
#else
#define fsys_stat(name,st) fsys_fstatat(AT_FDCWD,name,st,0)
#define fsys_lstat(name,st) fsys_fstatat(AT_FDCWD,name,st,AT_SYMLINK_NOFOLLOW)
#endif
def_fsys(fstat,fstat,int,2,int,struct stat*)
def_fsys(fstatat,newfstatat,int,4,int,const char *,struct stat *,int)
def_fsys(statx,statx,int,5,int,const char *,int,unsigned,struct statx *)
def_fsys(getrusage,getrusage,int,2,int,struct rusage *)
def_fsys(utimensat,utimensat,int,4,int,const char *,const struct timespec *,
         int)
#define fsys_futimens(a,b) fsys_utimensat(a,0,b,0)
#ifdef __x86_64__
def_fsys(utime,utime,int,2,const char *,const struct utimbuf *)
def_fsys(utimes,utimes,int,2,const char *,const struct timeval *)
#else
#define fsys_utime utime
#define fsys_utimes utimes
#endif
#ifdef __x86_64__
def_fsys(rename,rename,int,2,const char *,const char *)
#else
#define fsys_rename(a,b) fsys_renameat(AT_FDCWD,a,AT_FDCWD,b)
#endif
def_fsys(renameat,renameat,int,4,int,const char *,int,const char *)
def_fsys(statfs,statfs,int,2,const char *,struct statfs *)
def_fsys(fstatfs,fstatfs,int,2,int,struct statfs *)
def_fsys(getdents64,getdents64,long,3,int,void *,unsigned long)
// def_fsys(fork,fork,int,0)
// Be careful it's semantics is different from the libc wrapper.
// Use it with caution!
#define fsys_fork() fsys_clone2(17/*SIGCHLD*/,0)
#ifdef __x86_64__
def_fsys(vfork,vfork,int,0)
#else
#define fsys_vfork vfork
#endif
def_fsys(clone2,clone,int,2,int,void *)
def_fsys_nomem(getpid,getpid,int,0)
def_fsys_nomem(gettid,gettid,int,0)
def_fsys_nomem(getppid,getppid,int,0)
def_fsys_nomem(getuid,getuid,unsigned,0)
def_fsys_nomem(geteuid,geteuid,unsigned,0)
def_fsys_nomem(getgid,getgid,unsigned,0)
def_fsys_nomem(getegid,getegid,unsigned,0)
def_fsys_nomem(getsid,getsid,int,1,int)
def_fsys_nomem(sched_yield,sched_yield,int,0)
#ifdef __cplusplus
def_fsys(futex4,futex,long,4,unsigned*,int,unsigned,const struct timespec *)
def_fsys(futex3,futex,long,3,unsigned*,int,unsigned)
def_fsys(futex4,futex,int,4,int *,int,int,const struct timespec *)
def_fsys(futex3,futex,int,3,int *,int,int)
#else  // For C, which doesn't support overloading, we're foced to use void*
       // to avoid breaking old code.
       // Clang supoprts attribute overloadable, but we choose not to bother.
def_fsys(futex4,futex,long,4,void*,int,int,const struct timespec *)
def_fsys(futex3,futex,long,3,void*,int,int)
#endif
def_fsys(mmap,mmap,void*,6,void*,unsigned long,int,int,int,long)
def_fsys(munmap,munmap,int,2,void*,unsigned long)
def_fsys(madvise,madvise,int,3,void*,unsigned long,int)
def_fsys(mremap,mremap,void*,4,void*,unsigned long,unsigned long,int)
def_fsys(mprotect,mprotect,int,3,void*,unsigned long,int)
// Should be sigset_t or fsys_sigset_t
def_fsys(rt_sigprocmask,rt_sigprocmask,int,
         4,int,const void *,void *,unsigned long)
#define fsys_sigprocmask(a,b,c) fsys_rt_sigprocmask(a,b,c,_NSIG/8)
def_fsys_nomem(setsid,setsid,int,0)
def_fsys_nomem(kill,kill,int,2,int,int)
def_fsys_nomem(tkill,tkill,int,2,int,int)
def_fsys_nomem(tgkill,tgkill,int,3,int,int,int)
def_fsys(rt_sigaction,rt_sigaction,int,
         4,int,const struct fsys_sigaction *,struct fsys_sigaction *,
         unsigned long)
#define fsys_sigaction(a,b,c) fsys_rt_sigaction(a,b,c,_NSIG/8)
#ifdef __x86_64__
def_fsys_nomem(alarm,alarm,int,1,int)
#else
#define fsys_alarm alarm
#endif
def_fsys(setrlimit,setrlimit,int,2,int,const struct rlimit *)
def_fsys(getrlimit,getrlimit,int,2,int,struct rlimit *)
def_fsys(sysinfo,sysinfo,int,1,struct sysinfo *)
def_fsys_nomem(fadvise64,fadvise64,int,4,int,__OFF64_T_TYPE,__OFF64_T_TYPE,int)
def_fsys_nomem(fallocate,fallocate,int,4,int,int,long,long)

// Glibc wrappers of many of the following syscalls do some bookkeeping
// related to asynchronous cancelation.
// We don't need to do that because we never cancel threads
#ifdef __x86_64__
def_fsys(open2_raw,open,int,2,const char *,int)
def_fsys(open3_raw,open,int,3,const char *,int,int)
#endif
#define fsys_open2(a,b) fsys_openat3(AT_FDCWD,a,b)
#define fsys_open3(a,b,c) fsys_openat4(AT_FDCWD,a,b,c)
def_fsys(openat3,openat,int,3,int,const char *,int)
def_fsys(openat4,openat,int,4,int,const char *,int,int)
def_fsys_nomem(lseek,lseek,__OFF64_T_TYPE,3,int,__OFF64_T_TYPE,int)
def_fsys(connect,connect,int,3,int,const struct sockaddr *,unsigned long)
def_fsys(read,read,long,3,int,void *,unsigned long)
def_fsys(write,write,long,3,int,const void *,unsigned long)
def_fsys(writev,writev,long,3,int,const struct iovec *,unsigned long)
def_fsys(pwritev_raw,pwritev,long,5,int,const struct iovec *,
         unsigned long,unsigned long, __OFF64_T_TYPE)
#define fsys_pwritev(a,b,c,d) fsys_pwritev_raw(a,b,c,d,0)
def_fsys(pwrite,pwrite64,long,4,int,const void *,unsigned long, __OFF64_T_TYPE)
def_fsys(pread,pread64,long,4,int,void *,unsigned long, __OFF64_T_TYPE)
def_fsys(tee,tee,long,4,int,int,unsigned long,unsigned)
def_fsys(splice,splice,long,6,int,__OFF64_T_TYPE *,int,__OFF64_T_TYPE *,
         unsigned long,unsigned)
def_fsys(vmsplice,vmsplice,long,4,int,const struct iovec *,unsigned long,
         unsigned int)
def_fsys(sendfile,sendfile,long,4,int,int,__OFF64_T_TYPE *,unsigned long)
def_fsys(sendto,sendto,long,6,int,const void *,unsigned long,int,
         const struct sockaddr *,unsigned long)
def_fsys(sendmsg,sendmsg,long,3,int,const struct msghdr *, int)
def_fsys(sendmmsg,sendmmsg,int,4,int,struct mmsghdr*,unsigned,int)
def_fsys(recvfrom,recvfrom,long,6,int,void *,unsigned long,int,
         struct sockaddr *,unsigned *)
def_fsys(recvmsg,recvmsg,long,3,int,struct msghdr *,int)
def_fsys(recvmmsg,recvmmsg,int,5,int,struct mmsghdr*,unsigned,int,struct timespec*)
#define fsys_send(a,b,c,d) fsys_sendto(a,b,c,d,0,0)
#define fsys_recv(a,b,c,d) fsys_recvfrom(a,b,c,d,0,0)
def_fsys_nomem(fsync,fsync,int,1,int)
def_fsys_nomem(fdatasync,fdatasync,int,1,int)
def_fsys(msync,msync,int,3,void *,unsigned long,int)
def_fsys_nomem(close,close,int,1,int)
def_fsys_nomem(ftruncate,ftruncate,int,2,int,long)
def_fsys(fsetxattr,fsetxattr,int,5,int,const char *,const void *,long,int)
def_fsys(fgetxattr,fgetxattr,long,4,int,const char *,void *,unsigned long)
#define fsys_epoll_create(ignored) fsys_epoll_create1(0)
def_fsys_nomem(epoll_create1,epoll_create1,int,1,int)
def_fsys(epoll_ctl,epoll_ctl,int,4,int,int,int,struct epoll_event *)
#ifdef __x86_64__
def_fsys(epoll_wait,epoll_wait,int,4,int,struct epoll_event *,int,int)
#else
#define fsys_epoll_wait(fd,events,n,timeout) fsys_epoll_pwait(fd,events,n,timeout,0)
#endif
#define fsys_epoll_pwait(fd,events,n,timeout,sigmask) fsys_epoll_pwait_raw(fd,events,n,timeout,sigmask,_NSIG/8)
def_fsys(epoll_pwait_raw,epoll_pwait,int,6,int,struct epoll_event *,int,int,const void*,unsigned long)
def_fsys(ppoll_raw,ppoll,int,5,struct pollfd*,unsigned long,struct timespec*,const void*,unsigned long)

fsys_inline int fsys_ppoll(struct pollfd* fds, unsigned long nfds, const struct timespec* tmo, const void* sigmask) {
  // syscall has tmo modified, but glibc ppoll doesn't.  This is ridiculous.
  struct timespec tmobuf;
  if (tmo) tmobuf = *tmo;
  return fsys_ppoll_raw(fds, nfds, tmo ? &tmobuf : NULL, sigmask, _NSIG / 8);
}

#ifdef __x86_64__
def_fsys(select,select,int,5,int,/*fd_set*/void*,void*,void*,struct timeval*)
def_fsys(poll,poll,int,3,struct pollfd *,unsigned long,int)
#else
#define fsys_select select
fsys_inline int fsys_poll(struct pollfd* fds, unsigned long nfds, int timeout) {
  struct timespec tmobuf;
  if (timeout >= 0) {
    tmobuf.tv_sec = (unsigned)timeout / 1000;
    tmobuf.tv_nsec = (unsigned)timeout % 1000 * 1000000;
  }
  return fsys_ppoll(fds, nfds, timeout >= 0 ? &tmobuf : NULL, NULL);
}
#endif
def_fsys(signalfd4,signalfd4,int,4,int,const void/*sigset_t*/*,unsigned long,
         int)
#define fsys_signalfd(a,b,c) fsys_signalfd4(a,b,_NSIG/8,c)
def_fsys_nomem(timerfd_create,timerfd_create,int,2,int,int)
def_fsys(timerfd_settime,timerfd_settime,int,4,int,int,
         const struct itimerspec *,struct itimerspec *)
def_fsys_nomem(eventfd,eventfd2,int,2,unsigned,int)
#ifdef __x86_64__
def_fsys_nomem(inotify_init,inotify_init,int,0)
#else
#define fsys_inotify_init() fsys_inotify_init1(0)
#endif
def_fsys_nomem(inotify_init1,inotify_init1,int,1,int)
def_fsys(inotify_add_watch,inotify_add_watch,int,3,int,const char *,unsigned)
def_fsys_nomem(inotify_rm_watch,inotify_rm_watch,int,2,int,int)
def_fsys(nanosleep_raw,nanosleep,int,2,const struct timespec *,struct timespec *)
#define fsys_nanosleep(a,b) fsys_clock_nanosleep(0,0,a,b)
def_fsys(clock_nanosleep,clock_nanosleep,int,4,int,int,const struct timespec*, struct timespec*)
def_fsys(clock_gettime_raw,clock_gettime,int,2,int,struct timespec*)
// It's important to note that the caller cannot reliably determine the
// return value convention of this function, so it should really only compare
// it against 0 and not attempt to check for errno.
fsys_inline int fsys_clock_gettime_auto(clockid_t clk, struct timespec* ts) {
  // linux/include/vdso/datapage.h enumerates clocks that can be handled
  // without falling back to real system calls.
  // If we know at compile time that the clock can never be handled without
  // falling back to real system calls, we emit syscall instruction directly.
  if (__builtin_constant_p(clk) &&
      clk != CLOCK_REALTIME &&
      clk != CLOCK_MONOTONIC &&
      clk != CLOCK_BOOTTIME &&
      clk != CLOCK_TAI &&
      clk != CLOCK_REALTIME_COARSE &&
      clk != CLOCK_MONOTONIC_COARSE &&
      clk != CLOCK_MONOTONIC_RAW)
    return fsys_clock_gettime_raw(clk, ts);
  else
    return clock_gettime(clk, ts);
}
def_fsys(linkat,linkat,int,5,int,const char *,int, const char *, int)
def_fsys(unlinkat,unlinkat,int,3,int,const char *,int)
def_fsys(symlinkat,symlinkat,int,3,const char *,int,const char *)
def_fsys(faccessat_raw,faccessat,int,3,int,const char *,int)
def_fsys(faccessat2,faccessat2,int,4,int,const char *,int,int)
#define fsys_faccessat(a,b,c,d) fsys_faccessat2(a,b,c,d)
def_fsys(wait4,wait4,int,4,int,int *,int,struct rusage *)
#define fsys_waitpid(a,b,c) fsys_wait4(a,b,c,0)
def_fsys(waitid_raw,waitid,int,5,int,int,void*,int,struct rusage*)
#define fsys_waitid(a,b,c,d) fsys_waitid_raw(a,b,c,d,(struct rusage*)0)
def_fsys(execve,execve,int,3,const char *,char *const*,char *const *)
def_fsys(execveat,execveat,int,5,int,const char*,char*const*,char*const*,int)
def_fsys(exit_group,exit_group,int,1,int)
def_fsys(mount,mount,int,5,const char *,const char *,const char *,
         unsigned long,const void *)
def_fsys(umount2,umount2,int,2,const char *,int)
def_fsys(pivot_root,pivot_root,int,2,const char *,const char *)
def_fsys(memfd_create,memfd_create,int,2,const char *,unsigned)
def_fsys_nomem(copy_file_range,copy_file_range,long,6,int,long*,int,long*,unsigned long,unsigned)

// vsyscall is nowadays deprecated; We should use vDSO instead,
// of which modern glibc takes good care.
#define fsys_time time
#define fsys_gettimeofday gettimeofday

#ifdef ASSUME_RDTSCP
fsys_inline int fsys_sched_getcpu(void) {
  unsigned eax, edx, ecx;
  __asm__ __volatile__ ("rdtscp" : "=a"(eax), "=d"(edx), "=c"(ecx));
  return ecx & 0xfff;
}
#else
// vsyscall is nowadays deprecated; We should use vDSO instead,
// of which modern glibc takes good care.
# define fsys_sched_getcpu sched_getcpu
#endif

fsys_inline void fsys__exit (int x) {
  fsys_exit_group (x);
  __builtin_trap ();
}

fsys_inline int fsys_posix_fadvise(int fd, __OFF64_T_TYPE off,
                                   __OFF64_T_TYPE len, int advice) {
  return -fsys_fadvise64(fd, off, len, advice);
}

#else

#define FSYSCALL_USE 0

#include <unistd.h>
#include <sys/syscall.h>

#define fsys_generic(sysno,rettype,argc,...) \
  ((rettype)syscall(sysno,##__VA_ARGS__))
#define fsys_generic_nomem fsys_generic

#define fsys_errno(r,err) (errno==err)
#define fsys_errno_val(r) errno
#define fsys_failure(r) ((long)(r) < 0)
#define fsys_mmap_failed(r)	((r) == MAP_FAILED)

#define fsys_sigset_t sigset_t

#define fsys_uname uname
#define fsys_chdir chdir
#define fsys_fchdir fchdir
#define fsys_mkdir mkdir
#define fsys_mkdirat mkdirat
#define fsys_rmdir rmdir
#define fsys_time time
#define fsys_gettimeofday gettimeofday
#define fsys_socket socket
#define fsys_bind bind
#define fsys_listen listen
#define fsys_accept accept
#define fsys_accept4 accept4
#define fsys_socketpair socketpair
#define fsys_shutdown shutdown
#define fsys_setsockopt setsockopt
#define fsys_getsockopt getsockopt
#define fsys_getsockname getsockname
#define fsys_ioctl_long ioctl
#define fsys_ioctl_ptr ioctl
#define fsys_fcntl_void fcntl
#define fsys_fcntl_long fcntl
#define fsys_fcntl_int fcntl
#define fsys_fcntl_ptr fcntl
#define fsys_fcntl_cptr fcntl
#define fsys_prctl_ptr prctl
#define fsys_prctl_cptr prctl
#define fsys_setpriority setpriority
#define fsys_symlink symlink
#define fsys_link link
#define fsys_unlink unlink
#define fsys_access access
#define fsys_readlink readlink
#define fsys_readlinkat readlinkat
#define fsys_chmod chmod
#define fsys_fchmod fchmod
#define fsys_readahead readahead
#define fsys_flock flock
#define fsys_umask umask
#define fsys_dup dup
#define fsys_dup2 dup2
#define fsys_dup3 dup3
#define fsys_pipe pipe
#define fsys_pipe2 pipe2
#define fsys_stat stat
#define fsys_lstat lstat
#define fsys_fstat fstat
#define fsys_fstatat fstatat
#define fsys_statx statx
#define fsys_getrusage getrusage
#define fsys_utimensat utimensat
#define fsys_futimens futimens
#define fsys_utime utime
#define fsys_utimes utimes
#define fsys_sched_getcpu sched_getcpu
#define fsys_rename rename
#define fsys_renameat renameat
#define fsys_statfs statfs
#define fsys_fstatfs fstatfs
#define fsys_getdents(...) syscall(__NR_getdents,__VA_ARGS__)
#define fsys_getdents64(...) syscall(__NR_getdents64,__VA_ARGS__)
#define fsys_fork fork
#define fsys_vfork vfork
//#define fsys_clone2(a,b) // Undefined because it's not always safe to use
#define fsys_getpid getpid
#if defined __GLIBC_PREREQ && __GLIBC_PREREQ(2, 30)
# define fsys_gettid gettid
#else
# define fsys_gettid() ((pid_t)syscall(__NR_gettid))
#endif
#define fsys_getppid getppid
#define fsys_getuid getuid
#define fsys_geteuid geteuid
#define fsys_getgid getgid
#define fsys_getegid getegid
#define fsys_getsid getsid
#define fsys_sched_yield sched_yield
#define fsys_futex4(a,b,c,d) syscall(__NR_futex,a,b,c,d)
#define fsys_futex3(a,b,c) syscall(__NR_futex,a,b,c)
#define fsys_mmap mmap
#define fsys_munmap munmap
#define fsys_madvise madvise
#define fsys_mremap mremap
#define fsys_mprotect mprotect
#define fsys_sigprocmask sigprocmask
#define fsys_setsid setsid
#define fsys_kill kill
#define fsys_tkill tkill
#define fsys_tgkill tgkill
#define fsys_sigaction sigaction
#define fsys_alarm alarm
#define fsys_setrlimit setrlimit
#define fsys_getrlimit getrlimit
#define fsys_sysinfo sysinfo
#define fsys_posix_fadvise posix_fadvise
#define fsys_fallocate fallocate
#define fsys_open2 open
#define fsys_open3 open
#define fsys_openat3 openat
#define fsys_openat4 openat
#define fsys_lseek lseek
#define fsys_connect connect
#define fsys_read read
#define fsys_write write
#define fsys_writev writev
#define fsys_pwritev pwritev
#define fsys_pwrite pwrite
#define fsys_pread pread
#define fsys_tee tee
#define fsys_splice splice
#define fsys_vmsplice vmsplice
#define fsys_sendfile sendfile
#define fsys_sendto sendto
#define fsys_send send
#define fsys_sendmsg sendmsg
#define fsys_sendmmsg sendmmsg
#define fsys_recvfrom recvfrom
#define fsys_recvmsg recvmsg
#define fsys_recvmmsg recvmmsg
#define fsys_recv recv
#define fsys_fsync fsync
#define fsys_fdatasync fdatasync
#define fsys_msync msync
#define fsys_close close
#define fsys_ftruncate ftruncate
#define fsys_fsetxattr fsetxattr
#define fsys_fgetxattr fgetxattr
#define fsys_epoll_create epoll_create
#define fsys_epoll_create1 epoll_create1
#define fsys_epoll_ctl epoll_ctl
#define fsys_epoll_wait epoll_wait
#define fsys_select select
#define fsys_ppoll ppoll
#define fsys_poll poll
#define fsys_signalfd signalfd
#define fsys_timerfd_create timerfd_create
#define fsys_timerfd_settime timerfd_settime
#define fsys_eventfd eventfd
#define fsys_inotify_init inotify_init
#define fsys_inotify_init1 inotify_init1
#define fsys_inotify_add_watch inotify_add_watch
#define fsys_inotify_rm_watch inotify_rm_watch
#define fsys_nanosleep nanosleep
#define fsys_clock_nanosleep clock_nanosleep
#define fsys_clock_gettime_auto clock_gettime
#define fsys_linkat linkat
#define fsys_unlinkat unlinkat
#define fsys_symlinkat symlinkat
#define fsys_faccessat faccessat
#define fsys_wait4 wait4
#define fsys_waitpid waitpid
#define fsys_waitid waitid
#define fsys_execve execve
#define fsys_execveat execveat
#define fsys__exit _exit
#define fsys_mount mount
#define fsys_umount2 umount2
#define fsys_pivot_root pivot_root
#define fsys_memfd_create memfd_create
#define fsys_copy_file_range copy_file_range

#endif

// We always provide these functions to provide constexpr-ness
fsys_constexpr int fsys_sigemptyset(fsys_sigset_t* s) {
  for (size_t i = 0; i < sizeof(s->__val) / sizeof(s->__val[0]); ++i)
    s->__val[i] = 0;
  return 0;
}
fsys_constexpr int fsys_sigfillset(fsys_sigset_t* s) {
  for (size_t i = 0; i < sizeof(s->__val) / sizeof(s->__val[0]); ++i)
    s->__val[i] = -1;
  return 0;
}
fsys_constexpr int fsys_sigaddset(fsys_sigset_t* s, unsigned int n) {
  s->__val[(n - 1) / (8 * sizeof(s->__val[0]))] |=
      1ul << ((n - 1) % (8 * sizeof(s->__val[0])));
  return 0;
}
fsys_constexpr int fsys_sigdelset(fsys_sigset_t* s, unsigned int n) {
  s->__val[(n - 1) / (8 * sizeof(s->__val[0]))] &=
      ~(1ul << ((n - 1) % (8 * sizeof(s->__val[0]))));
  return 0;
}
fsys_constexpr int fsys_sigismember(const fsys_sigset_t* s, unsigned int n) {
  return !!(s->__val[(n - 1) / (8 * sizeof(s->__val[0]))] &
            (1ul << ((n - 1) % (8 * sizeof(s->__val[0])))));
}
