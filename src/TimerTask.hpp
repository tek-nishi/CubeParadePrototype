#pragma once

//
// 時間経過で関数オブジェクトを実行
//

#include <deque>
#include <functional>


namespace ngs {

class TimerTask {
  struct Object {
    float fire_time;
    std::function<void()> proc;
  };

  std::deque<Object> objects_;

  
  TimerTask(const TimerTask&) = delete;
  TimerTask& operator=(const TimerTask&) = delete;

  
public:
  TimerTask() = default;
  
  void add(const float fire_time, std::function<void()> proc) {
    // コンテナ格納時にfire_timeの小さい順に並ぶ
    objects_.push_back({ fire_time, proc });
  }
  
  void operator()(const float delta_time) {
    // 先頭のを取り出し、有効なら最後尾に追加する
    size_t num = objects_.size();
    while (num > 0) {
      auto& obj = objects_.front();

      obj.fire_time -= delta_time;
      if (obj.fire_time <= 0.0f) {
        // 指定時間経過で関数を実行
        obj.proc();
      }
      else {
        // まだなら何もしないで最後尾に追加
        objects_.push_back(obj);
      }
      objects_.pop_front();
      
      num -= 1;
    }
  }
  
};

}
