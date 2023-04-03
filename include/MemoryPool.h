#ifndef __MEMORYPOOL_H__
#define __MEMORYPOOL_H__

#include <cstdlib>
#include <memory>
#include <vector>

constexpr int NUM_OF_CHUNKS = 8;
constexpr int GROW_SIZE = 1000;
constexpr int MIN_BLOCK_SIZE_SHIFT = 3;
constexpr int MIN_BLOCK_SIZE = 1 << MIN_BLOCK_SIZE_SHIFT;
class MemoryPool {
  union Node {
    Node *next;
    void *memory;
  };

  struct Header {
    int block_size;
    int num_block;
    Node *head_node;
    std::vector<Node *> original_head;
  };

  Header chunks[NUM_OF_CHUNKS];

public:
  MemoryPool() {
    for (int i = 0; i < NUM_OF_CHUNKS; ++i) {
      int block_size = MIN_BLOCK_SIZE << i;
      int num_block = GROW_SIZE;
      Node *node = grow(block_size, num_block);
      chunks[i] = {block_size, num_block, node, {node}};
    }
  }

  ~MemoryPool() {
    for (auto &header : chunks) {
      for (auto &node_ptr : header.original_head) {
        delete[] (char *)node_ptr;
      }
    }
  }

  Node *grow(int block_size, int num_block) {
    char *memory = new char[block_size * num_block];
    char *next_pos = memory;
    for (int i = 0; i < num_block - 1; ++i) {
      Node *node_block = new (next_pos) Node();
      next_pos += block_size;
      node_block->next = (Node *)next_pos;
    }
    return (Node *)memory;
  }

  template <typename T> void *allocate(unsigned size) {
    const int type_size = sizeof(T);
    if (type_size < MIN_BLOCK_SIZE) {
      return syscall_allocate(size);
    }
    return allocate_pool(type_size, size);
  }

  inline void *syscall_allocate(unsigned size) { return malloc(size); }

  inline void *allocate_pool(int type_size, unsigned size) {
    const int idx = get_index(size);
    Node *return_ptr = chunks[idx].head_node;
    if (return_ptr == nullptr) {
      Node *new_chunk = grow(type_size, 100);
      chunks[idx].head_node = new_chunk;
      chunks[idx].original_head.push_back(new_chunk);
      return_ptr = chunks[idx].head_node;
    }
    chunks[idx].head_node = return_ptr->next;
    return (void *)return_ptr;
  }

  template <typename T> void deallocate(T *ptr, unsigned size) {
    deallocate_without_destructor(ptr, size);
    ptr->~T();
  }

  template <typename T>
  void deallocate_without_destructor(T *ptr, unsigned size) {
    Node *node_ptr = (Node *)ptr;
    const int type_size = sizeof(T);
    const int idx = get_index(size);
    node_ptr->next = chunks[idx].head_node;
    chunks->head_node = node_ptr;
  }

  // TODO: study how its work
  int get_index(int n) {
    if (n == 0) {
      return 1; // special case for 0
    }

    n--; // subtract 1 to handle cases where n is already a power of 2
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++; // add 1 to get the next power of 2

    int count = 0;
    while (n) {
      n >>= 1;
      count++;
    }
    return count - MIN_BLOCK_SIZE_SHIFT - 1;
  }
};

template <typename Derived> class MemoryPoolMixin {
private:
  static inline std::shared_ptr<MemoryPool> mem_pool =
      std::make_shared<MemoryPool>();

  void *allocate(unsigned size) { return mem_pool->allocate<Derived>(size); }

  void deallocate(Derived *ptr, unsigned size) {
    mem_pool->deallocate(ptr, size);
  }

public:
  void *operator new(size_t size) {
    static MemoryPool local_mem_pool;
    if (mem_pool == nullptr) {
      mem_pool = std::shared_ptr<MemoryPool>(&local_mem_pool);
    }
    return mem_pool->allocate<Derived>(size);
  }

  void operator delete(void *ptr) {
    mem_pool->deallocate_without_destructor((Derived *)ptr, 1);
  }
};

#endif