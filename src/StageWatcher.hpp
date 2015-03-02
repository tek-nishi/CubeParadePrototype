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
  int finish_line_;

  bool started_;
  int progress_;
  int player_num_;
  
  
public:
  explicit StageWatcher(Message& message) :
    message_(message),
    active_(true),
    start_line_(0),
    finish_line_(0),
    started_(false),
    progress_(0),
    player_num_(0)
  { }
  
  void setup(boost::shared_ptr<StageWatcher> obj_sp,
             const ci::JsonTree& params) {

    message_.connect(Msg::POST_STAGE_INFO, obj_sp, &StageWatcher::getStageInfo);

    message_.connect(Msg::CUBE_PLAYER_POS, obj_sp, &StageWatcher::check);
    message_.connect(Msg::CUBE_PLAYER_DEAD, obj_sp, &StageWatcher::deactivate);

    message_.connect(Msg::CREATE_CUBEPLAYER, obj_sp, &StageWatcher::createdPlayer);
  }

  
private:
  bool isActive() const override { return active_; }

  
  void getStageInfo(const Message::Connection& connection, Param& params) {
    start_line_ = boost::any_cast<int>(params["start_line"]);
    finish_line_ = boost::any_cast<int>(params["finish_line"]);
  }
  
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
      // 最大進んだ距離 -> スコア
      progress_ = std::max(pos.z, progress_);
      
      if (pos.z == finish_line_) {
        // Finish判定が済めば、もうこのタスクは必要ない
        active_ = false;

        message_.signal(Msg::PARADE_FINISH, Param());

        DOUT << "Parade Finish. score:" << progress_ << std::endl;
      }
    }
  }

  void deactivate(const Message::Connection& connection, Param& params) {
    active_ = false;
    DOUT << "Parade Finish. score:" << progress_ << std::endl;
  }

  void createdPlayer(const Message::Connection& connection, Param& params) {
    player_num_ += 1;
  }
  
};

}
