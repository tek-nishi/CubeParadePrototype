#pragma once

//
// ステージ
//

#include "GameEnvironment.hpp"
#include <vector>
#include "Message.hpp"
#include "StageCube.hpp"
#include "Task.hpp"
#include "TimerTask.hpp"
#include "LapTimer.hpp"


namespace ngs {

class Stage {
  Message& message_;
  Message::ConnectionHolder connection_holder_;
  
  const ci::JsonTree& params_;
  ci::Rand& random_;

  Task tasks_;
  TimerTask<double> timer_tasks_;
  
  float width_;
  float length_;

  float cube_size_;
  double build_speed_;
  
  LapTimer<double> collapse_timer_;
  size_t collapse_index_;

  LapTimer<double> build_timer_;
  
  std::deque<std::vector<StageCube> > cubes_;
  std::deque<std::vector<StageCube> > active_cubes_;

  size_t stage_block_length_;

  int start_line_;
  int finish_line_;
  int next_start_line_;

  bool started_;

  
public:
  Stage(Message& message,
        const ci::JsonTree& params, ci::Rand& random) :
    message_(message),
    params_(params),
    random_(random),
    cube_size_(params.getValueForKey<float>("cube.size")),
    collapse_timer_(params.getValueForKey<double>("stage.collapseSpeed")),
    collapse_index_(0),
    build_timer_(params.getValueForKey<double>("stage.buildSpeed")),
    build_speed_(params.getValueForKey<double>("stage.buildSpeed")),
    stage_block_length_(params.getValueForKey<size_t>("stage.startLength")),
    start_line_(0),
    finish_line_(0),
    next_start_line_(0),
    started_(false)
  {
    connection_holder_.add(message.connect(Msg::UPDATE, this, &Stage::update));
    connection_holder_.add(message.connect(Msg::DRAW, this, &Stage::draw));

    connection_holder_.add(message.connect(Msg::SETUP_STAGE, this, &Stage::setup));
    
    connection_holder_.add(message.connect(Msg::CUBE_STAGE_HEIGHT, this, &Stage::stageHight));
    
    connection_holder_.add(message.connect(Msg::CAMERAVIEW_INFO, this, &Stage::cameraviewInfo));

    connection_holder_.add(message.connect(Msg::PARADE_START, this, &Stage::start));
    connection_holder_.add(message.connect(Msg::PARADE_FINISH, this, &Stage::finish));
  }

  
private:
  void setup(const Message::Connection& connection, Param& params) {
    u_int width  = params_.getValueForKey<u_int>("stage.width");
    u_int length = params_.getValueForKey<u_int>("stage.length");

    width_  = width * cube_size_;
    length_ = stage_block_length_ * cube_size_;

    // Stage構築
    int offset_z = makeStage(params_["stage.start"], 0);
    start_line_ = offset_z - 1;

    offset_z += makeStage(params_["stage.data"][0], offset_z);
    finish_line_ = offset_z - 1;

    offset_z += makeStage(params_["stage.goal"], offset_z);
    next_start_line_ = offset_z - 1;

    // Stageから徐々に取り出して使う
    for (u_int iz = 0; iz < stage_block_length_; ++iz) {
      active_cubes_.push_back(cubes_.front());
      cubes_.pop_front();
    }

    {
      Param params = {
        { "start_line", start_line_ },
        { "finish_line", finish_line_ },
      };
      message_.signal(Msg::POST_STAGE_INFO, params);
    }
  }

  
  int makeStage(const ci::JsonTree& params, const int start_z) {
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
        if (z == goal_z) {
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

        Param params = {
          { "entry_pos", cube.posBlock() },
          { "offset_y", (5.0f + random_.nextFloat() * 1.0f) * cube_size_ },
          { "active_time", build_speed_ },
          { "color", cube.color() },
        };

        message_.signal(Msg::CREATE_ENTRYCUBE, params);
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

  
  void cameraviewInfo(const Message::Connection& connection, Param& params) {
    params["stage_bottom_z"] = float(collapse_index_ + collapse_timer_.lapseRate()) * cube_size_;
    params["stage_width"]    = width_;
    params["stage_length"]   = length_;
  }

  
  void start(const Message::Connection& connection, Param& params) {
    started_ = true;
  }

  void finish(const Message::Connection& connection, Param& params) {
    // finish_line_手前のStageを全て崩す
    // Goal地点も即時出現
    if (collapse_timer_.isActive()) collapse_timer_.setTimer(0.05);
    if (build_timer_.isActive()) build_timer_.setTimer(0.05);

    timer_tasks_.add(3.0, [this]() {
        if (collapse_timer_.isActive()) return;
        if (build_timer_.isActive()) return;

        started_ = false;
        
        collapse_timer_.setTimer(params_.getValueForKey<double>("stage.collapseSpeed"));
        // collapse_timer_.start();
        build_timer_.setTimer(params_.getValueForKey<double>("stage.buildSpeed"));
        build_timer_.start();
        
        // 次のステージを生成
        start_line_ = next_start_line_;

        int offset_z = next_start_line_ + 1;
        offset_z += makeStage(params_["stage.data"][1], offset_z);
        finish_line_ = offset_z - 1;

        offset_z += makeStage(params_["stage.goal"], offset_z);
        next_start_line_ = offset_z - 1;

        {
          Param params = {
            { "start_line", start_line_ },
            { "finish_line", finish_line_ },
          };
          message_.signal(Msg::POST_STAGE_INFO, params);
        }
      });

    tasks_.add([this]() {
        if (!build_timer_.isActive()) return false;

        if (active_cubes_.size() == stage_block_length_) {
          collapse_timer_.start();
          return true;
        }
        
        return false;
      });
  }
  
};

}
