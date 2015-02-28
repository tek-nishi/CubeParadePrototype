#pragma once

//
// ゲーム本編
//

#include "GameEnvironment.hpp"
#include "cinder/Json.h"
#include "cinder/Rand.h"
#include "cinder/Camera.h"
#include "cinder/Frustum.h"
#include "cinder/gl/Light.h"
#include "Entity.hpp"
#include "Message.hpp"
#include "Stage.hpp"
#include "JsonUtil.hpp"
#include "Camera.hpp"
#include "Light.hpp"
#include "Sound.hpp"
#include "EntityFactory.hpp"


namespace ngs {

class Game {

public:
  explicit Game(ci::JsonTree& params) :
    params_(params),
    factory_(message_, params, entity_holder_),
    camera_(message_, params),
    stage_(message_, params, random_),
    light_(message_, params),
    pause_(false)
    // sound_(message_, params)
  {
    message_.signal(Msg::SETUP_GAME, Param());
    {
      Param params = {
        { "entry_pos", Json::getVec3<int>(params_["game.entry"]) },
      };
      message_.signal(Msg::CREATE_CUBEPLAYER, params);
    }


#if 0
    {
      Param params = {
        { "name", std::string("sample_1") }
      };
      message_.signal(Msg::SOUND_PLAY, params);
    }
#endif
  }

  void resize() {
    camera_.resize();
  }

  
  void touchesBegan(const std::vector<Touch>& touches) {
    signalTouchMessage(Msg::TOUCH_BEGAN, touches);
  }
  
  void touchesMoved(const std::vector<Touch>& touches) {
    signalTouchMessage(Msg::TOUCH_MOVED, touches);
  }
  
  void touchesEnded(const std::vector<Touch>& touches) {
    signalTouchMessage(Msg::TOUCH_ENDED, touches);
  }


  void keyDown(const int keycode, const int charactor) {
    Param params = {
      { "keycode", keycode }
    };
    
    message_.signal(Msg::KEY_DOWN, params);

    if (charactor == 'T') {
      message_.signal(Msg::TOUCHPREVIEW_TOGGLE, Param());
    }

#if 0
    if (keycode == ci::app::KeyEvent::KEY_a) {
      Param params = {
        { "name", std::string("sample_1") }
      };
      message_.signal(Msg::SOUND_PLAY, params);
    }
    else if (keycode == ci::app::KeyEvent::KEY_s) {
      Param params = {
        { "name", std::string("sample_2") }
      };
      message_.signal(Msg::SOUND_PLAY, params);
    }
    else if (keycode == ci::app::KeyEvent::KEY_d) {
      Param params = {
        { "name", std::string("sample_3") }
      };
      message_.signal(Msg::SOUND_PLAY, params);
    }
    else if (keycode == ci::app::KeyEvent::KEY_f) {
      Param params = {
        { "category", std::string("se") }
      };
      message_.signal(Msg::SOUND_STOP, params);
    }
    else if (keycode == ci::app::KeyEvent::KEY_g) {
      Param params;
      message_.signal(Msg::SOUND_STOP, params);
    }
#endif
  }
  
  
  void update(const double delta_time) {
    if (pause_) return;
    
    Param params = {
      { "deltaTime", delta_time },
      { "frustum", ci::Frustumf(camera_.body()) },
      { "camera", &camera_ },
    };
    message_.signal(Msg::UPDATE, params);

    // 他のすべてが更新されてから更新したいもの(カメラとか)
    message_.signal(Msg::POST_UPDATE, params);

    entity_holder_.update();
  }

  void draw() {
    // 3D向け描画
    ci::gl::pushMatrices();
    ci::gl::setMatrices(camera_.body());

    ci::gl::enable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    ci::gl::enable(GL_LIGHTING);
    ci::gl::enable(GL_NORMALIZE);
    
    message_.signal(Msg::LIGHT_ENABLE, Param());

    glEnable(GL_COLOR_MATERIAL);
#if !(TARGET_OS_IPHONE)
    // OpenGL ESは未対応
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
#endif
    
    ci::gl::enableDepthRead();
    ci::gl::enableDepthWrite();
    

    message_.signal(Msg::DRAW, Param());

    message_.signal(Msg::LIGHT_DISABLE, Param());
    ci::gl::disable(GL_LIGHTING);
    
    ci::gl::popMatrices();

    // 2D向け描画
    ci::gl::disableDepthRead();
    ci::gl::disableDepthWrite();

    ci::gl::disable(GL_CULL_FACE);
    message_.signal(Msg::DRAW_2D, Param());
  }

  
  bool isPause() const { return pause_; }
  void pause(const bool pause) { pause_ = pause; }

  
private:
  // TIPS:コピー不可
  Game(const Game&) = delete;
  Game& operator=(const Game&) = delete;

  ci::JsonTree& params_;
  ci::Rand random_;
  
  Message message_;

  EntityHolder entity_holder_;
  EntityFactory factory_;
  
  Camera camera_;
  Stage stage_;
  Light light_;

  bool pause_;

  // Sound sound_;


  
  void signalTouchMessage(const int msg, const std::vector<Touch>& touches) {
    Param params = {
      { "touch",  &touches },
      { "camera", &camera_ },
      { "handled", false }
    };
    message_.signal(msg, params);
  }
  
};

}
