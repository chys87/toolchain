/*
 * cbu - chys's basic utilities
 * Copyright (c) 2022-2025, chys <admin@CHYS.INFO>
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

#define CBU_TRIVIAL_ABI
#define CBU_STATIC_CALL
#define CBU_STATIC_CALL_CONST const

#ifdef __clang__
#  undef CBU_TRIVIAL_ABI
#  define CBU_TRIVIAL_ABI [[clang::trivial_abi]]
#endif

#ifdef __cpp_static_call_operator
#  undef CBU_STATIC_CALL
#  undef CBU_STATIC_CALL_CONST
#  define CBU_STATIC_CALL static
#  define CBU_STATIC_CALL_CONST
#endif

#ifdef __SANITIZE_ADDRESS__
#  define CBU_ADDRESS_SANITIZER 1
#  define CBU_DISABLE_ADDRESS_SANITIZER [[gnu::no_sanitize("address")]]
#elif defined __has_feature
#  if __has_feature(address_sanitizer)
#    define CBU_ADDRESS_SANITIZER 1
#    define CBU_DISABLE_ADDRESS_SANITIZER [[clang::no_sanitize("address")]]
#  endif
#endif

#ifndef CBU_DISABLE_ADDRESS_SANITIZER
#  define CBU_DISABLE_ADDRESS_SANITIZER
#endif

#ifndef CBU_ADDRESS_SANITIZER
#  define CBU_ADDRESS_SANITIZER 0
#endif

// Some early clang versions recognize them but segfaults
#if defined __clang__ && __clang_major__ >= 17 && \
    (defined __x86_64__ || defined __aarch64__)
#define CBU_PRESERVE_ALL [[clang::preserve_all]]
#define CBU_PRESERVE_MOST [[clang::preserve_most]]
#if __clang_major__ >= 19
#define CBU_PRESERVE_NONE [[clang::preserve_none]]
#else
#define CBU_PRESERVE_NONE
#endif
#else
#define CBU_PRESERVE_ALL
#define CBU_PRESERVE_MOST
#define CBU_PRESERVE_NONE
#endif

// preserve_all and preserve_most are obviously more useful on aarch64 than
// x86-64
#ifdef __aarch64__
#define CBU_AARCH64_PRESERVE_ALL CBU_PRESERVE_ALL
#define CBU_AARCH64_PRESERVE_MOST CBU_PRESERVE_MOST
#define CBU_AARCH64_PRESERVE_NONE CBU_PRESERVE_NONE
#else
#define CBU_AARCH64_PRESERVE_ALL
#define CBU_AARCH64_PRESERVE_MOST
#define CBU_AARCH64_PRESERVE_NONE
#endif

#ifdef __clang__
#define CBU_NAIVE_LOOP _Pragma("clang loop vectorize(disable) unroll(disable)")
#else
#define CBU_NAIVE_LOOP
#endif

#ifdef __clang__
#define CBU_NO_BUILTIN __attribute__((no_builtin))
#else
#define CBU_NO_BUILTIN __attribute__((optimize("-fno-builtin")))
#endif


#define CBU_NO_DESTROY
#define CBU_LIFETIME_BOUND

#ifdef __has_cpp_attribute
#if __has_cpp_attribute(clang::no_destroy)
#  undef CBU_NO_DESTROY
#  define CBU_NO_DESTROY [[clang::no_destroy]]
#endif

#if __has_cpp_attribute(clang::lifetimebound)
#  undef CBU_LIFETIME_BOUND
#  define CBU_LIFETIME_BOUND [[clang::lifetimebound]]
#endif
#endif
