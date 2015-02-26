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
#include "StageWatcher.hpp"
#include "JsonUtil.hpp"
#include "CubePlayer.hpp"
#include "Camera.hpp"
#include "FallCube.hpp"
#include "EntryCube.hpp"
#include "Light.hpp"
#include "Sound.hpp"


namespace ngs {

class Game {

public:
  explicit Game(const ci::JsonTree& params) :
    params_(params),
    pause_(false),
    camera_(message_, params),
    stage_(message_, params, random_),
    light_(message_, params)
    // sound_(message_, params)
  {
    
    createAndAddEntity<CubePlayer>(params, Json::getVec3<int>(params["game.entry"]));
    createAndAddEntity<StageWatcher>(params["stage"]);

    message_.connect(Msg::CREATE_FALLCUBE, this, &Game::createFallcube);
    message_.connect(Msg::CREATE_ENTRYCUBE, this, &Game::createEntrycube);

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


  void keyDown(const int keycode) {
    Param params = {
      { "keycode", keycode }
    };
    
    message_.signal(Msg::KEY_DOWN, params);

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
    
    ci::gl::enableDepthRead(true);
    ci::gl::enableDepthWrite(true);
    

    Param params;
    message_.signal(Msg::DRAW, params);

    message_.signal(Msg::LIGHT_DISABLE, Param());
    ci::gl::disable(GL_LIGHTING);
    
    ci::gl::popMatrices();
  }

  
  bool isPause() const { return pause_; }
  void pause(const bool pause) { pause_ = pause; }

  
private:
  // TIPS:コピー不可
  Game(const Game&) = delete;
  Game& operator=(const Game&) = delete;

  const ci::JsonTree& params_;
  ci::Rand random_;

  bool pause_;
  
  Message message_;
  EntityHolder entity_holder_;
  
  Camera camera_;
  Stage stage_;
  Light light_;

  // Sound sound_;


  
  void signalTouchMessage(const int msg, const std::vector<Touch>& touches) {
    Param params = {
      { "touch",  &touches },
      { "camera", &camera_ },
      { "handled", false }
    };
    message_.signal(msg, params);
  }

  
  void createFallcube(const Message::Connection& connection, Param& params) {
    const auto& pos   = boost::any_cast<const ci::Vec3i& >(params["entry_pos"]);
    const auto& color = boost::any_cast<const ci::Color& >(params["color"]);
    const float speed = boost::any_cast<float>(params["speed"]);

    createAndAddEntity<FallCube>(params_, pos, speed, color);
  }

  void createEntrycube(const Message::Connection& connection, Param& params) {
    const auto& pos   = boost::any_cast<const ci::Vec3i& >(params["entry_pos"]);
    float offset_y = boost::any_cast<float>(params["offset_y"]);
    float active_time = boost::any_cast<float>(params["active_time"]);
    const auto& color = boost::any_cast<const ci::Color& >(params["color"]);

    createAndAddEntity<EntryCube>(params_, pos, offset_y, active_time, color);
  }

  
  
  // Entityを生成してHolderに追加
  // FIXME:可変長引数がconst参照になってたりしてる??
  template<typename T, typename... Args>
  void createAndAddEntity(Args&&... args) {
    // boost::shared_ptr<T> obj
    //   = boost::signals2::deconstruct<T>().postconstruct(boost::ref(message_),
    //                                                     std::forward<Args>(args)...);

    boost::shared_ptr<T> obj = boost::shared_ptr<T>(new T(message_));
    obj->setup(obj, std::forward<Args>(args)...);

    entity_holder_.add(obj);
  }
  
};

}
