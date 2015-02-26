#pragma once

//
// 光源
//

#include "GameEnvironment.hpp"
#include "cinder/gl/Light.h"
#include "Message.hpp"


namespace ngs {

class Light {
  Message& message_;
  Message::ConnectionHolder connection_holder_;

  ci::gl::Light light_;

  ci::Vec3f pos_;
  ci::Vec3f offset_;
  ci::Vec3f target_pos_;


public:
  Light(Message& message, const ci::JsonTree& params) :
    message_(message),
    light_(ci::gl::Light::POINT, 0),
    pos_(Json::getVec3<float>(params["light.pos"])),
    offset_(pos_)
  {
    light_.setPosition(pos_);

    light_.setAttenuation(params["light.ConstantAttenuation"].getValue<float>(),
                          params["light.LinearAttenuation"].getValue<float>(),
                          params["light.QuadraticAttenuation"].getValue<float>());

    light_.setDiffuse(Json::getColor<float>(params["light.Diffuse"]));
    light_.setAmbient(Json::getColor<float>(params["light.Ambient"]));
    light_.setSpecular(Json::getColor<float>(params["light.Specular"]));

    connection_holder_.add(message.connect(Msg::UPDATE, this, &Light::update));
    connection_holder_.add(message.connect(Msg::STAGE_POS, this, &Light::stagePos));
    connection_holder_.add(message.connect(Msg::LIGHT_ENABLE, this, &Light::enable));
    connection_holder_.add(message.connect(Msg::LIGHT_DISABLE, this, &Light::disable));
    
  }


private:
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
