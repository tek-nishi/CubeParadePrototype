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

  enum {
    MOVE_NONE = -1,

    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT
  };

  bool  now_rotation_;
  int   move_direction_;
  u_int move_speed_;

  ci::Quatf move_rotate_;
  ci::Quatf move_rotate_start_;
  ci::Quatf move_rotate_end_;
  ci::Vec3f rotate_pivpot_;

  float move_rotate_time_;
  float move_rotate_time_end_;

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

    move_rotate_time_end_ = params_["cubeEnemy.moveRotateTime"].getValue<float>();

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

    if (now_rotation_) {
      move_rotate_time_ += delta_time;
      if (move_rotate_time_ >= move_rotate_time_end_) {
        now_rotation_ = false;
        // 表示用情報をここで更新
        rot_ = move_rotate_end_ * rot_;
        pos_ = ci::Vec3f(pos_block_) * size_;

#if 0
        {
          Param params = {
            { "block_pos", pos_block_ },
          };
          message_.signal(Msg::CUBE_PLAYER_POS, params);
        }
#endif
      }
      else {
        move_rotate_ = move_rotate_start_.slerp(move_rotate_time_ / move_rotate_time_end_,
                                                move_rotate_end_);
      }
    }
    else {
      if (ci::randFloat() < 0.01f) {
        int directions[] = { MOVE_UP, MOVE_DOWN, MOVE_LEFT, MOVE_RIGHT };
        
        move_direction_ = directions[ci::randInt(elemsof(directions))];
        
        auto& information = boost::any_cast<std::vector<CubeInfo>& >(params["playerInfo"]);
        if (startRotationMove(information)) {
          updateInformation(pos_block_, information);
        }
      }

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
    if (now_rotation_) {
      ci::gl::translate(-rotate_pivpot_);
      ci::gl::rotate(move_rotate_);
      ci::gl::translate(rotate_pivpot_);
    }
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

  
  bool startRotationMove(const std::vector<CubeInfo>& information) {
    ci::Quatf rotate_table[] = {
      ci::Quatf(ci::Vec3f(1, 0, 0),  M_PI / 2),
      ci::Quatf(ci::Vec3f(1, 0, 0), -M_PI / 2),
      ci::Quatf(ci::Vec3f(0, 0, 1), -M_PI / 2),
      ci::Quatf(ci::Vec3f(0, 0, 1),  M_PI / 2)
    };

    ci::Vec3f pivot_table[] = {
      ci::Vec3f(         0, size_ / 2, -size_ / 2),
      ci::Vec3f(         0, size_ / 2,  size_ / 2),
      ci::Vec3f(-size_ / 2, size_ / 2,          0),
      ci::Vec3f( size_ / 2, size_ / 2,          0)
    };
      
    ci::Vec3i move_table[] = {
      ci::Vec3i( 0, 0,  1),
      ci::Vec3i( 0, 0, -1),
      ci::Vec3i( 1, 0,  0),
      ci::Vec3i(-1, 0,  0),
    };

    {
      // 移動可能か調べる
      Param params = {
        { "block_pos", pos_block_ + move_table[move_direction_] },
      };
      message_.signal(Msg::CUBE_STAGE_HEIGHT, params);

      if (!boost::any_cast<bool>(params["is_cube"])) return false;
      const auto& pos = boost::any_cast<const ci::Vec3i&>(params["height"]);
      if (pos.y > pos_block_.y) return false;
    }

    if (searchOtherPlayer(pos_block_ + move_table[move_direction_], information)) {
      return false;
    }

    now_rotation_      = true;
    move_rotate_time_  = 0.0;
    move_rotate_start_ = ci::Quatf::identity();
    move_rotate_end_   = rotate_table[move_direction_];
    move_rotate_       = move_rotate_start_;
    rotate_pivpot_     = pivot_table[move_direction_];

    pos_block_ += move_table[move_direction_];

    return true;
  }

  bool searchOtherPlayer(const ci::Vec3i block_pos, const std::vector<CubeInfo>& information) {
    for (auto& info : information) {
      if (info.id == id_) continue;

      // 高さ判定はしない
      if ((info.block_pos.x == block_pos.x)
          && (info.block_pos.z == block_pos.z)) {
        return true;
      }
    }
    return false;
  }
  
  void updateInformation(const ci::Vec3i block_pos, std::vector<CubeInfo>& information) {
    for (auto& info : information) {
      if (info.id != id_) continue;

      info.block_pos = block_pos;
      break;
    }
  }
  
};

}
