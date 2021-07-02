/*
 * cbu - chys's basic utilities
 * Copyright (c) 2019-2021, chys <admin@CHYS.INFO>
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

// Defines a fifo_list<T> container, which is a first-in-first-out
// singly-linked list.
//
// It offers push_back, pop_front, and fast access to both front and back
// elements.

#pragma once

#include <forward_list>

namespace cbu {

template <typename T>
class fifo_list {
 public:
  using forward_list = std::forward_list<T>;
  using value_type = T;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using iterator = typename forward_list::iterator;
  using const_iterator = typename forward_list::const_iterator;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

 public:
  fifo_list() noexcept : fl_(), back_it_(fl_.before_begin()) {}
  fifo_list(const fifo_list&) = delete;
  fifo_list(fifo_list&& o) noexcept :
      fl_(std::move(o.fl_)),
      back_it_(fl_.empty() ? fl_.before_begin() : o.back_it_) {
    o.back_it_ = o.fl_.before_begin();
  }

  fifo_list& operator = (const fifo_list&) = delete;
  fifo_list& operator = (fifo_list&& o) noexcept { swap(o); return *this; }

  void swap(fifo_list& o) noexcept {
    fl_.swap(o.fl_);
    std::swap(back_it_, o.back_it_);
    if (o.fl_.empty())
      o.back_it_ = o.fl_.before_begin();
    if (fl_.empty())
      back_it_ = fl_.before_begin();
  }

  iterator push_back(const T& val) { return emplace_back(val); }
  template <typename... Args> iterator emplace_back(Args&&... args);

  reference front() noexcept { return fl_.front(); }
  const_reference front() const noexcept { return fl_.front(); }
  reference back() noexcept { return *back_it_; }
  const_reference back() const noexcept { return *back_it_; }

  [[nodiscard]] bool empty() const noexcept { return fl_.empty(); }

  void pop_front() noexcept;

  iterator begin() noexcept { return fl_.begin(); }
  iterator end() noexcept { return fl_.end(); }
  const_iterator begin() const noexcept { return fl_.begin(); }
  const_iterator end() const noexcept { return fl_.end(); }
  const_iterator cbegin() const noexcept { return fl_.cbegin(); }
  const_iterator cend() const noexcept { return fl_.cend(); }

 private:
  forward_list fl_;
  iterator back_it_;
};

template <typename T>
template <typename... Args>
typename fifo_list<T>::iterator fifo_list<T>::emplace_back(Args&&... args) {
  back_it_ = fl_.emplace_after(back_it_, std::forward<Args>(args)...);
  return back_it_;
}

template <typename T>
void fifo_list<T>::pop_front() noexcept {
  fl_.pop_front();
  if (fl_.empty())
    back_it_ = fl_.before_begin();
}

} // namespace cbu
