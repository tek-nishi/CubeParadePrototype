#pragma once

//
// ゲーム内カメラ
//

#include "cinder/params/Params.h"


namespace ngs {

class Camera {


public:
  Camera(Message& message, const ci::JsonTree& params) :
    message_(message),
    camera_(ci::app::getWindowWidth(), ci::app::getWindowHeight(), 
            params.getValueForKey<float>("camera.fov"),
            params.getValueForKey<float>("camera.nearZ"),
            params.getValueForKey<float>("camera.farZ")),
    eye_pos_(Json::getVec3<float>(params["camera.eyePos"])),
    interest_pos_(Json::getVec3<float>(params["camera.interestPos"])),
    target_eye_pos_(eye_pos_),
    target_interest_pos_(interest_pos_),
    fov_(params.getValueForKey<float>("camera.fov")),
    near_(params.getValueForKey<float>("camera.nearZ")),
    center_rate_(params.getValueForKey<float>("camera.centerRate")),
    bottom_rate_(params.getValueForKey<float>("camera.bottomRate")),
    ease_cube_stop_(params.getValueForKey<float>("camera.easeCubeStop")),
    ease_cube_move_(params.getValueForKey<float>("camera.easeCubeMove"))
  {
    camera_.setEyePoint(eye_pos_);
    camera_.setCenterOfInterestPoint(interest_pos_);
    
    connection_holder_.add(message.connect(Msg::POST_UPDATE, this, &Camera::update));
  }


  void resize() {
    DOUT << "resize()" << std::endl;
    
    float aspect = ci::app::getWindowAspectRatio();
    camera_.setAspectRatio(aspect);
    if (aspect < 1.0) {
      // 画面が縦長になったら、幅基準でfovを求める
      // fovとnear_zから投影面の幅の半分を求める
      float half_w = std::tan(ci::toRadians(fov_ / 2)) * near_;

      // 表示画面の縦横比から、投影面の高さの半分を求める
      float half_h = half_w / aspect;

      // 投影面の高さの半分とnear_zから、fovが求まる
      float fov = std::atan(half_h / near_) * 2;
      camera_.setFov(ci::toDegrees(fov));
    }
    else {
      // 横長の場合、fovは固定
      camera_.setFov(fov_);
    }
    DOUT << "camera fov:" << camera_.getFov()
         << " aspect ratio:" << aspect
         << " window size:" << ci::app::getWindowSize()
         << std::endl;
  }

  ci::CameraPersp& body() { return camera_; }


  ci::Ray generateRay(const ci::Vec2f& pos) {
    float u = pos.x / (float) ci::app::getWindowWidth();
    float v = pos.y / (float) ci::app::getWindowHeight();
    // because OpenGL and Cinder use a coordinate system
    // where (0, 0) is in the LOWERleft corner, we have to flip the v-coordinate
    return camera_.generateRay(u, 1.0f - v, camera_.getAspectRatio());
  }
  
  
private:
  Message& message_;
  Message::ConnectionHolder connection_holder_;

  ci::CameraPersp camera_;
  
  ci::Vec3f eye_pos_;
  ci::Vec3f interest_pos_;

  ci::Vec3f target_eye_pos_;
  ci::Vec3f target_interest_pos_;

  float fov_;
  float near_;

  float center_rate_;
  float bottom_rate_;

  float ease_cube_stop_;
  float ease_cube_move_;

  
  
  void update(const Message::Connection& connection, Param& param) {
    {
      Param params = {
        { "player_active", false },
      };
      message_.signal(Msg::CAMERAVIEW_INFO, params);

      float easing_rate = ease_cube_stop_;
      
      if (boost::any_cast<bool>(params["player_active"])) {
        const auto& player_pos = boost::any_cast<const ci::Vec3f& >(params["player_pos"]);

        // Stageの中心から左右への移動量 -> offset
        float stage_width  = boost::any_cast<float>(params["stage_width"]);
        float center_x = stage_width / 2;

        // Stageの下端からの移動量 -> offset
        float stage_length = boost::any_cast<float>(params["stage_length"]);
        float bottom_z     = boost::any_cast<float>(params["stage_bottom_z"]);

        ci::Vec3f pos = player_pos;
        pos.x = center_x + (player_pos.x - center_x) * center_rate_;
        pos.z = bottom_z + (player_pos.z - bottom_z) * bottom_rate_;
        
        target_eye_pos_      = eye_pos_ + pos;
        target_interest_pos_ = interest_pos_ + pos;

        if (boost::any_cast<bool>(params["player_rotation"])) easing_rate = ease_cube_move_;
      }

      // TODO:なめらか補完
      auto eye_pos = camera_.getEyePoint();
      camera_.setEyePoint(eye_pos + (target_eye_pos_ - eye_pos) * easing_rate);

      auto interest_pos = camera_.getCenterOfInterestPoint();
      camera_.setCenterOfInterestPoint(interest_pos + (target_interest_pos_ - interest_pos) * easing_rate);
    }
  }
  
};

}
