#pragma once

//
// ステージ
//

#include "GameEnvironment.hpp"
#include <vector>
#include "Message.hpp"
#include "StageCube.hpp"


namespace ngs {

class Stage {

public:
  Stage(Message& message,
        const ci::JsonTree& params, ci::Rand& random) :
    message_(message),
    params_(params),
    random_(random),
    cube_size_(params.getValueForKey<float>("cube.size")),
    spawn_speed_(params.getValueForKey<float>("stage.speed")),
    next_spawn_(0),
    cube_index_(0),
    started_(false),
    finished_(false)
  {
    u_int width        = params.getValueForKey<u_int>("stage.width");
    u_int length       = params.getValueForKey<u_int>("stage.length");
    u_int start_length = params.getValueForKey<u_int>("stage.startLength");
    u_int start_line   = params.getValueForKey<u_int>("stage.startLine");
    u_int finish_line  = params.getValueForKey<u_int>("stage.finishLine");

    width_  = width * cube_size_;
    length_ = start_length * cube_size_;
    
    ci::Color color_a = ci::Color(0.8, 0.8, 0.8);
    ci::Color color_b = ci::Color(0.6, 0.6, 0.6);

    // FIXME:テスト生成
    for (u_int iz = 0; iz < length; ++iz) {
      std::vector<StageCube> cube_line;
      cube_line.reserve(width);
      for (u_int ix = 0; ix < width; ++ix) {
        int y = 0;
        bool active = true;

        if ((iz > start_line) && (iz < finish_line)) {
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
        if ((iz == start_line) || (iz == finish_line)) {
          color = ci::Color(1, 0, 0) * color;
        }
        
        cube_line.emplace_back(ci::Vec3i(ix, y, iz), cube_size_, active, color);
        
        std::swap(color_a, color_b);
      }
      cubes_.push_back(std::move(cube_line));
    }

    for (u_int iz = 0; iz < start_length; ++iz) {
      active_cubes_.push_back(cubes_[iz]);
    }
    spawn_index_ = start_length;

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
      float offset = (spawn_index_ < cubes_.size()) ? next_spawn_ / spawn_speed_
                                                    : 0.0f;
      float z = (cube_index_ + offset) * cube_size_;
      ci::Vec3f pos(width_ / 2.0, 0.0, z);
      
      Param params = {
        { "stage_pos", pos },
      };
      message_.signal(Msg::STAGE_POS, params);
    }

    if (!started_) return;
    
    double delta_time = boost::any_cast<double>(params.at("deltaTime"));
    
    next_spawn_ += delta_time;
    if ((spawn_index_ <= cubes_.size()) && (next_spawn_ > spawn_speed_)) {
      next_spawn_ -= spawn_speed_;

      // Cubeの落下演出
      if (!active_cubes_.empty()) {
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
        cube_index_ += 1;
      }
      
      // Cubeの追加演出
      if (spawn_index_ < cubes_.size()) {
        for (const auto& cube : cubes_[spawn_index_]) {
          if (!cube.isActive()) continue;

          Param params = {
            { "entry_pos", cube.posBlock() },
            { "offset_y", (5.0f + random_.nextFloat() * 1.0f) * cube_size_ },
            { "active_time", spawn_speed_ },
            { "color", cube.color() },
          };

          message_.signal(Msg::CREATE_ENTRYCUBE, params);
        }
      }

      // 最初の１回目はCube落下開始演出のため、ステージを広げない
      if (!first_spawn_) {
        active_cubes_.push_back(cubes_[spawn_index_ - 1]);
      }
      spawn_index_ += 1;
      first_spawn_ = false;
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
    u_int z = pos.z - cube_index_;
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
    // Stageの先端z座標
    float offset = (spawn_index_ < cubes_.size()) ? next_spawn_ / spawn_speed_
                                                  : 0.0f;
    
    params["stage_bottom_z"]  = (cube_index_ + offset) * cube_size_;
    params["stage_width"]  = width_;
    params["stage_length"] = length_;
  }

  
  void start(const Message::Connection& connection, Param& params) {
    started_ = true;
    first_spawn_ = true;
  }

  void finish(const Message::Connection& connection, Param& params) {
    finished_ = true;
  }

  
  Message& message_;
  Message::ConnectionHolder connection_holder_;
  
  const ci::JsonTree& params_;
  ci::Rand& random_;

  float width_;
  float length_;

  float cube_size_;

  size_t spawn_index_;
  float spawn_speed_;
  float next_spawn_;
  bool first_spawn_;

  std::deque<std::vector<StageCube> > cubes_;
  std::deque<std::vector<StageCube> > active_cubes_;
  size_t cube_index_;

  bool started_;
  bool finished_;
  
};

}
