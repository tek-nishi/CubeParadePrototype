#pragma once

//
// お邪魔Cube
//

#include "Message.hpp"
#include "Entity.hpp"


namespace ngs {

class CubeEnemy : public Entity {
  Message& message_;
  const ci::JsonTree& params_;

  bool active_;

  u_int id_;

  ci::Vec3i pos_block_;
  ci::Vec3f pos_;
  ci::Quatf rot_;

  bool  now_rotation_;

  float size_;
  ci::Color color_;



public:
  explicit CubeEnemy(Message& message, ci::JsonTree& params) :
    message_(message),
    params_(params),
    active_(true),
    id_(getUniqueNumber()),
    now_rotation_(false)
  { }

  // FIXME:コンストラクタではshared_ptrが決まっていないための措置
  void setup(boost::shared_ptr<CubeEnemy> obj_sp,
             const ci::Vec3i& entry_pos_block) {

    rot_       = ci::Quatf::identity();
    size_      = params_["cube.size"].getValue<float>();
    pos_block_ = entry_pos_block;
    pos_       = ci::Vec3f(entry_pos_block) * size_;
    color_     = Json::getColor<float>(params_["cubeEnemy.color"]);

    message_.connect(Msg::UPDATE, obj_sp, &CubeEnemy::update);
    message_.connect(Msg::DRAW, obj_sp, &CubeEnemy::draw);

    message_.connect(Msg::RESET_STAGE, obj_sp, &CubeEnemy::inactive);
    message_.connect(Msg::GATHER_INFORMATION, obj_sp, &CubeEnemy::gatherInfo);
  }


private:
  bool isActive() const override { return active_; }

  
  void inactive(const Message::Connection& connection, Param& params) {
    active_ = false;
  }

  void update(const Message::Connection& connection, Param& params) {
    double delta_time = boost::any_cast<double>(params.at("deltaTime"));

    {
      Param params = {
        { "block_pos", pos_block_ },
      };
      message_.signal(Msg::CUBE_STAGE_HEIGHT, params);

      if (!boost::any_cast<bool>(params["is_cube"])) {
        // stageから落下
        Param params = {
          { "entry_pos", ci::Vec3i(pos_block_.x, pos_block_.y + 1, pos_block_.z) },
          { "color", color_ },
          { "speed", 1.0f },
        };
        
        message_.signal(Msg::CREATE_FALLCUBE, params);
          
        active_ = false;
        return;
      }
    }
  }


  void draw(const Message::Connection& connection, Param& params) {
    ci::gl::color(color_);

    // 下の平面が(y = 0)
    ci::Vec3f pos(pos_);
    pos.y += size_ / 2;

    ci::gl::pushModelView();

    ci::gl::translate(pos);
    // if (now_rotation_) {
    //   ci::gl::translate(-rotate_pivpot_);
    //   ci::gl::rotate(move_rotate_);
    //   ci::gl::translate(rotate_pivpot_);
    // }
    ci::gl::rotate(rot_);
    
    ci::gl::drawCube(ci::Vec3f::zero(), ci::Vec3f(size_, size_, size_));
    
    ci::gl::popModelView();
  }

  
  void gatherInfo(const Message::Connection& connection, Param& params) {
    auto& informations = boost::any_cast<std::vector<CubeInfo>& >(params["playerInfo"]);

    CubeInfo info = {
      id_,
      false,
      pos_block_,
      pos_,
      now_rotation_
    };
    
    informations.push_back(std::move(info));
  }
};

}
