#include <iostream>

#include "Resolver.h"
#include "TaskNode.h"

/**
 * TODO:
 * - memory leak
 * - solve the raw & smart pointer issue
 * - revise copy
 * - rewrite ThreadSafeQueue;
 * - loop
 * - conditions
 * - supports lambda
 * - supports class function
 */

void hello() { std::cout << "Hello world\n"; }

int init() { return 100; }
int add1(int x) { return x + 10; }
int add2(int x) { return x + 20; }
int add3(int x, int y) { return x + y; }

int main() {
  std::shared_ptr<SimpleTask> base = std::make_shared<TaskNode<int>>(&init);
  std::shared_ptr<SimpleTask> node =
      std::make_shared<TaskNode<int>>(&add1, base);
  std::shared_ptr<SimpleTask> node2 =
      std::make_shared<TaskNode<int>>(&add2, base);
  std::shared_ptr<SimpleTask> node3 =
      std::make_shared<TaskNode<int>>(&add3, node, node2);
  std::shared_ptr<SimpleTask> extra = std::make_shared<TaskNode<void>>(&hello);

  extra->depends_on(node3);

  NodeResolver resolver;
  resolver.add(base);
  resolver.add(node);
  resolver.add(node2);
  resolver.add(node3);
  resolver.add(extra);

  resolver.resolve_sync();
  std::printf("result: %d\n", *((int *)node3->get_result()));

  return 0;
}