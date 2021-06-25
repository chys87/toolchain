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

#pragma once

#include <assert.h>

// Adapted from jemalloc's rb implementation
// This implementation requires all elements to be unique, which is the caller's
// responsibility to guarantee.

namespace cbu {
namespace alloc {

template <typename Node>
struct RbLink {
  Node* left_;
  uintptr_t right_color_;

  Node* get(bool lft) const noexcept {
    // Use this trick to generate cmov if possible
    auto r = right();
    auto l = left();
    return (lft ? l : r);
  }
  Node* left() const noexcept { return left_; }
  Node* right() const noexcept {
    // Put the static assertion here so that it's evaluated after
    // Node is complete
    static_assert(alignof(Node) >= 2,
                  "Node isn't sufficiently aligned "
                  "(we exploit the least significant bit for color)");
    return reinterpret_cast<Node*>(right_color_ & ~1);
  }
  void left(Node* v) noexcept { left_ = v; }
  void right(Node* v) noexcept {
    right_color_ = (right_color_ & 1) | reinterpret_cast<uintptr_t>(v);
  }
  uint32_t color() const noexcept { return right_color_ & 1; }
  void color(uint32_t color) noexcept {
    if (__builtin_constant_p(color != 0) && (color != 0))
      right_color_ |= 1;
    else
      right_color_ = (right_color_ & ~1) | color;
  }
  void right_color_set(Node* v, uint32_t color) noexcept {
    right_color_ = reinterpret_cast<uintptr_t>(v) | color;
  }
};

template <typename Node>
class RbBase {
 public:
  using LinkPtr = RbLink<Node> Node::*;
  static constexpr uint32_t kRed = 1;
  static constexpr uint32_t kBlack = 0;

  static RbLink<Node>* link(Node* p, LinkPtr lp) noexcept { return &(p->*lp); }
  static const RbLink<Node>* link(const Node* p, LinkPtr lp) noexcept {
    return &(p->*lp);
  }

  static Node* left(const Node* p, LinkPtr lp) noexcept {
    return link(p, lp)->left();
  }
  static void left(Node* p, Node* left, LinkPtr lp) noexcept {
    link(p, lp)->left(left);
  }

  static Node* right(const Node* p, LinkPtr lp) noexcept {
    return link(p, lp)->right();
  }
  static void right(Node* p, Node* right, LinkPtr lp) noexcept {
    link(p, lp)->right(right);
  }

  static Node* left_exchange(Node* p, Node* child, LinkPtr lp) noexcept {
    auto ret = left(p, lp);
    left(p, child, lp);
    return ret;
  }
  static Node* right_exchange(Node* p, Node* child, LinkPtr lp) noexcept {
    auto ret = right(p, lp);
    right(p, child, lp);
    return ret;
  }

  static uint32_t color(const Node* p, LinkPtr lp) noexcept {
    return link(p, lp)->color();
  }
  static void color(Node* p, uint32_t clr, LinkPtr lp) noexcept {
    link(p, lp)->color(clr);
  }
  static void red_set(Node* p, LinkPtr lp) noexcept { color(p, kRed, lp); }
  static void black_set(Node* p, LinkPtr lp) noexcept { color(p, kBlack, lp); }
  static void right_color_set(Node* p, Node* right, uint32_t color,
                              LinkPtr lp) noexcept {
    link(p, lp)->right_color_set(right, color);
  }
  static bool is_red(const Node* p, LinkPtr lp) noexcept {
    return color(p, lp) == kRed;
  }
  static bool is_black(const Node* p, LinkPtr lp) noexcept {
    return color(p, lp) == kBlack;
  }
  static uint32_t color_exchange(Node* p, uint32_t clr, LinkPtr lp) noexcept {
    auto ret = color(p, lp);
    color(p, clr, lp);
    return ret;
  }

  static Node* first(const Node* root, LinkPtr lp) noexcept;
  static Node* last(const Node* root, LinkPtr lp) noexcept;

  static Node* rotate_left(Node* node, LinkPtr lp) noexcept {
    Node* r = right(node, lp);
    right(node, left_exchange(r, node, lp), lp);
    return r;
  }
  static Node* rotate_right(Node* node, LinkPtr lp) noexcept {
    Node* r = left(node, lp);
    left(node, right_exchange(r, node, lp), lp);
    return r;
  }
  static Node* lean_left(Node* node, LinkPtr lp) noexcept {
    Node* r = rotate_left(node, lp);
    color(r, color_exchange(node, kRed, lp), lp);
    return r;
  }
  static Node* lean_right(Node* node, LinkPtr lp) noexcept {
    Node* r = rotate_right(node, lp);
    color(r, color_exchange(node, kRed, lp), lp);
    return r;
  }
  static void cmpxchg_child(Node* node, Node* oldval, Node* newval,
                            LinkPtr lp) noexcept {
    if (left(node, lp) == oldval)
      left(node, newval, lp);
    else if (right(node, lp) == oldval)
      right(node, newval, lp);
  }
  static void ucmpxchg_child(Node* node, Node* oldval, Node* newval,
                             LinkPtr lp) noexcept {
    assert(left(node, lp) == oldval || right(node, lp) == oldval);
    if (left(node, lp) == oldval)
      left(node, newval, lp);
    else
      right(node, newval, lp);
  }

  static Node* move_red_left(Node* node, LinkPtr lp) noexcept;
  static Node* move_red_right(Node* node, LinkPtr lp) noexcept;
};

template <typename Node>
Node* RbBase<Node>::first(const Node* root, LinkPtr lp) noexcept {
  const Node* r = nullptr;
  for (const Node* p = root; p; p = left(p, lp)) r = p;
  return const_cast<Node*>(r);
}

template <typename Node>
Node* RbBase<Node>::last(const Node* root, LinkPtr lp) noexcept {
  const Node* r = nullptr;
  for (const Node* p = root; p; p = right(p, lp)) r = p;
  return const_cast<Node*>(r);
}

template <typename Node>
Node* RbBase<Node>::move_red_left(Node* node, LinkPtr lp) noexcept {
  red_set(left(node, lp), lp);
  if (Node* t = right(node, lp); t && left(t, lp) && is_red(left(t, lp), lp)) {
    right(node, rotate_right(t, lp), lp);
    Node* r = rotate_left(node, lp);
    if (Node* rt = right(node, lp); rt && is_red(rt, lp)) {
      black_set(rt, lp);
      red_set(node, lp);
      left(r, rotate_left(node, lp), lp);
    } else {
      black_set(node, lp);
    }
    return r;
  } else {
    red_set(node, lp);
    return rotate_left(node, lp);
  }
}

template <typename Node>
Node* RbBase<Node>::move_red_right(Node* node, LinkPtr lp) noexcept {
  Node* t = left(node, lp);
  if (is_red(t, lp)) {
    Node* u = right(t, lp);
    if (Node* v = left(u, lp); v && is_red(v, lp)) {
      color(u, color(node, lp), lp);
      black_set(v, lp);
      left(node, rotate_left(t, lp), lp);
    } else {
      color(t, color(node, lp), lp);
      red_set(u, lp);
    }
    red_set(node, lp);
  } else {
    red_set(t, lp);
    if (Node* s = left(t, lp); s && is_red(s, lp))
      black_set(s, lp);
    else
      return rotate_left(node, lp);
  }

  Node* r = rotate_right(node, lp);
  right(r, rotate_left(node, lp), lp);
  return r;
}

template <typename Accessor>
class Rb : public RbBase<typename Accessor::Node> {
 public:
  Rb(const Rb&) = delete;
  Rb& operator=(const Rb&) = delete;

  using Node = typename Accessor::Node;
  using Base = RbBase<Node>;
  using LinkPtr = typename Base::LinkPtr;
  static constexpr LinkPtr kLp = Accessor::link;

  using Base::kBlack;
  using Base::kRed;

  static RbLink<Node>* link(Node* p) noexcept { return Base::link(p, kLp); }
  static const RbLink<Node>* link(const Node* p) noexcept {
    return Base::link(p, kLp);
  }

  static Node* left(const Node* p) noexcept { return Base::left(p, kLp); }
  static void left(Node* p, Node* left) noexcept { Base::left(p, left, kLp); }

  static Node* right(const Node* p) noexcept { return Base::right(p, kLp); }
  static void right(Node* p, Node* right) noexcept {
    Base::right(p, right, kLp);
  }

  static Node* left_exchange(Node* p, Node* left) noexcept {
    return Base::left_exchange(p, left, kLp);
  }
  static Node* right_exchange(Node* p, Node* right) noexcept {
    return Base::right_exchange(p, right, kLp);
  }

  static uint32_t color(const Node* p) noexcept { return Base::color(p, kLp); }
  static void color(Node* p, uint32_t color) noexcept {
    Base::color(p, color, kLp);
  }
  static void red_set(Node* p) noexcept { Base::red_set(p, kLp); }
  static void black_set(Node* p) noexcept { Base::black_set(p, kLp); }
  static void right_color_set(Node* p, Node* right, uint32_t color) noexcept {
    Base::right_color_set(p, right, color, kLp);
  }
  static bool is_red(const Node* p) noexcept { return Base::is_red(p, kLp); }
  static bool is_black(const Node* p) noexcept {
    return Base::is_black(p, kLp);
  }
  static uint32_t color_exchange(Node* p, uint32_t color) noexcept {
    return Base::color_exchange(p, color, kLp);
  }

  static Node* first(const Node* root) noexcept {
    return Base::first(root, kLp);
  }
  static Node* last(const Node* root) noexcept { return Base::last(root, kLp); }
  static Node* next(const Node* root, const Node* node) noexcept;
  static Node* prev(const Node* root, const Node* node) noexcept;

  Node* first() const noexcept { return first(root_); }
  Node* last() const noexcept { return last(root_); }
  Node* next(const Node* node) const noexcept { return next(root_, node); }
  Node* prev(const Node* node) const noexcept { return prev(root_, node); }

  // If Unsafe, the caller must ensure that value exists
  template <bool Unsafe, typename Key>
  static Node* search(const Node* root, Key key) noexcept;
  template <bool NSearch, typename Key>
  static Node* npsearch(const Node* root, Key key) noexcept;

  // Search for a specified key, returns the pointer or nullptr
  template <typename Key>
  Node* search(Key key) const noexcept {
    return search<false>(root_, key);
  }
  // Ditto, but caller's responsibility to guarantee the key exists;
  // otheriwse it will segfault.
  template <typename Key>
  Node* usearch(Key key) const noexcept {
    return search<true>(root_, key);
  }
  // nsearch and psearch:
  // Similar with search, but if the specified key doesn't exist,
  // psearch returns the immediately smaller node the nsearch returns the
  // immediately large.
  // NOTE THAT THEY ARE NOT COUNTERPARTS OF STL'S lower_bound/upper_bound
  template <typename Key>
  Node* nsearch(Key key) const noexcept {
    return npsearch<true>(root_, key);
  }
  template <typename Key>
  Node* psearch(Key key) const noexcept {
    return npsearch<false>(root_, key);
  }

  static Node* rotate_left(Node* node) noexcept {
    return Base::rotate_left(node, kLp);
  }
  static Node* rotate_right(Node* node) noexcept {
    return Base::rotate_right(node, kLp);
  }
  static Node* lean_left(Node* node) noexcept {
    return Base::lean_left(node, kLp);
  }
  static Node* lean_right(Node* node) noexcept {
    return Base::lean_right(node, kLp);
  }
  static void cmpxchg_child(Node* node, Node* oldval, Node* newval) noexcept {
    Base::cmpxchg_child(node, oldval, newval, kLp);
  }
  static void ucmpxchg_child(Node* node, Node* oldval, Node* newval) noexcept {
    Base::ucmpxchg_child(node, oldval, newval, kLp);
  }

  static Node* move_red_left(Node* node) noexcept {
    return Base::move_red_left(node, kLp);
  }
  static Node* move_red_right(Node* node) noexcept {
    return Base::move_red_right(node, kLp);
  }
  Node* insert(Node* node) noexcept;
  Node* remove(Node* node) noexcept;

 public:
  constexpr Rb() noexcept = default;

 private:
  Node* root_ = nullptr;
};

template <typename Accessor>
auto Rb<Accessor>::next(const Node* root, const Node* p) noexcept -> Node* {
  if (const Node* r = right(p)) {
    return first(r);
  } else {
    const Node* ret = nullptr;
    for (const Node* node = root; node != p;) {
      assert(node != nullptr);
      if (Accessor::lt(p, node)) {
        ret = node;
        node = left(node);
      } else {
        node = right(node);
      }
    }
    return const_cast<Node*>(ret);
  }
}

template <typename Accessor>
auto Rb<Accessor>::prev(const Node* root, const Node* p) noexcept -> Node* {
  if (const Node* l = left(p)) {
    return last(l);
  } else {
    const Node* r = nullptr;
    for (const Node* node = root; node != p;) {
      assert(node != nullptr);
      if (Accessor::lt(p, node)) {
        node = left(node);
      } else {
        r = node;
        node = right(node);
      }
    }
    return const_cast<Node*>(r);
  }
}

template <typename Accessor>
template <bool Unsafe, typename Key>
auto Rb<Accessor>::search(const Node* root, Key key) noexcept -> Node* {
  const Node* node = root;
  while (Unsafe || node) {
    int cmp = Accessor::cmp(key, node);
    if (cmp == 0)
      break;
    else
      node = link(node)->get(cmp < 0);
  }
  return const_cast<Node*>(node);
}

template <typename Accessor>
template <bool NSearch, typename Key>
auto Rb<Accessor>::npsearch(const Node* root, Key key) noexcept -> Node* {
  const Node* node = root;
  const Node* r = nullptr;
  while (node) {
    int cmp = Accessor::cmp(key, node);
    if (cmp < 0) {
      if constexpr (NSearch) r = node;
      node = left(node);
    } else if (cmp > 0) {
      if constexpr (!NSearch) r = node;
      node = right(node);
    } else {
      r = node;
      break;
    }
  }
  return const_cast<Node*>(r);
}

template <typename Accessor>
auto Rb<Accessor>::insert(Node* node) noexcept -> Node* {
  left(node, nullptr);
  right_color_set(node, nullptr, kRed);
  if (!root_) {
    right_color_set(node, nullptr, kBlack);
    root_ = node;
    return node;
  }

  Node s;
  left(&s, root_);
  right_color_set(&s, nullptr, kBlack);

  Node* g = nullptr;
  Node* p = &s;
  Node* c = root_;

  /* Iteratively search down the tree for the insertion point,      */
  /* splitting 4-nodes as they are encountered.  At the end of each */
  /* iteration, g->p->c is a 3-level path down    */
  /* the tree, assuming a sufficiently deep tree.                   */
  bool lt = true;
  do {
    if (Node* t = left(c); t && is_red(t) && left(t) && is_red(left(t))) {
      /* c is the top of a logical 4-node, so split it.   */
      /* This iteration does not move down the tree, due to the */
      /* disruptiveness of node splitting.                      */
      /*                                                        */
      /* Rotate right.                                          */
      Node* T = rotate_right(c);
      /* Pass red links up one level.                           */
      black_set(left(T));
      if (left(p) == c) {
        left(p, T);
        c = T;
      } else {
        /* c was the right child of p, so rotate  */
        /* left in order to maintain the left-leaning         */
        /* invariant.                                         */
        assert(right(p) == c);
        right(p, T);
        Node* uu = lean_left(p);
        ucmpxchg_child(g, p, uu);
        p = uu;
        c = link(p)->get(Accessor::lt(node, p));
        continue;
      }
    }
    g = p;
    p = c;
    lt = Accessor::lt(node, c);
    c = link(c)->get(lt);
  } while (c);
  /* p now refers to the node under which to insert.          */
  if (lt) {
    left(p, node);
  } else {
    right(p, node);
    cmpxchg_child(g, p, lean_left(p));
  }
  /* Update the root and make sure that it is black.                */
  root_ = left(&s);
  black_set(root_);

  return node;
}

template <typename Accessor>
auto Rb<Accessor>::remove(Node* node) noexcept -> Node* {
  Node s;
  left(&s, root_);
  right_color_set(&s, nullptr, kBlack);
  Node* p = &s;
  Node* c = root_;
  Node* xp = nullptr;
  /* Iterate down the tree, but always transform 2-nodes to 3- or   */
  /* 4-nodes in order to maintain the invariant that the current    */
  /* node is not a 2-node.  This allows simple deletion once a leaf */
  /* is reached.  Handle the root specially though, since there may */
  /* be no way to convert it from a 2-node to a 3-node.             */
  int cmp = (node == c) ? 0 : Accessor::lt(node, c) ? -1 : 1;
  if (cmp < 0) {
    /* node is within the left child */
    if (Node* t = left(c); is_red(t) || (left(t) && is_red(left(t)))) {
      /* Move left.                                             */
      p = c;
      c = left(c);
    } else {
      /* Apply standard transform to prepare for left move.     */
      c = move_red_left(c);
      black_set(c);
      left(p, c);
    }
  } else {
    if (node == c) {
      if (right(c)) {
        /* This is the node we want to delete, but we will    */
        /* instead swap it with its successor and delete the  */
        /* successor.  Record enough information to do the    */
        /* swap later.  xp is the node's parent.      */
        xp = p;
        cmp = 1; /* Note that deletion is incomplete.   */
      } else {
        /* Delete root node (which is also a leaf node).      */
        Node* t = nullptr;
        if (left(c)) {
          t = lean_right(c);
          right(t, nullptr);
        }
        left(p, t);
      }
    }
    if (cmp > 0) {
      if (Node* cr = right(c); cr && left(cr) && is_red(left(cr))) {
        // Move right.
        p = c;
        c = cr;
      } else {
        Node* t = left(c);
        if (is_red(t)) {
          /* Standard transform.                            */
          t = move_red_right(c);
        } else {
          /* Root-specific transform.                       */
          red_set(c);
          if (Node* u = left(t); u && is_red(u)) {
            black_set(u);
            t = rotate_right(c);
            right(t, rotate_left(c));
          } else {
            red_set(t);
            t = rotate_left(c);
          }
        }
        left(p, t);
        c = t;
      }
    }
  }
  if (cmp != 0) {
    for (;;) {
      assert(p != nullptr);
      if ((node != c) && Accessor::lt(node, c)) {
        Node* t = left(c);
        if (t == nullptr) {
          /* c now refers to the successor node to    */
          /* relocate, and xp/node refer to the     */
          /* context for the relocation.                    */
          *link(c) = *link(node);
          ucmpxchg_child(xp, node, c);
          ucmpxchg_child(p, c, nullptr);
          break;
        }
        if (is_black(t) && (!left(t) || is_black(left(t)))) {
          Node* rt = move_red_left(c);
          ucmpxchg_child(p, c, rt);
          c = rt;
        } else {
          p = c;
          c = left(c);
        }
      } else {
        /* Check whether to delete this node (it has to be    */
        /* the correct node and a leaf node).                 */
        if (node == c) {
          if (right(c)) {
            /* This is the node we want to delete, but we */
            /* will instead swap it with its successor    */
            /* and delete the successor.  Record enough   */
            /* information to do the swap later.          */
            /* xp is node's parent.               */
            xp = p;
          } else {
            /* Delete leaf node.                          */
            Node* t = nullptr;
            if (left(c)) {
              t = lean_right(c);
              right(t, nullptr);
            }
            ucmpxchg_child(p, c, t);
            break;
          }
        }
        if (Node* t = right(c); t && left(t) && is_red(left(t))) {
          p = c;
          c = right(c);
        } else {
          Node* rt = move_red_right(c);
          ucmpxchg_child(p, c, rt);
          c = rt;
        }
      }
    }
  }
  // Update root
  root_ = left(&s);

  return node;
}

}  // namespace alloc
}  // namespace cbu
