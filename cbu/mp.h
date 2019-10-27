/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019, chys <admin@CHYS.INFO>
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

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace cbu {
namespace mp {

#if __WORDSIZE == 64 || defined __x86_64__
using Word = std::uint64_t;
using DWord = unsigned __int128;
#else
using Word = std::uint32_t;
using DWord = std::uint64_t;
#endif

size_t mul(Word *r, const Word *a, size_t na, Word b) noexcept;
size_t mul(Word *r, const Word *a, size_t na,
           const Word *b, size_t nb) noexcept;
size_t add(Word *r, const Word *a, size_t na,
           const Word *b, size_t nb) noexcept;
size_t add(Word *r, const Word *a, size_t na, Word b) noexcept;
std::pair<size_t, Word> div(Word *r, const Word *a, size_t na, Word b) noexcept;

int compare(const Word *a, size_t na, const Word *b, size_t nb) noexcept;
bool eq(const Word *a, size_t na, const Word *b, size_t nb) noexcept;
bool ne(const Word *a, size_t na, const Word *b, size_t nb) noexcept;
bool gt(const Word *a, size_t na, const Word *b, size_t nb) noexcept;
bool lt(const Word *a, size_t na, const Word *b, size_t nb) noexcept;
bool ge(const Word *a, size_t na, const Word *b, size_t nb) noexcept;
bool le(const Word *a, size_t na, const Word *b, size_t nb) noexcept;

size_t from_dec(Word *r, const char *s, size_t n) noexcept;
char *to_dec(char *r, const Word *s, size_t n) noexcept;
std::string to_dec(const Word *s, size_t n) noexcept;

size_t from_hex(Word *r, const char *s, size_t n) noexcept;
char *to_hex(char *r, const Word *s, size_t n) noexcept;
std::string to_hex(const Word *s, size_t n) noexcept;

size_t from_oct(Word *r, const char *s, size_t n) noexcept;
char *to_oct(char *r, const Word *s, size_t n) noexcept;
std::string to_oct(const Word *s, size_t n) noexcept;

size_t from_bin(Word *r, const char *s, size_t n) noexcept;
char *to_bin(char *r, const Word *s, size_t n) noexcept;
std::string to_bin(const Word *s, size_t n) noexcept;

} // namespace mp
} // namespace cbu
