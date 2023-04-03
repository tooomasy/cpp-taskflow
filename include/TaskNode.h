#ifndef __TASKNODE_H__
#define __TASKNODE_H__

#include <functional>
#include <future>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "CondNode.h"
#include "MemoryPool.h"

struct SimpleTask;

template <typename U, typename T> auto &&retrieve_value(T &&value) {
  return std::forward<T>(value);
}

template <typename U, typename T> auto retrieve_value(T *value) {
  return *((U *)value->get_result());
}

template <typename U, typename T>
auto retrieve_value(std::shared_ptr<T> value) {
  return *((U *)value->get_result());
}

template <typename Tuple, size_t N, typename... Tail>
struct NodeValueExtractor {
  static std::tuple<> extract(const Tuple &t) { return {}; }
};

template <typename Tuple, size_t N, typename Head, typename... Tail>
struct NodeValueExtractor<Tuple, N, Head, Tail...> {
  static auto extract(const Tuple &t) {
    auto tmp_tuple = NodeValueExtractor<Tuple, N - 1, Tail...>::extract(t);
    return std::tuple_cat(
        tmp_tuple, std::make_tuple(retrieve_value<Head>(std::get<N - 1>(t))));
  }
};

// TODO: handle case for reference
template <typename Tuple, typename Head>
struct NodeValueExtractor<Tuple, 1, Head> {
  static std::tuple<Head> extract(const Tuple &t) {
    return {retrieve_value<Head>(std::get<0>(t))};
  }
};

struct SimpleTask {
  using BaseNodePtr = std::shared_ptr<SimpleTask>;

  virtual void operator()() = 0;
  virtual void *get_result() = 0;
  virtual std::unordered_set<BaseNodePtr> &get_prerequisites() = 0;
  virtual bool check_prerequisites() = 0;
  virtual void update_prerequisites_status(BaseNodePtr &) = 0;
  virtual void depends_on(BaseNodePtr) = 0;
  virtual void depends_on(SimpleTask *) = 0;

  // virtual bool is_valid_branch_checking() = 0;
};

template <typename ReturnType>
class TaskNode : public SimpleTask,
                 public MemoryPoolMixin<TaskNode<ReturnType>> {
private:
  struct Metadata {
    bool is_ready = false;
  };

  using BaseNodePtr = std::shared_ptr<SimpleTask>;
  using MetadataPtr = std::shared_ptr<Metadata>;

  std::function<void()> func_wrapper;
  std::unordered_set<BaseNodePtr> prerequisites_nodes;
  std::unordered_map<BaseNodePtr, MetadataPtr> prerequisites_info;

  std::shared_ptr<ReturnType> func_result = nullptr;
  std::promise<std::shared_ptr<ReturnType>> func_promise;
  std::shared_future<std::shared_ptr<ReturnType>> result_future =
      func_promise.get_future();

  template <typename... Rest> void add_strickly_depended_node() {}

  template <typename First, typename... Rest>
  void add_strickly_depended_node(First first, Rest... rest) {
    if constexpr (std::is_convertible_v<First, SimpleTask *>) {
      this->depends_on(std::shared_ptr<SimpleTask>(first));
    } else if constexpr (std::is_convertible_v<First, BaseNodePtr>) {
      this->depends_on(first);
    }
    this->add_strickly_depended_node(rest...);
  }

public:
  using return_type = ReturnType;

  template <typename... FArgs, typename... Args>
  TaskNode(ReturnType (*func)(FArgs...), Args &&...args)
      : func_wrapper([&, this, func]() mutable {
          auto args_tuple = std::make_tuple(std::forward<Args>(args)...);
          if constexpr (std::is_void_v<ReturnType>) {
            std::apply(func,
                       NodeValueExtractor<decltype(args_tuple), sizeof...(Args),
                                          FArgs...>::extract(args_tuple));
            func_promise.set_value(func_result);
          } else {
            func_result = std::make_shared<ReturnType>(std::apply(
                func, NodeValueExtractor<decltype(args_tuple), sizeof...(Args),
                                         FArgs...>::extract(args_tuple)));
            func_promise.set_value(func_result);
          }
        }) {
    add_strickly_depended_node(args...);
  }

  void operator()() override { func_wrapper(); }

  void *get_result() override {
    return static_cast<void *>(result_future.get().get());
  }

  void depends_on(BaseNodePtr node) override {
    prerequisites_nodes.insert(node);
    prerequisites_info[node] = std::make_shared<Metadata>();
  }

  void depends_on(SimpleTask *node) override {
    BaseNodePtr node_ptr = std::shared_ptr<SimpleTask>(node);
    prerequisites_nodes.insert(node_ptr);
    prerequisites_info[node_ptr] = std::make_shared<Metadata>();
  }

  std::unordered_set<BaseNodePtr> &get_prerequisites() override {
    return prerequisites_nodes;
  }

  bool check_prerequisites() override {
    for (auto &pair : prerequisites_info) {
      if (pair.second->is_ready == false)
        return false;
    }
    return true;
  }

  void update_prerequisites_status(BaseNodePtr &node) override {
    prerequisites_info[node]->is_ready = true;
  }
};

#endif // __TASKNODE_H__