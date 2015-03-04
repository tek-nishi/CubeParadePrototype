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
  const ci::JsonTree& params_;

  bool active_;

  int start_line_;
  int finish_line_;
  bool final_stage_;

  bool started_;
  bool finished_;
  
  int progress_;
  
  
public:
  explicit StageWatcher(Message& message, ci::JsonTree& params) :
    message_(message),
    params_(params),
    active_(true),
    start_line_(0),
    finish_line_(0),
    started_(false),
    finished_(false),
    progress_(0)
  { }
  
  void setup(boost::shared_ptr<StageWatcher> obj_sp) {
    message_.connect(Msg::POST_STAGE_INFO, obj_sp, &StageWatcher::getStageInfo);

    message_.connect(Msg::CUBE_PLAYER_POS, obj_sp, &StageWatcher::check);
    message_.connect(Msg::CUBE_PLAYER_DEAD, obj_sp, &StageWatcher::inactive);
  }

  
private:
  bool isActive() const override { return active_; }

  
  void getStageInfo(const Message::Connection& connection, Param& params) {
    start_line_  = boost::any_cast<int>(params["start_line"]);
    finish_line_ = boost::any_cast<int>(params["finish_line"]);
    final_stage_ = boost::any_cast<bool>(params["final_stage"]);

    started_  = false;
    finished_ = false;
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
    else if (!finished_) {
      // 最大進んだ距離 -> スコア
      progress_ = std::max(pos.z, progress_);
      
      if (pos.z == finish_line_) {
        auto value = std::vector<int>();
        Param params = {
          { "playerZ", value },
        };
        message_.signal(Msg::CUBE_PLAYER_CHECK_FINISH, params);
        
        const auto& player_z = boost::any_cast<const std::vector<int>& >(params["playerZ"]);
        if (player_z.empty()) return;
        for (const auto z : player_z) {
          if (z < finish_line_) return;
        }
        
        finished_ = true;

        message_.signal(Msg::PARADE_FINISH, Param());

        DOUT << "Parade Finish. score:" << progress_ << std::endl;
        if (final_stage_) {
          DOUT << "Cleared final stage." << std::endl;
          active_ = false;
        }
      }
    }
  }

  void inactive(const Message::Connection& connection, Param& params) {
    message_.signal(Msg::PARADE_MISS, params);
    active_ = false;
    DOUT << "Parade Miss. score:" << progress_ << std::endl;
  }
  
};

}
