#pragma once

//
// 時間経過で関数オブジェクトを実行
//

#include <vector>
#include <functional>
#include <boost/range/algorithm_ext/erase.hpp>


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

  std::vector<Object> objects_;

  
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
    // TIPS:コンテナに対してremoveとeraseを同時に処理
    boost::remove_erase_if(objects_,
                           [delta_time](Object& obj) {
                             obj.fire_time -= delta_time;
                             if (obj.fire_time <= 0.0f) {
                               obj.proc();
                               return true;
                             }
                             return false;
                           });
  }
  
};

}
