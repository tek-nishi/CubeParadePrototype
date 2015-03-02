#pragma once

//
// タスク
// 関数オブジェクトがtrueを返すとタスクから削除する
//

#include <vector>
#include <functional>
#include <boost/range/algorithm_ext/erase.hpp>

namespace ngs {

class Task {
  struct Object {
    std::function<bool()> proc;

    Object(std::function<bool()> p) :
      proc(p)
    {}
  };

  std::vector<Object> objects_;

  
  Task(const Task&) = delete;
  Task& operator=(const Task&) = delete;

  
public:
  Task() = default;
  
  void add(std::function<bool()>& proc) {
    objects_.emplace_back(proc);
  }

  void add(std::function<bool()>&& proc) {
    objects_.emplace_back(proc);
  }
  
  void operator()() {
    // TIPS:コンテナに対してremoveとeraseを同時に処理
    boost::remove_erase_if(objects_,
                           [](Object& obj) {
                             return obj.proc();
                           });
  }
  
};

}
