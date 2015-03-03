#pragma once

//
// 光源
//

#include "GameEnvironment.hpp"
#include "cinder/gl/Light.h"
#include "Message.hpp"
#include "Entity.hpp"


namespace ngs {

class Light : public Entity {
  Message& message_;
  const ci::JsonTree& params_;

  bool active_;

  ci::gl::Light light_;

  ci::Vec3f pos_;
  ci::Vec3f offset_;
  ci::Vec3f target_pos_;


public:
  explicit Light(Message& message, ci::JsonTree& params) :
    message_(message),
    params_(params),
    light_(ci::gl::Light::POINT, 0),
    active_(true)
  {}

  void setup(boost::shared_ptr<Light> obj_sp) {
    pos_    = Json::getVec3<float>(params_["light.pos"]);
    offset_ = pos_;
    light_.setPosition(pos_);

    light_.setAttenuation(params_["light.ConstantAttenuation"].getValue<float>(),
                          params_["light.LinearAttenuation"].getValue<float>(),
                          params_["light.QuadraticAttenuation"].getValue<float>());

    light_.setDiffuse(Json::getColor<float>(params_["light.Diffuse"]));
    light_.setAmbient(Json::getColor<float>(params_["light.Ambient"]));
    light_.setSpecular(Json::getColor<float>(params_["light.Specular"]));

    message_.connect(Msg::UPDATE, obj_sp, &Light::update);
    message_.connect(Msg::STAGE_POS, obj_sp, &Light::stagePos);
    message_.connect(Msg::LIGHT_ENABLE, obj_sp, &Light::enable);
    message_.connect(Msg::LIGHT_DISABLE, obj_sp, &Light::disable);
  }


private:
  bool isActive() const override { return active_; }


  void update(const Message::Connection& connection, Param& param) {
    pos_.x = pos_.x + (target_pos_.x - pos_.x) * 0.1f;
    pos_.z = pos_.z + (target_pos_.z - pos_.z) * 0.1f;
    light_.setPosition(pos_);
  }

  void stagePos(const Message::Connection& connection, Param& param) {
    target_pos_ = boost::any_cast<const ci::Vec3f&>(param["stage_pos"]) + offset_;
  }

  
  void enable(const Message::Connection& connection, Param& param) {
    light_.enable();
  }

  void disable(const Message::Connection& connection, Param& param) {
    light_.disable();
  }
  
};

}
