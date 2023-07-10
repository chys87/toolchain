/*
 * cbu - chys's basic utilities
 * Copyright (c) 2021-2023, chys <admin@CHYS.INFO>
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

// This file provides utilities to write to memory bypassing cache.
// Don't use this unless you know what you're doing.
//
// Currently this is only implemented for x86-64 (i386 also supports some
// of the features, but nobody uses it nowadays.)

#include "cbu/sys/cacheless.h"

#include <gtest/gtest.h>

namespace cbu {
namespace cacheless {
namespace {

class CachelessTest : public testing::Test {
 public:
  void ResetDst() { memset(dst_, 0, sizeof(dst_)); }
  void ResetSrc() {
    unsigned i = 0;
    for (char& c : src_) c = 'A' + (i++ % 26);
  }
  void Reset() {
    ResetDst();
    ResetSrc();
  }

  void VerifyCopy(size_t off, size_t size) {
    for (size_t i = 0; i < off; ++i) {
      ASSERT_EQ(dst_[i], '\0')
        << "i=" << i << ", off=" << off << ", size=" << size;
    }
    for (size_t i = off; i < off + size; ++i) {
      ASSERT_EQ(dst_[i], src_[i])
        << "i=" << i << ", off=" << off << ", size=" << size;
    }
    for (size_t i = off + size; i < off + size + 64; ++i) {
      ASSERT_EQ(dst_[i], '\0')
        << "i=" << i << ", off=" << off << ", size=" << size;
    }
  }

  template <typename T>
  void VerifyFill(size_t off, size_t size, T value) {
    for (size_t i = 0; i < off; ++i) {
      ASSERT_EQ(dst_[i], '\0')
          << "i=" << i << ", off=" << off << ", size=" << size;
    }
    for (size_t i = 0; i < size; ++i) {
      ASSERT_EQ(*reinterpret_cast<const T*>(dst_ + off + i * sizeof(T)), value)
          << "i=" << i << ", off=" << off << ", size=" << size;
    }
    for (size_t i = off + size * sizeof(T); i < off + size * sizeof(T) + 64;
         ++i) {
      ASSERT_EQ(dst_[i], '\0')
          << "i=" << i << ", off=" << off << ", size=" << size;
    }
  }

 protected:
  static constexpr size_t N = 16384;
  static constexpr size_t BUF = N + 256;
  char dst_[BUF];
  char src_[BUF];
};

TEST_F(CachelessTest, SingleStoreTest) {
  store(dst_ + 1, mempick4(src_ + 1));
  VerifyCopy(1, 4);

  ResetDst();
  store(dst_ + 1, mempick8(src_ + 1));
  VerifyCopy(1, 8);
}

TEST_F(CachelessTest, CopyTest) {
  for (size_t off = 0; off < 32; off += off / 16 * 3 + 1) {
    for (size_t size = 0; size < N; size += size / 64 + size / 128 * 71 + 1) {
      Reset();
      ASSERT_EQ(copy(dst_ + off, src_ + off, size), dst_ + off + size);
      VerifyCopy(off, size);
    }
  }
}

TEST_F(CachelessTest, FillTest) {
  for (size_t off = 0; off < 32; off += off / 16 * 3 + 1) {
    for (size_t size = 0; size < N; size += size / 64 + size / 128 * 71 + 1) {
      auto do_it = [&](auto value) {
        constexpr size_t U = sizeof(value);
        ResetDst();
        ASSERT_EQ(fill(dst_ + off / U * U, value, size / U),
                  dst_ + off / U * U + size / U * U);
        ASSERT_NO_FATAL_FAILURE(VerifyFill(off / U * U, size / U, value));
      };

      ASSERT_NO_FATAL_FAILURE(do_it(uint8_t(42)));
      ASSERT_NO_FATAL_FAILURE(do_it(uint16_t(2554)));
      ASSERT_NO_FATAL_FAILURE(do_it(uint32_t(25542554)));
      ASSERT_NO_FATAL_FAILURE(do_it(uint64_t(2554255425542554ULL)));
    }
  }
}

} // namespace
} // namespace cacheless
} // namespace
