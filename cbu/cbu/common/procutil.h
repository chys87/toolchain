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

#include "cbu/common/fixed_string.h"
#include "cbu/common/shared_instance.h"

namespace cbu {

// Print a message and abort
[[noreturn, gnu::cold]] void fatal_length_prefixed(const char* info) noexcept;

template <cbu::fixed_string msg>
  requires(msg.size() > 0 && msg[msg.size() - 1] == '\n')
[[noreturn, gnu::always_inline]] inline void fatal() noexcept {
  constexpr auto str =
      LittleEndianFixedString<static_cast<unsigned char>(msg.size())>() + msg;
  fatal_length_prefixed(as_shared<str>.data());
}

template <cbu::fixed_string msg>
  requires(msg.size() == 0 || msg[msg.size() - 1] != '\n')
[[noreturn, gnu::always_inline]] inline void fatal() noexcept {
  constexpr auto str =
      LittleEndianFixedString<static_cast<unsigned char>(msg.size())>() + msg +
      LittleEndianFixedString<static_cast<unsigned char>('\n')>();
  fatal_length_prefixed(as_shared<str>.data());
}

}  // namespace cbu