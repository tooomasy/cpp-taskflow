# Cpp-Taskflow

keywords: `c++`, `multi-threading`, `concurrency`, `lock-free queue`, `lock-free stack`, `hazard pointers`, `memory pool`, `DAG`, `template`

Cpp-Taskflow is a C++ library for managing and scheduling tasks that may be dependent on one another, represented as a DAG (directed acyclic graph). The library provides a simple and efficient way to express complex task dependencies and execute them in a highly concurrent and parallel manner.

## Features

- DAG-based task scheduling: Cpp-Taskflow provides a flexible and expressive way to define complex task dependencies using a DAG graph. Tasks can be added to the graph, with their dependencies specified as edges between nodes.

- Concurrency and multithreading: Cpp-Taskflow is designed to make the most of modern hardware, with built-in support for concurrency and multithreading. Tasks can be executed concurrently, with the library automatically managing the scheduling and synchronization of the tasks.

- Lock-free algorithms: Cpp-Taskflow is built using lock-free algorithms and techniques, which help to ensure that tasks can be executed quickly and efficiently without the overhead of locks and other synchronization primitives.

- Memory pool: Cpp-Taskflow uses a memory pool to efficiently manage memory usage, reducing the overhead of dynamic memory allocation and ensuring that memory is used efficiently.

- Easy to use: Cpp-Taskflow is designed to be easy to use, with a simple and intuitive API that does not require users to break their existing code. The library can be seamlessly integrated into existing C++ projects, making it easy to get started with task scheduling and management.

## Disclaimer

Cpp-Taskflow is still under development, and the API and features are subject to change without notice. The library is currently experimental and is primarily intended to provide a proof-of-concept for managing and scheduling tasks using a DAG graph.

While the library is functional and has been tested, it may not be suitable for production use. Use at your own risk.

## Examples

Here's a simple example of how to use Cpp-Taskflow to execute a set of tasks:

```c++
void hello() { std::cout << "Hello world\n"; }

int init() { return 100; }
int add1(int x) { return x + 10; }
int add2(int x) { return x + 20; }
int add3(int x, int y) { return x + y; }

int main() {
  std::shared_ptr<SimpleTask> base = std::make_shared<TaskNode<int>>(&init);
  std::shared_ptr<SimpleTask> node = std::make_shared<TaskNode<int>>(&add1, base);
  std::shared_ptr<SimpleTask> node2 = std::make_shared<TaskNode<int>>(&add2, base);
  std::shared_ptr<SimpleTask> node3 = std::make_shared<TaskNode<int>>(&add3, node, node2);
  std::shared_ptr<SimpleTask> extra = std::make_shared<TaskNode<void>>(&hello);

  extra->depends_on(node3);

  NodeResolver resolver;
  resolver.add(base);
  resolver.add(node);
  resolver.add(node2);
  resolver.add(node3);
  resolver.add(extra);

  resolver.resolve();
  std::printf("result: %d\n", *((int*)node3->get_result())); // output: 230

  return 0;
}
```

## Contributing
Cpp-Taskflow is an open-source project, and contributions are always welcome! If you have any ideas for new features, improvements, or bug fixes, please don't hesitate to submit a pull request or open an issue on the official GitHub repository.