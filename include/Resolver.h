#ifndef __RESOLVER_H__
#define __RESOLVER_H__

#include <queue>

#include "TaskNode.h"
#include "ThreadPool.h"
#include "ThreadSafeQueue.h"

using BaseNodePtr = std::shared_ptr<SimpleTask>;

class NodeResolver {
private:
  std::unordered_map<BaseNodePtr, std::unordered_set<BaseNodePtr>> graph;
  std::unordered_set<BaseNodePtr> nodes;
  std::queue<BaseNodePtr> roots;

  ThreadSafeQueue<BaseNodePtr> process_queue;

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
    process_queue.wait_and_pop(node);
    (*node)();
    for (auto &child_func_node : graph[node]) {
      child_func_node->update_prerequisites_status(node);
      bool valid = child_func_node->check_prerequisites();
      if (valid) {
        process_queue.push(child_func_node);
        thread_pool.add_task([this] { this->resolve_task(); });
      }
    }
  }

public:
  NodeResolver(
      unsigned int max_num_thread = std::thread::hardware_concurrency() - 1)
      : thread_pool(max_num_thread) {}

  void add(BaseNodePtr &node) { nodes.insert(node); }
  void add(SimpleTask *node) {
    nodes.insert(std::shared_ptr<SimpleTask>(node));
  }

  void resolve() {
    build_graph();
    std::queue<BaseNodePtr> next = build_roots();
    while (!next.empty()) {
      process_queue.push(next.front());
      thread_pool.add_task([this] { this->resolve_task(); });
      next.pop();
    }
  }
};

#endif