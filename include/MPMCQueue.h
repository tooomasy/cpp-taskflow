#ifndef MPMCQUEUE_H_
#define MPMCQUEUE_H_

#include <thread>
#include <unordered_set>
#include <vector>

#include "SPSCQueue.h"

/**
 * TODO:
 * - benchmark
 * - revamp
 */

template <typename T> class MPMCQueue {
private:
  unsigned max_threads_;
  std::vector<SPSCQueue<T>> queues_;

public:
  MPMCQueue(unsigned max_threads = std::thread::hardware_concurrency())
      : max_threads_(max_threads), queues_(max_threads) {}

  void push(T val) {
    while (!try_push(std::move(val)))
      ;
  }

  void pop(T &val) {
    while (!try_pop(val))
      ;
  }

  bool try_push(T val) {
    for (int i = 0; i < max_threads_; ++i) {
      if (queues_[i].try_push(std::move(val))) {
        return true;
      }
    }
    return false;
  }

  bool try_pop(T &val) {
    for (int i = 0; i < max_threads_; ++i) {
      if (queues_[i].try_pop(val)) {
        return true;
      }
    }
    return false;
  }
};

#endif // MPMCQUEUE_H_