#pragma once

//
// タスク
// 関数オブジェクトがtrueを返すとタスクから削除する
//

#include <list>
#include <functional>


namespace ngs {

class Task {
  struct Object {
    std::function<bool()> proc;

    Object(std::function<bool()> p) :
      proc(p)
    {}
  };

  // 実行中に追加されることもあるのでvectorではなくlistを使う
  std::list<Object> objects_;

  
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
    // FIXME:task実行中にtaskへ追加が行われることがあるので
    //       この実装になっている(std::listはコンテナへの操作でiteratorが無効にならない)
    for (auto it = std::begin(objects_); it != std::end(objects_); /* do nothing */) {
      if (it->proc()) it = objects_.erase(it);
      else            ++it;
    }
  }
  
};

}
