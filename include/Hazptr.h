#ifndef HAZPTR_H_
#define HAZPTR_H_

#include <atomic>
#include <unordered_set>

struct hazptr_holder;
struct hazptr_domain;
template <typename T> struct hazptr_obj_base;
struct hazptr_retire_obj;
struct hazptr_rec;

hazptr_domain *get_default_hazptr_domain();

struct hazptr_rec {
  std::atomic<const void *> ptr_ = {nullptr};
  hazptr_rec *next_ = {nullptr};
  std::atomic<bool> active_ = {false};
};

struct hazptr_holder {
  hazptr_domain *domain_;
  hazptr_rec *hptr_;

  hazptr_holder();
  template <typename T> void protect(T *raw_ptr);
  void release_protection();
};

struct hazptr_domain {
  std::atomic<hazptr_rec *> hptr_list = {nullptr};
  std::atomic<hazptr_retire_obj *> retire_list = {nullptr};

  std::atomic<hazptr_rec *> next_available_hptr_head = {nullptr};

  ~hazptr_domain();

  hazptr_rec *acquire_hazptr();
  template <typename T> void retire(hazptr_obj_base<T> *base_obj);
  void retire(hazptr_retire_obj *reclaim_obj);
  void reclaim_if_reach_threshold();
};

template <typename T> struct hazptr_obj_base {
  hazptr_domain *domain_;

  hazptr_obj_base();
  void retire();
};

struct hazptr_retire_obj : hazptr_obj_base<hazptr_retire_obj> {
  const void *const raw_ptr_;
  hazptr_retire_obj *next_ = {nullptr};
  hazptr_retire_obj(const void *ptr) : raw_ptr_(ptr) {}
};

inline hazptr_domain *get_default_hazptr_domain() {
  static hazptr_domain d;
  return &d;
}

inline hazptr_holder::hazptr_holder() : domain_(get_default_hazptr_domain()) {}

template <typename T> void hazptr_holder::protect(T *raw_ptr) {
  hptr_ = domain_->acquire_hazptr();
  hptr_->ptr_.store(raw_ptr);
}

inline void hazptr_holder::release_protection() {
  hptr_->ptr_.store(nullptr);
  hptr_->active_.store(false);
}

inline hazptr_domain::~hazptr_domain() {
  // release all hptr
  // relase all retire obj
}

inline hazptr_rec *hazptr_domain::acquire_hazptr() {
  hazptr_rec *new_hptr = new hazptr_rec();
  new_hptr->active_.store(true);
  while (!hptr_list.compare_exchange_weak(new_hptr->next_, new_hptr))
    ;
  return new_hptr;
}

template <typename T> void hazptr_domain::retire(hazptr_obj_base<T> *base_obj) {
  retire(new hazptr_retire_obj(base_obj));
}

inline void hazptr_domain::retire(hazptr_retire_obj *reclaim_obj) {
  // add next avail?
  while (!retire_list.compare_exchange_weak(reclaim_obj->next_, reclaim_obj))
    ;
  reclaim_if_reach_threshold();
}

inline void hazptr_domain::reclaim_if_reach_threshold() {
  //
  hazptr_rec *cur = hptr_list.load();
  std::unordered_set<const void *> raw_ptr_set;
  while (cur) {
    if (cur->ptr_ != nullptr) {
      raw_ptr_set.insert(cur->ptr_);
    }
    cur = cur->next_;
  }

  hazptr_retire_obj *old_retire_list = new hazptr_retire_obj(nullptr);
  old_retire_list->next_ = retire_list.exchange(nullptr);
  hazptr_retire_obj *cur_retire = old_retire_list->next_;
  hazptr_retire_obj *prev_retire = nullptr;
  hazptr_retire_obj *next_retire = nullptr;

  while (cur_retire) {
    next_retire = cur_retire->next_;
    if (raw_ptr_set.find(cur_retire) != raw_ptr_set.end()) {
      delete cur_retire;
      prev_retire = next_retire;
    } else {
      prev_retire = cur_retire;
    }
    cur_retire = next_retire;
  }

  while (prev_retire && !retire_list.compare_exchange_weak(
                            prev_retire->next_, old_retire_list->next_))
    ;
}

template <typename T>
hazptr_obj_base<T>::hazptr_obj_base() : domain_(get_default_hazptr_domain()) {}

template <typename T> void hazptr_obj_base<T>::retire() {
  domain_->retire(this);
}

#endif // HAZPTR_H_