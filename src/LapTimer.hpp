#pragma once

//
// 一定時間ごとにtrueを返すカウンタ
//

namespace ngs {

class LapTimer {
  float lap_time_;
  float goal_time_;

  bool paused_;
  

public:
  explicit LapTimer(const float goal_time) :
    lap_time_(0.0f),
    goal_time_(goal_time),
    paused_(false)
  { }

  
  bool operator()(const float delta_time) {
    if (paused_) return false;
    
    lap_time_ += delta_time;
    if (lap_time_ >= goal_time_) {
      lap_time_ -= goal_time_;
      return true;
    }
    
    return false;
  }

  void clear() { lap_time_ = 0.0f; }
  void set(const float time) { goal_time_ = time; }

  void pause(const bool pause = true) { paused_ = pause; }
  
};

}
