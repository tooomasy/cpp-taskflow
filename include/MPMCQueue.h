#ifndef MPMCQUEUE_H_
#define MPMCQUEUE_H_

#include <thread>
#include <unordered_set>
#include <vector>

#include "SPSCQueue.h"

template <typename T>
class MPMCQueue {
 private:
  // struct record {
  //   unsigned max_threads_ = 3;
  //   std::unordered_set<std::thread::id> thread_in_use_;

  //   record(unsigned max_threads) : max_threads_(max_threads) {}

  //   bool is_available_for_new_thread(std::thread::id cosnt& id) {
  //     if (thread_in_use_.find(id) == thread_in_use_.end()) {
  //       return true;
  //     }

  //     if (thread_in_use_.size() == max_threads_) {
  //       return false;
  //     }
  //     thread_in_use_.insert(id);
  //     return true;
  //   }

  //   void remove_thread_from_in_use(std::thread::id const& id) {
  //     thread_in_use_.erase(id);
  //   }
  // };

  // record push_action;
  // record pop_action;
  unsigned max_threads_;
  std::vector<SPSCQueue<T>> queues_;

 public:
  MPMCQueue(unsigned max_threads)
      // : push_action(max_threads),
      //   pop_action(max_threads),
      : max_threads_(max_threads), queues_(max_threads) {}

  void push(T val) {}

  bool pop(T& val) {}

  bool try_push(T val) {
    for (int i = 0; i < max_threads_; ++i) {
      if (queues_[i].try_push(std::move(val))) {
        return true;
      }
    }
    return false;
  }

  bool try_pop(T& val) {
    for (int i = 0; i < max_threads_; ++i) {
      if (queues_[i].try_pop(val)) {
        return true;
      }
    }
    return false;
  }
};

#endif  // MPMCQUEUE_H_