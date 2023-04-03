#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include "ThreadSafeQueue.h"

class ThreadPool {
 private:
  std::queue<std::function<void()>> task_queue;
  std::vector<std::thread> threads;

  std::mutex m;
  std::condition_variable cv;
  bool stop = false;

 public:
  ThreadPool(unsigned int num_thread);
  ~ThreadPool();

  template <typename T>
  void add_task(T task);
  void terminate();
};

ThreadPool::ThreadPool(unsigned int num_thread) {
  for (int i = 0; i < num_thread; ++i) {
    threads.push_back(std::thread([this]() {
      while (true) {
        std::function<void()> task;
        {
          std::unique_lock lk(m);
          cv.wait(lk, [this]() { return stop || !task_queue.empty(); });
          if (stop) return;
          task = std::move(task_queue.front());
          task_queue.pop();
        }
        task();
      }
    }));
  }
}

ThreadPool::~ThreadPool() {
  terminate();
  for (auto& thread : threads) {
    thread.join();
  }
}

template <typename T>
void ThreadPool::add_task(T task) {
  {
    std::unique_lock lk(m);
    task_queue.push([=] { task(); });
  }
  cv.notify_one();
}

void ThreadPool::terminate() {
  {
    std::unique_lock<std::mutex> lk(m);
    stop = true;
  }
  cv.notify_all();
}

#endif