/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2024, chys <admin@CHYS.INFO>
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

namespace cbu {

struct ReservedTag {};
inline constexpr ReservedTag kReserved;

struct UninitializedTag {};
inline constexpr UninitializedTag kUninitialized;

struct UnsafeTag {};
inline constexpr UnsafeTag kUnsafe;

struct RefTag {};
inline constexpr RefTag kRef;

// specifying radix
template <int>
struct RadixTag {};

using AutoRadixTag = RadixTag<0>;
using OctTag = RadixTag<8>;
using DecTag = RadixTag<10>;
using HexTag = RadixTag<16>;

// for str_to_number
struct IgnoreOverflowTag {};
inline constexpr IgnoreOverflowTag kIgnoreOverflow;

struct HaltScanOnOverflowTag {};
inline constexpr HaltScanOnOverflowTag kHaltScanOnOverflow;

template <unsigned long long>
struct OverflowThresholdTag {};

}  // namespace cbu
