#pragma once

//
// 一定時間ごとにtrueを返すカウンタ
//

namespace ngs {

template <typename T>
class LapTimer {
  T lap_time_;
  T goal_time_;

  bool paused_;
  

public:
  LapTimer() :
    lap_time_(static_cast<T>(0)),
    goal_time_(static_cast<T>(1)),
    paused_(false)
  { }
  
  explicit LapTimer(const T goal_time) :
    lap_time_(static_cast<T>(0)),
    goal_time_(goal_time),
    paused_(false)
  { }

  
  bool operator()(const T delta_time) {
    if (paused_) return false;
    
    lap_time_ += delta_time;
    if (lap_time_ >= goal_time_) {
      lap_time_ -= goal_time_;
      return true;
    }
    
    return false;
  }

  void clear() { lap_time_ = static_cast<T>(0); }
  void setTimer(const T time) { goal_time_ = time; }

  void pause(const bool pause = true) { paused_ = pause; }

  void stop() {
    pause();
    clear();
  }

  void start() {
    clear();
    pause(false);
  }

  
  T lapseRate() const { return lap_time_ / goal_time_; }
  bool isActive() const { return !paused_; }
  
};

}
