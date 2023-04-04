#ifndef SPSCQUEUE_H_
#define SPSCQUEUE_H_

#include <atomic>

/**
 * TODO:
 * - benchmark
 * - revamp
 */

template <typename T> class SPSCQueue {
private:
  struct Node {
    T val_;
    Node *next_;

    Node() : next_{nullptr} {}
    Node(T val) : val_(std::move(val)), next_{nullptr} {}
  };

  std::atomic<Node *> head_ = {new Node()};
  std::atomic<Node *> tail_ = head_.load();

  std::atomic<bool> push_in_use_ = {false};
  std::atomic<bool> pop_in_use_ = {false};

  Node *pop_head() {
    Node *old_head = head_.load();
    if (old_head == tail_.load()) {
      return nullptr;
    }
    head_.store(old_head->next_);
    return old_head;
  }

public:
  SPSCQueue() {}
  ~SPSCQueue() {}

  void push(T val) {
    while (!try_push(std::move(val)))
      ;
  }

  void pop(T &val) {
    while (!try_pop(val))
      ;
  }

  bool try_push(T val) {
    bool expect = false;
    if (!push_in_use_.compare_exchange_strong(expect, true))
      return false;
    // push_in_use_.store(true);
    Node *new_node = new Node();
    Node *old_tail = tail_.load();
    old_tail->val_ = val;
    old_tail->next_ = new_node;
    tail_.store(new_node);
    push_in_use_.store(false);
    return true;
  }

  bool try_pop(T &val) {
    bool expect = false;
    if (!pop_in_use_.compare_exchange_strong(expect, true))
      return false;
    Node *old_head = pop_head();
    if (old_head == nullptr) {
      pop_in_use_.store(false);
      return false;
    }
    val = std::move(old_head->val_);
    delete old_head;
    pop_in_use_.store(false);
    return true;
  }
};

#endif // SPSCQUEUE_H_