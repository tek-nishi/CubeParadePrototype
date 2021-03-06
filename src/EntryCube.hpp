﻿#pragma once

//
// StageCube登場
//

#include "GameEnvironment.hpp"
#include "cinder/gl/gl.h"
#include "cinder/Vector.h"
#include "cinder/Sphere.h"
#include "Message.hpp"
#include "Entity.hpp"


namespace ngs {

class EntryCube : public Entity {
  Message& message_;
  const ci::JsonTree& params_;
  
  bool active_;

  float size_;
  
  ci::Vec3f pos_;
  ci::Vec3f pos_start_;
  ci::Vec3f pos_end_;
  
  double active_time_;
  double active_time_end_;

  ci::Color color_;

  
public:
  explicit EntryCube(Message& message, ci::JsonTree& params) :
    message_(message),
    params_(params),
    active_(true),
    active_time_(0.0)
  { }

  void setup(boost::shared_ptr<EntryCube> obj_sp,
             const ci::Vec3i& entry_pos, const float start_offset_y,
             const double active_time, const ci::Color& color) {

    size_ = params_["cube.size"].getValue<float>();
    pos_end_   = ci::Vec3f(entry_pos) * size_;
    pos_start_ = pos_end_;
    pos_start_.y += start_offset_y;

    pos_ = pos_start_;

    color_ = color;
    
    active_time_end_ = active_time;
    
    message_.connect(Msg::UPDATE, obj_sp, &EntryCube::update);
    message_.connect(Msg::DRAW, obj_sp, &EntryCube::draw);
    message_.connect(Msg::RESET_STAGE, obj_sp, &EntryCube::inactive);
  }
  

private:
  bool isActive() const override { return active_; }

  
  void inactive(const Message::Connection& connection, Param& params) {
    active_ = false;
  }
  
  void update(const Message::Connection& connection, Param& params) {
    auto delta_time = boost::any_cast<double>(params.at("deltaTime"));

    active_time_ += delta_time;
    pos_.y = pos_start_.y + (pos_end_.y - pos_start_.y) * (active_time_ / active_time_end_);
    
    if (active_time_ > active_time_end_) {
      active_ = false;
      return;
    }
  }

  void draw(const Message::Connection& connection, Param& params) {
    ci::gl::color(color_);

    ci::Vec3f pos(pos_);
    // 上平面が(y = 0)
    pos.y -= size_ / 2;
    
    ci::gl::drawCube(pos, ci::Vec3f(size_, size_, size_));
  }
  
};

}
