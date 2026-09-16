/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2026, chys <admin@CHYS.INFO>
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
 * WARRANTIES OF MERCHANTABILITY AND FITNESS IN A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL chys <admin@CHYS.INFO> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cbu/io/scoped_mmap.h"

#include <sys/mman.h>

#include <gtest/gtest.h>

#include <utility>

namespace cbu {
namespace {

TEST(ScopedMMapTest, Move) {
  // The move constructor used not to compile (single-argument std::exchange)
  ScopedMMap<> a;
  ASSERT_EQ(a.ptr(), nullptr);
  ASSERT_EQ(a.size(), 0);

  void* p = mmap(nullptr, 4096, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(p, MAP_FAILED);
  ScopedMMap<> b(static_cast<char*>(p), 4096);

  ScopedMMap<> c(std::move(b));
  ASSERT_EQ(c.ptr(), p);
  ASSERT_EQ(c.size(), 4096);
  ASSERT_EQ(b.ptr(), nullptr);
  ASSERT_EQ(b.size(), 0);

  ScopedMMap<> d;
  d = std::move(c);
  ASSERT_EQ(d.ptr(), p);
  ASSERT_EQ(d.size(), 4096);

  d.reset();
  ASSERT_EQ(d.ptr(), nullptr);
  ASSERT_EQ(d.size(), 0);
}

}  // namespace
}  // namespace cbu
