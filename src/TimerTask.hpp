#pragma once

//
// 時間経過で関数オブジェクトを実行
//

#include <list>
#include <functional>


namespace ngs {

template <typename T>
class TimerTask {
  struct Object {
    T fire_time;
    std::function<void()> proc;

    Object(const T t, std::function<void()> p) :
      fire_time(t),
      proc(p)
    {}
  };

  std::list<Object> objects_;

  
  TimerTask(const TimerTask&) = delete;
  TimerTask& operator=(const TimerTask&) = delete;

  
public:
  TimerTask() = default;
  
  void add(const T fire_time, std::function<void()>& proc) {
    objects_.emplace_back(fire_time, proc);
  }

  void add(const T fire_time, std::function<void()>&& proc) {
    objects_.emplace_back(fire_time, proc);
  }
  
  void operator()(const T delta_time) {
    for (auto it = std::begin(objects_); it != std::end(objects_); /* do nothing */) {
      it->fire_time -= delta_time;
      if (it->fire_time <= static_cast<T>(0)) {
        it->proc();
        it = objects_.erase(it);
      }
      else {
        ++it;
      }
    }
  }
  
};

}
