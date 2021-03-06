﻿#pragma once

//
// ステージを構成する立方体
//

#include "GameEnvironment.hpp"
#include "cinder/gl/gl.h"
#include "cinder/Vector.h"


namespace ngs {

class StageCube {
  ci::Vec3i pos_block_;
  ci::Vec3f pos_;
  float size_;
  ci::Color color_;

  bool active_;

  bool on_entity_;
  int entity_type_;


public:
  enum {
    ON_PLAYER,
    ON_ITEM,
    ON_ENEMY
  };
  

  StageCube(const ci::Vec3i& pos_block, const float size,
            const bool active,
            const ci::Color& color = ci::Color::white()) :
    pos_block_(pos_block),
    pos_(ci::Vec3f(pos_block) * size),
    size_(size),
    color_(color),
    active_(active),
    on_entity_(false)
  { }

  
  void draw() const {
    if (!active_) return;
    
    // ci::gl::color(on_entity_ ? ci::Color(0.0f, 0.0f, 1.0f) + color_ * 0.5f : color_);
    ci::gl::color(color_);

    ci::Vec3f pos(pos_);
    // 上平面が(y = 0)
    pos.y -= size_ / 2;
    
    ci::gl::drawCube(pos, ci::Vec3f(size_, size_, size_));
  }


  const ci::Vec3i& posBlock() const { return pos_block_; }

  void posBlock(const ci::Vec3i& pos) {
    pos_block_ = pos;
    pos_ = ci::Vec3f(pos_block_) * size_;
  }
  
  const ci::Vec3f& pos() const { return pos_; }
  float size() const { return size_; }
  const ci::Color& color() const { return color_; }

  bool isOnEntity() const { return on_entity_; }
  void onEntity(const bool entity) { on_entity_ = entity; }

  int entityType() const { return entity_type_; }
  void entityType(const int entity_type) { entity_type_ = entity_type; }

  bool isActive() const { return active_; }
  void active(const bool value) { active_ = value; }

  
};
  
}
