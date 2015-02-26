#pragma once

//
// ゲーム開始と終了の監視
//


#include "GameEnvironment.hpp"
#include "Entity.hpp"
#include "Message.hpp"


namespace ngs {

class StageWatcher : public Entity {
  Message& message_;

  bool active_;

  int start_line_;
  int goal_line_;

  bool started_;
  
  
public:
  explicit StageWatcher(Message& message) :
    message_(message),
    active_(true),
    started_(false)
  { }
  
  void setup(boost::shared_ptr<StageWatcher> obj_sp,
             const ci::JsonTree& params) {

    start_line_ = params["startLine"].getValue<int>();
    goal_line_  = params["finishLine"].getValue<int>();

    message_.connect(Msg::CUBE_PLAYER_POS, obj_sp, &StageWatcher::check);
    message_.connect(Msg::CUBE_PLAYER_DEAD, obj_sp, &StageWatcher::deactivate);
  }

  
private:
  bool isActive() const override { return active_; }

  void check(const Message::Connection& connection, Param& params) {
    const auto& pos = boost::any_cast<const ci::Vec3i& >(params["block_pos"]);
    if (!started_) {
      if (pos.z == start_line_) {
        started_ = true;

        message_.signal(Msg::PARADE_START, Param());

        DOUT << "Parade Start!!" << std::endl;
      }
    }
    else {
      if (pos.z == goal_line_) {
        // Finish判定が済めば、もうこのタスクは必要ない
        active_ = false;

        message_.signal(Msg::PARADE_FINISH, Param());

        DOUT << "Parade Finish!!" << std::endl;
      }
    }
  }

  void deactivate(const Message::Connection& connection, Param& params) {
    active_ = false;
  }
  
};

}
