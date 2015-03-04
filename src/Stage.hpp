#pragma once

//
// ステージ
//

#include "GameEnvironment.hpp"
#include <deque>
#include <vector>
#include "Message.hpp"
#include "Entity.hpp"
#include "StageCube.hpp"
#include "Task.hpp"
#include "TimerTask.hpp"
#include "LapTimer.hpp"


namespace ngs {

class Stage : public Entity {
  Message& message_;
  const ci::JsonTree& params_;
  ci::Rand random_;

  bool active_;

  Task tasks_;
  TimerTask<double> timer_tasks_;

  u_int current_stage_;
  u_int stage_num_;
  
  float width_;

  float cube_size_;

  double collapse_speed_;
  double build_speed_;
  
  LapTimer<double> collapse_timer_;
  size_t collapse_index_;

  LapTimer<double> build_timer_;
  
  std::deque<std::vector<StageCube> > cubes_;
  std::deque<std::vector<StageCube> > active_cubes_;

  size_t start_block_length_;
  size_t goal_block_length_;
  size_t field_block_length_;

  size_t stage_block_length_;

  int start_line_;
  int finish_line_;
  int next_start_line_;

  bool started_;

  
public:
  Stage(Message& message, ci::JsonTree& params) :
    message_(message),
    params_(params),
    active_(true),
    current_stage_(0),
    collapse_index_(0),
    start_line_(0),
    finish_line_(0),
    next_start_line_(0),
    started_(false)
  { }

  void setup(boost::shared_ptr<Stage> obj_sp) {
    cube_size_ = params_.getValueForKey<float>("cube.size");

    stage_block_length_ = params_.getValueForKey<size_t>("stage.startLength");

    stage_num_ = params_["stage.data"].getNumChildren();
    
    message_.connect(Msg::UPDATE, obj_sp, &Stage::update);
    message_.connect(Msg::DRAW, obj_sp, &Stage::draw);

    message_.connect(Msg::SETUP_STAGE, obj_sp, &Stage::setupStage);
    message_.connect(Msg::RESET_STAGE, obj_sp, &Stage::inactive);
    
    message_.connect(Msg::CUBE_STAGE_HEIGHT, obj_sp, &Stage::stageHight);
    
    message_.connect(Msg::GATHER_INFORMATION, obj_sp, &Stage::gatherInfo);

    message_.connect(Msg::PARADE_START, obj_sp, &Stage::start);
    message_.connect(Msg::PARADE_FINISH, obj_sp, &Stage::finish);
  }

  
private:
  bool isActive() const override { return active_; }


  void setupStage(const Message::Connection& connection, Param& params) {
    width_ = params_.getValueForKey<u_int>("stage.width") * cube_size_;

    // Stage構築
    start_block_length_ = makeStage(params_["stage.start"], 0);
    int offset_z = start_block_length_;
    start_line_ = offset_z - 1;

    const auto& stage_data = params_["stage.data"][current_stage_];
    field_block_length_ = makeStage(stage_data, offset_z);
    offset_z += field_block_length_;
    finish_line_ = offset_z - 1;

    collapse_speed_ = stage_data.getValueForKey<double>("collapseSpeed");
    build_speed_    = stage_data.getValueForKey<double>("buildSpeed");
    collapse_timer_.setTimer(collapse_speed_);
    build_timer_.setTimer(build_speed_);
    
    goal_block_length_ = makeStage(params_["stage.goal"], offset_z);
    offset_z += goal_block_length_;
    next_start_line_ = offset_z - 1;

    finishEntry(stage_data, finish_line_ + 1);
    
    // Stageから徐々に取り出して使う
    for (u_int iz = 0; iz < stage_block_length_; ++iz) {
      active_cubes_.push_back(cubes_.front());
      cubes_.pop_front();
    }

    {
      Param params = {
        { "start_line",  start_line_ },
        { "finish_line", finish_line_ },
        { "final_stage", isFinalStage(current_stage_) },
      };
      message_.signal(Msg::POST_STAGE_INFO, params);
    }
  }

  void inactive(const Message::Connection& connection, Param& params) {
    active_ = false;
  }

  void update(const Message::Connection& connection, Param& params) {
    {
      // 光源の更新
      float z = (collapse_index_ + collapse_timer_.lapseRate()) * cube_size_;
      ci::Vec3f pos(width_ / 2.0, 0.0, z);
      
      Param params = {
        { "stage_pos", pos },
      };
      message_.signal(Msg::STAGE_POS, params);
    }
    
    auto delta_time = boost::any_cast<double>(params.at("deltaTime"));
    timer_tasks_(delta_time);
    tasks_();

    if (!started_) return;
    
    if (collapse_timer_(delta_time)) {
      // 一定時間ごとにステージ端が崩壊
      const auto& cube_line = active_cubes_.front();
      for (const auto& cube : cube_line) {
        if (!cube.isActive()) continue;
        
        Param params = {
          { "entry_pos", cube.posBlock() },
          { "color", cube.color() },
          { "speed", 1.0f + random_.nextFloat() }
        };
        
        message_.signal(Msg::CREATE_FALLCUBE, params);
      };
      active_cubes_.pop_front();
      
      collapse_index_ += 1;
      if (collapse_index_ == finish_line_) {
        collapse_timer_.stop();
      }
    }

    if (build_timer_(delta_time)) {
      // 一定時間ごとにステージを生成
      // Cubeの追加演出
      for (const auto& cube : cubes_.front()) {
        if (!cube.isActive()) continue;

        auto pos_block = cube.posBlock();
        float y = (5.0f + random_.nextFloat() * 1.0f) * cube_size_;
        Param params = {
          { "entry_pos", pos_block },
          { "offset_y", y },
          { "active_time", build_speed_ },
          { "color", cube.color() },
        };

        message_.signal(Msg::CREATE_ENTRYCUBE, params);

        if (cube.isOnEntity()) {
          // Player生成
          params["entry_pos"] = ci::Vec3i(pos_block.x, pos_block.y + 1, pos_block.z);
          params["offset_y"]  = y + cube_size_;
          params["color"]     = Json::getColor<float>(params_["CubePlayer.color"]);
          
          message_.signal(Msg::CREATE_ENTRYCUBE, params);

          timer_tasks_.add(build_speed_, [this, pos_block]() {
              Param params = {
                { "entry_pos", pos_block },
                { "paused", true },
              };
              message_.signal(Msg::CREATE_CUBEPLAYER, params);
            });
        }
      }

      // ステージの追加は追加演出の後に行うので、タスクに積んでおく
      auto cube_line = cubes_.front();
      timer_tasks_.add(build_speed_, [this, cube_line]() {
          active_cubes_.push_back(cube_line);
        });

      cubes_.pop_front();
      if (cubes_.empty()) {
        build_timer_.stop();
      }
    }
  }

  void draw(const Message::Connection& connection, Param& params) {
    for (const auto& cube_line : active_cubes_) {
      for (const auto& cube : cube_line) {
        cube.draw();
      }
    }
  }
  
  void stageHight(const Message::Connection& connection, Param& params) {
    params["is_cube"] = false;
    
    const auto& pos = boost::any_cast<const ci::Vec3i& >(params["block_pos"]);
    u_int z = pos.z - collapse_index_;
    if (isValidCube(pos.x, z)) {
      params["is_cube"] = true;
      params["height"]  = active_cubes_[z][pos.x].posBlock();
    }
  }

  bool isValidCube(const u_int x, const u_int z) {
    return (z < active_cubes_.size())
      && (x < active_cubes_[z].size())
      && active_cubes_[z][x].isActive();
  }

  
  void gatherInfo(const Message::Connection& connection, Param& params) {
    params["stageWidth"]   = width_;
    params["stageLength"]  = active_cubes_.size() * cube_size_;
    params["stageBottomZ"] = float(collapse_index_ + collapse_timer_.lapseRate()) * cube_size_;
  }

  
  void start(const Message::Connection& connection, Param& params) {
    started_ = true;
  }

  void finish(const Message::Connection& connection, Param& params) {
    // finish_line_手前のStageを全て崩す
    // Goal地点も即時出現
    if (collapse_timer_.isActive()) collapse_timer_.setTimer(0.05);
    if (build_timer_.isActive()) build_timer_.setTimer(0.05);

    timer_tasks_.add(2.5, [this]() {
        // stageの全消去とGoal地点の生成を待って、次のstageの生成準備
        if (collapse_timer_.isActive()) return;
        if (build_timer_.isActive()) return;

        size_t stage_num = params_["stage.data"].getNumChildren();
        current_stage_ += 1;
        if (current_stage_ == stage_num) {
          // 全stageクリア
          DOUT << "all stage clear!!" << std::endl;
          message_.signal(Msg::ALL_STAGE_CLEAR, Param());
          return;
        }

        bool filal_stage = isFinalStage(current_stage_);

        // 次のステージの一部分をさっさと生成
        build_timer_.setTimer(build_speed_ / 3);
        build_timer_.start();
        
        // 次のステージを生成
        start_line_ = next_start_line_;

        int offset_z = next_start_line_ + 1;
        const auto& stage_data = params_["stage.data"][current_stage_];
        field_block_length_ = makeStage(stage_data, offset_z);
        offset_z += field_block_length_;
        finish_line_ = offset_z - 1;

        // 生成と崩壊速度の再指定
        collapse_speed_ = stage_data.getValueForKey<double>("collapseSpeed");
        build_speed_    = stage_data.getValueForKey<double>("buildSpeed");
        
        std::string key_id = std::string("stage") + (filal_stage ? ".finalGoal"
                                                                 : ".goal");
        goal_block_length_ = makeStage(params_[key_id], offset_z, filal_stage ? false : true);
        offset_z += goal_block_length_;
        next_start_line_ = offset_z - 1;

        finishEntry(stage_data, field_block_length_);
        
        {
          Param params = {
            { "start_line",  start_line_ },
            { "finish_line", finish_line_ },
            { "final_stage", filal_stage },
          };
          message_.signal(Msg::POST_STAGE_INFO, params);
        }

        tasks_.add([this]() {
            // stageが規定サイズ生成されたらstage開始!!
            if (cubes_.size() ==
                (goal_block_length_ * 2 + field_block_length_ - stage_block_length_)) {
              collapse_timer_.setTimer(collapse_speed_);
              collapse_timer_.start();
              build_timer_.setTimer(build_speed_);
              started_ = false;

              // start lineの書き換え
              // TODO:ゲートオープン的な演出
              auto& cube_line = active_cubes_[goal_block_length_];
              for (auto& cube : cube_line) {
                const auto pos = cube.posBlock();
                cube.posBlock(ci::Vec3i(pos.x, pos.y - 1, pos.z));
              }
              
              return true;
            }
        
            return false;
          });
        
      });
  }


  bool isFinalStage(size_t current_stage) const {
    return current_stage == (stage_num_ - 1);
  }
  
  int makeStage(const ci::JsonTree& params, const int start_z, bool finish_line = true) {
    const auto& body = params["body"];

    int z      = start_z;
    int goal_z = z + body.getNumChildren() - 1;

    for (const auto& body_line : body) {
      std::vector<StageCube> stage_line;
      int x = 0;
      for (const auto& cube : body_line) {
        int y = cube.getValue<int>();
        // 高さがマイナス -> ブロックなし
        bool active = y >= 0;

        // FIXME:互い違いの色
        auto color = ((x + z) & 1) ? ci::Color(0.8f, 0.8f, 0.8f)
                                   : ci::Color(0.6f, 0.6f, 0.6f);
        // 終端がスタート & ゴールライン
        if (finish_line && (z == goal_z)) {
          color *= ci::Color(1.0f, 0.0f, 0.0f);
        }

        stage_line.emplace_back(ci::Vec3i(x, y, z), cube_size_, active, color);
        x += 1;
      }
      cubes_.push_back(std::move(stage_line));
      z += 1;
    }

    return body.getNumChildren();
  }

  void finishEntry(const ci::JsonTree& params, const u_int finish_line) {
    if (!params.hasChild("finishEntry")) return;

    for (const auto& entry : params["finishEntry"]) {
      const auto& pos = Json::getVec2<int>(entry);

      auto& cube = cubes_[pos.y + finish_line][pos.x];
      cube.onEntity(true);
      cube.entityType(StageCube::ON_PLAYER);
    }
  }
  
};

}
