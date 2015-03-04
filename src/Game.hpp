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
#include "JsonUtil.hpp"
#include "Camera.hpp"
#include "Sound.hpp"
#include "EntityFactory.hpp"
#include "TimerTask.hpp"


namespace ngs {

class Game {
  ci::JsonTree& params_;
  ci::Rand random_;
  
  Message message_;

  EntityHolder  entity_holder_;
  EntityFactory factory_;
  
  Camera camera_;

  // Sound sound_;

  TimerTask<double> timer_tasks_;

  bool pause_;


public:
  explicit Game(ci::JsonTree& params) :
    params_(params),
    factory_(message_, params, entity_holder_),
    camera_(message_, params),
    // sound_(message_, params),
    pause_(false)
  {
    message_.connect(Msg::CUBE_PLAYER_DEAD, this, &Game::restartStage);
    message_.connect(Msg::ALL_STAGE_CLEAR, this, &Game::restartStage);
    
    setup();
  }

  void resize() {
    camera_.resize();
  }

  
  void touchesBegan(std::vector<Touch>& touches) {
    signalTouchMessage(Msg::TOUCH_BEGAN, touches);
  }
  
  void touchesMoved(std::vector<Touch>& touches) {
    signalTouchMessage(Msg::TOUCH_MOVED, touches);
  }
  
  void touchesEnded(std::vector<Touch>& touches) {
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
  }

  void keyUp(const int keycode, const int charactor) {
  }
  
  
  void update(const double delta_time) {
    if (pause_) return;

    timer_tasks_(delta_time);
    
    // FIXME:VS2013 update1はboost::anyの初期化リストにコンテナの右辺値を入れると
    //       実行時エラーになる??
    auto player_info = std::vector<PlayerInfo>();
    Param params = {
      { "deltaTime", delta_time },
      { "frustum", ci::Frustumf(camera_.body()) },
      { "camera", &camera_ },
      { "playerInfo", player_info },
      { "stageWidth", 0.0f },
      { "stageLength", 0.0f },
      { "stageBottomZ", 0.0f },
    };
    
    message_.signal(Msg::GATHER_INFORMATION, params);
    message_.signal(Msg::UPDATE, params);

    // 他のすべてが更新されてから更新したいもの(カメラとか)
    message_.signal(Msg::POST_UPDATE, params);

    entity_holder_.eraseInactiveEntity();
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

    // FIXME: ci::gl::colorで色を決める
    //        ci::gl::Materialを使わない
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

  
  void signalTouchMessage(const int msg, std::vector<Touch>& touches) {
    Param params = {
      { "touch",  &touches },
      { "camera", &camera_ },
      { "handled", false }
    };
    message_.signal(msg, params);
  }


  void restartStage(const Message::Connection& connection, Param& params) {
    timer_tasks_.add(3.0, [this]() {
        message_.signal(Msg::RESET_STAGE, Param());
        // この場でentityを破棄
        entity_holder_.eraseInactiveEntity();
        setup();
      });
  }

  void setup() {
    message_.signal(Msg::SETUP_GAME, Param());
    // FIXME:Stageからstartとfinish位置をpostするため、
    //       gameに必要な準備を終えてからStageを生成する
    message_.signal(Msg::SETUP_STAGE, Param());

    // 最後にPlayerの生成
    for (const auto& pos : params_["game.entry"]) {
      Param params = {
        { "entry_pos", Json::getVec3<int>(pos) },
        { "paused", false },
      };
      message_.signal(Msg::CREATE_CUBEPLAYER, params);
    }
  }
  
};

}
