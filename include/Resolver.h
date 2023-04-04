#ifndef __RESOLVER_H__
#define __RESOLVER_H__

#include <condition_variable>
#include <queue>

#include "LockFreeStack.h"
#include "MPMCQueue.h"
#include "TaskNode.h"
#include "ThreadPool.h"
#include "ThreadSafeQueue.h"

/**
 * TODO:
 *  - add a version that use lock-free queue
 */

using BaseNodePtr = std::shared_ptr<SimpleTask>;

template <typename TaskQueue = MPMCQueue<BaseNodePtr>,
          typename ThreadPool = ThreadPool>
class NodeResolver {
private:
  std::unordered_map<BaseNodePtr, std::unordered_set<BaseNodePtr>> graph;
  std::unordered_set<BaseNodePtr> nodes;
  std::queue<BaseNodePtr> roots;

  std::unordered_set<BaseNodePtr> end_nodes;
  int end_nodes_count = 0;
  std::mutex end_nodes_mutex;
  std::condition_variable end_nodes_cv;

  TaskQueue process_queue;
  ThreadPool thread_pool;

  void build_graph() {
    std::unordered_map<BaseNodePtr, std::unordered_set<BaseNodePtr>> new_graph;
    for (auto &node : nodes) {
      for (auto &depended_node : node->get_prerequisites()) {
        if (new_graph.find(depended_node) == new_graph.end()) {
          new_graph[depended_node] = std::unordered_set<BaseNodePtr>();
        }
        new_graph[depended_node].insert(node);
      }
      if (new_graph.find(node) == new_graph.end()) {
        new_graph[node] = std::unordered_set<BaseNodePtr>();
      }
    }
    graph = std::move(new_graph);
  }

  std::queue<BaseNodePtr> build_roots() {
    std::queue<BaseNodePtr> res;
    for (auto &node : nodes) {
      if (node->get_prerequisites().size() == 0) {
        res.push(node);
      }
    }
    return res;
  }

  void resolve_task() {
    BaseNodePtr node;
    process_queue.pop(node);

    (*node)(); // execute the node task

    if (end_nodes.find(node) != end_nodes.end()) {
      std::unique_lock lk(end_nodes_mutex);
      --end_nodes_count;
      end_nodes_cv.notify_all();
    }

    for (auto &child_func_node : graph[node]) {
      child_func_node->update_prerequisites_status(node);
      bool valid = child_func_node->check_prerequisites();
      if (valid) {
        process_queue.push(child_func_node);
        thread_pool.add_task([this] { this->resolve_task(); });
      }
    }
  }

  int find_number_of_end_nodes(
      std::unordered_map<BaseNodePtr, std::unordered_set<BaseNodePtr>> &graph) {
    for (auto &[node, node_list] : graph) {
      if (node_list.size() == 0) {
        end_nodes.insert(node);
      }
    }
    return end_nodes.size();
  }

public:
  NodeResolver(
      unsigned int max_num_thread = std::thread::hardware_concurrency() - 1)
      : thread_pool(max_num_thread) {}

  void add(BaseNodePtr &node) { nodes.insert(node); }
  void add(SimpleTask *node) {
    nodes.insert(std::shared_ptr<SimpleTask>(node));
  }

  void resolve_async() {
    build_graph();
    std::queue<BaseNodePtr> next = build_roots();
    end_nodes_count = find_number_of_end_nodes(graph);
    while (!next.empty()) {
      process_queue.push(next.front());
      thread_pool.add_task([this] { this->resolve_task(); });
      next.pop();
    }
  }

  void join() {
    std::unique_lock lk(end_nodes_mutex);
    end_nodes_cv.wait(lk, [&] { return end_nodes_count == 0; });
  }

  void resolve_sync() {
    resolve_async();
    join();
  }
};

#endif