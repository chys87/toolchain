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

#if __has_include(<sys/user.h>)
# include <sys/user.h>
#endif

namespace cbu {
namespace alloc {

#ifdef PAGE_SHIFT
constexpr unsigned kPageSizeBits = PAGE_SHIFT;
constexpr unsigned kPageSize = 1 << kPageSizeBits;
#else
constexpr unsigned kPageSizeBits = 12;
constexpr unsigned kPageSize = 1 << kPageSizeBits;
static_assert(kPageSize == 4096);
#endif

// Transparent Huge Page size
#if (defined __x86_64__ || defined __i386__ || defined __aarch64__) && \
    defined __linux__
constexpr unsigned kTHPSize = 2048 * 1024;
#else
constexpr unsigned kTHPSize = 0;
#endif

// Cache line
// We know for sure modern x86 and most modern aarch64 have 64-byte cache lines.
// This is probably also a good guess for other architectures.
constexpr unsigned kCacheLineSize = 64;

}  // namespace alloc
}  // namespace cbu
