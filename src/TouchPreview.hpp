#pragma once

//
// Touch位置を表示
// 

#include "Entity.hpp"


namespace ngs {

class TouchPreview : public Entity {
  Message& message_;
  bool active_;

  std::vector<Touch> touches_;
  bool display_;


  bool isActive() const override { return active_; }


public:
  explicit TouchPreview(Message& message) :
    message_(message),
    active_(true),
    display_(false)
  { }

  
  // FIXME:コンストラクタではshared_ptrが決まっていないための措置
  void setup(boost::shared_ptr<TouchPreview> obj_sp,
             const ci::JsonTree& params) {
    
    message_.connect(Msg::TOUCH_BEGAN, obj_sp, &TouchPreview::touchBegan);
    message_.connect(Msg::TOUCH_MOVED, obj_sp, &TouchPreview::touchMoved);
    message_.connect(Msg::TOUCH_ENDED, obj_sp, &TouchPreview::touchEnded);

    message_.connect(Msg::TOUCHPREVIEW_TOGGLE, obj_sp, &TouchPreview::display);
    
    message_.connect(Msg::DRAW_2D, obj_sp, &TouchPreview::draw);
  }


private:
  void touchBegan(const Message::Connection& connection, Param& params) {
    const auto* touches = boost::any_cast<const std::vector<Touch>* >(params.at("touch"));
    for (const auto& touch : *touches) {
      if (std::find(std::begin(touches_), std::end(touches_), touch.id) != std::end(touches_)) {
        DOUT << "touch already began." << std::endl;
      }
      else {
        touches_.push_back(touch);
      }
    }
  }

  void touchMoved(const Message::Connection& connection, Param& params) {
    const auto* touches = boost::any_cast<const std::vector<Touch>* >(params.at("touch"));
    for (const auto& touch : *touches) {
      auto it = std::find(std::begin(touches_), std::end(touches_), touch.id);
      if (it == std::end(touches_)) {
        DOUT << "touch dose not began." << std::endl;
      }
      else {
        *it = touch;
      }
    }
  }

  void touchEnded(const Message::Connection& connection, Param& params) {
    const auto* touches = boost::any_cast<const std::vector<Touch>* >(params.at("touch"));
    for (const auto& touch : *touches) {
      auto it = std::find(std::begin(touches_), std::end(touches_), touch.id);
      if (it == std::end(touches_)) {
        DOUT << "touch does not began." << std::endl;
      }

      // erase-remove idiom
      touches_.erase(std::remove(std::begin(touches_), std::end(touches_), touch.id),
                     std::end(touches_));
    }
  }

  
  void display(const Message::Connection& connection, Param& params) {
    display_ = !display_;
  }

  
  
  void draw(const Message::Connection& connection, Param& params) {
    if (!display_) return;
    
    ci::gl::color(0, 0, 1);
    
    for (const auto& touch : touches_) {
      ci::gl::drawSolidEllipse(ci::Vec2f(touch.pos),
                               10.0, 10.0);
    }
  }
  
};

}
