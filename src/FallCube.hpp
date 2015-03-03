#pragma once

//
// 崩れ落ちるCube
//

#include "GameEnvironment.hpp"
#include "cinder/gl/gl.h"
#include "cinder/Vector.h"
#include "cinder/Sphere.h"
#include "Message.hpp"
#include "Entity.hpp"


namespace ngs {

class FallCube : public Entity {
  Message& message_;
  const ci::JsonTree& params_;

  bool active_;

  float size_;
  
  ci::Vec3f pos_;

  float active_time_;
  
  ci::Vec3f vec_;
  ci::Vec3f acc_;

  ci::Color color_;

  
public:
  explicit FallCube(Message& message, ci::JsonTree& params) :
    message_(message),
    params_(params),
    active_(true),
    vec_(ci::Vec3f::zero())
  { }

  void setup(boost::shared_ptr<FallCube> obj_sp,
             const ci::Vec3i& entry_pos, const float speed, const ci::Color& color) {

    size_ = params_["cube.size"].getValue<float>();
    pos_  = ci::Vec3f(entry_pos) * size_;

    color_ = color;

    active_time_ = params_["fallCube.activeTime"].getValue<float>();
    acc_ = Json::getVec3<float>(params_["fallCube.acc"]);
    acc_.y = acc_.y * speed;
    
    message_.connect(Msg::UPDATE, obj_sp, &FallCube::update);
    message_.connect(Msg::DRAW, obj_sp, &FallCube::draw);
  }
  

private:
  bool isActive() const override { return active_; }
  
  void update(const Message::Connection& connection, Param& params) {
    double delta_time = boost::any_cast<double>(params.at("deltaTime"));

    // s = v0 * t + 0.5 * a * t^2
    // v = v0 + a * t
    pos_ += vec_ * delta_time + acc_ * 0.5f * delta_time * delta_time;
    vec_ = vec_ + acc_ * delta_time;

    active_time_ -= delta_time;
    if (active_time_ < 0.0) {
      ci::Sphere sphere(pos_, size_);
      // Cameraから見える領域から外れたら削除
      auto& frustum = boost::any_cast<ci::Frustumf& >(params["frustum"]);
      if (!frustum.contains(sphere) && !frustum.intersects(sphere)) {
        active_ = false;
      }
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
