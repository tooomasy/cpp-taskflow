#ifndef __THREAD_SAFE_QUEUE_H__
#define __THREAD_SAFE_QUEUE_H__

#include <condition_variable>
#include <memory>
#include <mutex>

/**
 * TODO:
 * - benchmark
 */

template <typename T> class ThreadSafeQueue {
private:
  struct Node {
    std::shared_ptr<T> data;
    std::unique_ptr<Node> next;
  };

  mutable std::mutex head_mutex_;
  std::unique_ptr<Node> head_;
  mutable std::mutex tail_mutex_;
  Node *tail_;
  mutable std::condition_variable cond_;

private:
  Node *get_tail() {
    std::lock_guard<std::mutex> lock(tail_mutex_);
    return tail_;
  }

  std::unique_ptr<Node> pop_head() {
    std::unique_ptr<Node> old_head = std::move(head_);
    head_ = std::move(old_head->next);
    return old_head;
  }

  std::unique_lock<std::mutex> wait_for_data() {
    std::unique_lock<std::mutex> lock(head_mutex_);
    cond_.wait(lock, [this] { return head_.get() != get_tail(); });
    return std::move(lock);
  }

  std::unique_ptr<Node> try_pop_head() {
    std::lock_guard<std::mutex> lock(head_mutex_);
    if (head_.get() == get_tail()) {
      return std::unique_ptr<Node>();
    }
    return pop_head();
  }

  std::unique_ptr<Node> try_pop_head(T &value) {
    std::lock_guard<std::mutex> lock(head_mutex_);
    if (head_.get() == get_tail()) {
      return std::unique_ptr<Node>();
    }
    value = std::move(*head_->data);
    return pop_head();
  }

public:
  ThreadSafeQueue() : head_(new Node), tail_(head_.get()) {}

  ThreadSafeQueue(const ThreadSafeQueue &) = delete;
  ThreadSafeQueue &operator=(const ThreadSafeQueue &) = delete;

  void push(T value) {
    std::shared_ptr<T> new_data(std::make_shared<T>(std::move(value)));
    std::unique_ptr<Node> new_node(new Node);
    {
      std::lock_guard<std::mutex> lock(tail_mutex_);
      tail_->data = new_data;
      tail_->next = std::move(new_node);
      tail_ = tail_->next.get();
    }
    cond_.notify_one();
  }

  std::shared_ptr<T> try_pop() {
    std::unique_ptr<Node> old_head = try_pop_head();
    return (old_head ? old_head->data : std::shared_ptr<T>());
  }

  bool try_pop(T &value) {
    std::unique_ptr<Node> old_head = try_pop_head(value);
    return (old_head != nullptr);
  }

  void wait_and_pop(T &value) {
    std::unique_lock<std::mutex> lock(wait_for_data());
    value = std::move(*head_->data);
    pop_head();
  }

  bool empty() const {
    std::lock_guard<std::mutex> lock(head_mutex_);
    return (head_.get() == get_tail());
  }
};

#endif