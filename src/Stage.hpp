#pragma once

//
// ステージ
//

#include "GameEnvironment.hpp"
#include <vector>
#include "Message.hpp"
#include "StageCube.hpp"
#include "TimerTask.hpp"
#include "LapTimer.hpp"


namespace ngs {

class Stage {

public:
  Stage(Message& message,
        const ci::JsonTree& params, ci::Rand& random) :
    message_(message),
    params_(params),
    random_(random),
    cube_size_(params.getValueForKey<float>("cube.size")),
    finish_line_(params.getValueForKey<u_int>("stage.finishLine")),
    collapse_timer_(params.getValueForKey<float>("stage.collapseSpeed")),
    collapse_index_(0),
    build_timer_(params.getValueForKey<float>("stage.buildSpeed")),
    build_index_(0),
    build_speed_(params.getValueForKey<float>("stage.buildSpeed")),
    started_(false),
    finished_(false)
  {
    u_int width        = params.getValueForKey<u_int>("stage.width");
    u_int length       = params.getValueForKey<u_int>("stage.length");
    u_int start_length = params.getValueForKey<u_int>("stage.startLength");
    u_int start_line   = params.getValueForKey<u_int>("stage.startLine");

    width_  = width * cube_size_;
    length_ = start_length * cube_size_;
    
    ci::Color color_a = ci::Color(0.8f, 0.8f, 0.8f);
    ci::Color color_b = ci::Color(0.6f, 0.6f, 0.6f);

    // FIXME:テスト生成
    for (u_int iz = 0; iz < length; ++iz) {
      std::vector<StageCube> cube_line;
      cube_line.reserve(width);
      for (u_int ix = 0; ix < width; ++ix) {
        int y = 0;
        bool active = true;

        if ((iz > start_line) && (iz < finish_line_)) {
          // 時々高いの
          if (random_.nextFloat() < 0.05) {
            y = 1;
          }

          // 時々非表示
          if (random_.nextFloat() < 0.05) {
            active = false;
          }
        }
        

        ci::Color color = color_a;
        if ((iz == start_line) || (iz == finish_line_)) {
          color = ci::Color(1.0f, 0.0f, 0.0f) * color;
        }
        
        cube_line.emplace_back(ci::Vec3i(ix, y, iz), cube_size_, active, color);
        
        std::swap(color_a, color_b);
      }
      cubes_.push_back(std::move(cube_line));
    }

    // 初期状態のステージを別のコンテナにコピー
    for (u_int iz = 0; iz < start_length; ++iz) {
      active_cubes_.push_back(cubes_[iz]);
    }
    build_index_ = start_length;

    connection_holder_.add(message.connect(Msg::UPDATE, this, &Stage::update));
    connection_holder_.add(message.connect(Msg::DRAW, this, &Stage::draw));

    connection_holder_.add(message.connect(Msg::CUBE_STAGE_INFO, this, &Stage::info));
    connection_holder_.add(message.connect(Msg::CUBE_STAGE_HEIGHT, this, &Stage::stageHight));
    
    connection_holder_.add(message.connect(Msg::CAMERAVIEW_INFO, this, &Stage::cameraviewInfo));

    connection_holder_.add(message.connect(Msg::PARADE_START, this, &Stage::start));
    connection_holder_.add(message.connect(Msg::PARADE_FINISH, this, &Stage::finish));
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

    if (!started_) return;
    
    double delta_time = boost::any_cast<double>(params.at("deltaTime"));

    time_task_(delta_time);
    
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
      for (const auto& cube : cubes_[build_index_]) {
        if (!cube.isActive()) continue;

        Param params = {
          { "entry_pos", cube.posBlock() },
          { "offset_y", (5.0f + random_.nextFloat() * 1.0f) * cube_size_ },
          { "active_time", build_speed_ },
          { "color", cube.color() },
        };

        message_.signal(Msg::CREATE_ENTRYCUBE, params);
      }

      // TIPS:メンバ変数をキャプチャして使う場合は一旦変数にコピー
      // ステージの追加は追加演出の後に行うので、タスクに積んでおく
      size_t build_index = build_index_;
      time_task_.add(build_speed_, [this, build_index]() {
          active_cubes_.push_back(cubes_[build_index]);
        });

      build_index_ += 1;
      if (build_index_ == cubes_.size()) {
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

  
private:
  void info(const Message::Connection& connection, Param& params) {
    // FIXME:コピーはダメです
    params["info"] = &active_cubes_;
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
    finished_ = true;
  }

  
  Message& message_;
  Message::ConnectionHolder connection_holder_;
  
  const ci::JsonTree& params_;
  ci::Rand& random_;

  TimerTask<double> time_task_;
  
  float width_;
  float length_;

  float cube_size_;
  u_int finish_line_;
  float build_speed_;
  
  LapTimer<double> collapse_timer_;
  size_t   collapse_index_;

  LapTimer<double> build_timer_;
  size_t   build_index_;
  
  std::deque<std::vector<StageCube> > cubes_;
  std::deque<std::vector<StageCube> > active_cubes_;

  bool started_;
  bool finished_;
  
};

}
