#ifndef LOCKFREESTACK_H_
#define LOCKFREESTACK_H_

#include "Hazptr.h"

template <typename T> struct LockFreeStack {
  struct Node : hazptr_obj_base<Node> {
    T val_;
    Node *next_;

    Node(T val) : val_(std::move(val)), next_(nullptr) {}
  };

  std::atomic<Node *> head;

  void push(T val) {
    Node *new_node = new Node(std::move(val));
    while (!head.compare_exchange_weak(new_node->next_, new_node))
      ;
  }

  bool pop(T &val) {
    Node *node;
    hazptr_holder h;
    while (true) {
      node = head.load();
      h.protect(node);

      if (node == nullptr) {
        return false;
      }

      if (head.compare_exchange_weak(node, node->next_))
        break;
    }
    h.release_protection();
    val = node->val_;
    node->retire();
    return true;
  }
};

#endif // LOCKFREESTACK_H_