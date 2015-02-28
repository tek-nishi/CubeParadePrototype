//
// Cube Parade プロトタイプ
// 

#include "Defines.hpp"
#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/Json.h"
#include "cinder/System.h"
#include "Touch.hpp"
#include "Game.hpp"


using namespace ci;
using namespace ci::app;


namespace ngs {

class CubeParadePrototypeApp : public AppNative {

  // TIPS:継承元のAppNativeでpublicになっているメンバ関数は
  //      privateになっていて構わない
  
  void prepareSettings(Settings *settings) override {
    settings->setWindowSize(Frame::WIDTH, Frame::HEIGHT);
    settings->setTitle(PREPRO_TO_STR(PRODUCT_NAME));

#if (TARGET_OS_MAC) && !(TARGET_OS_IPHONE)
    // FIXME:OSXでフレームレートが不安定になる現象のworkaround
    // settings->setFrameRate(120.0);
#endif
    
    settings->enableMultiTouch();

    DOUT << "MultiTouch:" << ci::System::hasMultiTouch()
         << " touch num:" << ci::System::getMaxMultiTouchPoints()
         << std::endl;
  }
  
	void setup() override {
#if (TARGET_OS_MAC) && !(TARGET_OS_IPHONE)
    // OSXでタイトルバーにアプリ名を表示するworkaround
    getWindow()->setTitle(PREPRO_TO_STR(PRODUCT_NAME));
#endif

#if (TARGET_OS_IPHONE)
    // 縦横画面両対応
    getSignalSupportedOrientations().connect([](){ return ci::app::InterfaceOrientation::All; });
#endif
    
    params_ = JsonTree(loadAsset("params.json"));
    game_   = std::unique_ptr<Game>(new Game(params_));
    paused_ = false;
                            
    elapsed_time_ = getElapsedSeconds();
  }


  // FIXME:Windowsではtouchイベントとmouseイベントが同時に呼ばれる
	void mouseDown(MouseEvent event) override {
    if (!event.isLeft()) return;

    mouse_pos_ = event.getPos();
    mouse_prev_pos_ = mouse_pos_;

    std::vector<Touch> t = {
      { true,
        false,
        getElapsedSeconds(),
        Touch::MOUSE_EVENT_ID,
        mouse_pos_, mouse_prev_pos_ }
    };

    game_->touchesBegan(t);
  }

	void mouseDrag(MouseEvent event) override {
    if (!event.isLeftDown()) return;

    mouse_prev_pos_ = mouse_pos_;
    mouse_pos_ = event.getPos();

    std::vector<Touch> t = {
      { true,
        false,
        getElapsedSeconds(),
        Touch::MOUSE_EVENT_ID,
        mouse_pos_, mouse_prev_pos_ }
    };

    game_->touchesMoved(t);
  }

	void mouseUp(MouseEvent event) override {
    if (!event.isLeft()) return;

    mouse_prev_pos_ = mouse_pos_;
    mouse_pos_ = event.getPos();

    std::vector<Touch> t = {
      { true,
        false,
        getElapsedSeconds(),
        Touch::MOUSE_EVENT_ID,
        mouse_pos_, mouse_prev_pos_ }
    };

    game_->touchesEnded(t);
  }


  // OSX版はTrackPadの領域をWindowにマップした座標になっている
  void touchesBegan(TouchEvent event) override {
    auto touches = createTouchInfo(event);
    game_->touchesBegan(touches);
  }

  void touchesMoved(TouchEvent event) override {
    auto touches = createTouchInfo(event);
    game_->touchesMoved(touches);
  }

  void touchesEnded(TouchEvent event) override {
    auto touches = createTouchInfo(event);
    game_->touchesEnded(touches);
  }
  
  
  void keyDown(KeyEvent event) override {
    int keycode   = event.getCode();
    int charactor = event.getChar();

    game_->keyDown(keycode, charactor);

    if (keycode == ci::app::KeyEvent::KEY_ESCAPE) {
      DOUT << "Pause Game." << std::endl;

      paused_ = !paused_;
      game_->pause(paused_);
    }

    if (charactor == 'R') {
      DOUT << "Reset Game." << std::endl;

      // TIPS:[先に解放]リソースを二重に使うのを回避
      game_.reset();
      game_ = std::unique_ptr<Game>(new Game(params_));
      // 初回起動時に合わせ、resizeを実行
      game_->resize();
      // Pause状態を維持
      game_->pause(paused_);
    }
  }
  

  void resize() override {
    game_->resize();
  }
  
  
	void update() override {
    double current_time = getElapsedSeconds();
    game_->update(current_time - elapsed_time_);

    // DOUT << current_time - elapsed_time_ << std::endl;
    
    elapsed_time_ = current_time;
  }

	void draw() override {
    // clear out the window with black
    gl::clear(Color(0.2, 0.2, 0.2));

    game_->draw();
  }


  ci::JsonTree params_;
  std::unique_ptr<Game> game_;
  bool paused_;

  double elapsed_time_;

  ci::Vec2f mouse_pos_;
  ci::Vec2f mouse_prev_pos_;

  
  static std::vector<ngs::Touch> createTouchInfo(const TouchEvent& event) {
    std::vector<ngs::Touch> touches;
    
    const auto& event_touches = event.getTouches();
    for (const auto& touch : event_touches) {
      ngs::Touch t = {
// OSごとのtouch入力の違いを吸収するworkaround
#if (TARGET_OS_IPHONE)
        true,
#else
        false,
#endif
        false,
        
        touch.getTime(),
        touch.getId(),
        touch.getPos(), touch.getPrevPos()
      };
      touches.push_back(std::move(t));
    }
    
    return std::move(touches);
  }
  
};

}


enum  { AA_TYPE = 
#if (TARGET_OS_MAC) && !(TARGET_OS_IPHONE)
  ci::app::RendererGl::AA_MSAA_16
#elif (TARGET_OS_IPHONE)
  // TODO:iOSの機種ごとにAAのtypeを変える
  ci::app::RendererGl::AA_NONE
#elif defined(_MSC_VER)
  ci::app::RendererGl::AA_MSAA_16
#else
  ci::app::RendererGl::AA_MSAA_16
#endif
};
        
CINDER_APP_NATIVE(ngs::CubeParadePrototypeApp, RendererGl(AA_TYPE))
