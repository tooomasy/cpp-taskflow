#ifndef __CONDNODE_H__
#define __CONDNODE_H__

#include <functional>

class CondNode {
  //  private:
  //   std::function<bool()> cond_wrapper = []() { return true; };

  //  public:
  //   CondNode() = default;

  //   template <typename Cond, typename... Args>
  //   void set_condition(Cond condition) {
  //     cond_wrapper = [=](Args&&... args) {
  //       return condition(std::forward<Args>(args)...);
  //     };
  //   }

  //   bool is_valid_branch_checking() { return cond_wrapper(); }
};

#endif