/*
 * cbu - chys's basic utilities
 * Copyright (c) 2022-2023, chys <admin@CHYS.INFO>
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
#  define CBU_ADDRESS_SANITIZER
#  define CBU_DISABLE_ADDRESS_SANITIZER [[gnu::no_sanitize("address")]]
#elif defined __has_feature
#  if __has_feature(address_sanitizer)
#    define CBU_ADDRESS_SANITIZER
#    define CBU_DISABLE_ADDRESS_SANITIZER [[clang::no_sanitize("address")]]
#  endif
#endif

#ifndef CBU_DISABLE_ADDRESS_SANITIZER
#  define CBU_DISABLE_ADDRESS_SANITIZER
#endif
