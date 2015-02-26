#pragma once

//
// プレイヤー操作のCube
//

#include "cinder/Vector.h"
#include "cinder/AxisAlignedBox.h"
#include "Message.hpp"
#include "Camera.hpp"
#include "Entity.hpp"


namespace ngs {

class CubePlayer : public Entity {

public:
  explicit CubePlayer(Message& message) :
    message_(message),
    active_(true),
    picking_(false),
    now_rotation_(false)
  { }

  // FIXME:コンストラクタではshared_ptrが決まっていないための措置
  void setup(boost::shared_ptr<CubePlayer> obj_sp,
             const ci::JsonTree& params, const ci::Vec3i& entry_pos_block) {

    rot_       = ci::Quatf::identity();
    size_      = params["cube.size"].getValue<float>();
    pos_block_ = entry_pos_block;
    pos_       = ci::Vec3f(entry_pos_block) * size_;
    color_     = Json::getColor<float>(params["cubePlayer.color"]);
    
    move_rotate_time_end_max_ = params["cubePlayer.moveRotateTime"].getValue<float>();
    
    move_threshold_ = params["cubePlayer.moveThreshold"].getValue<float>();
    speed_rate_     = params["cubePlayer.speedRate"].getValue<float>();
    max_move_speed_ = params["cubePlayer.maxMoveSpeed"].getValue<int>();
    speed_table_ = Json::getArray<float>(params["cubePlayer.moveSpeed"]);
    
    // 必要なメッセージを受け取るように指示
    // TIPS:オブジェクトが消滅すると自動的に解除される
    message_.connect(Msg::UPDATE, obj_sp, &CubePlayer::update);
    message_.connect(Msg::DRAW, obj_sp, &CubePlayer::draw);
    
    message_.connect(Msg::CAMERAVIEW_INFO, obj_sp, &CubePlayer::info);

    message_.connect(Msg::TOUCH_BEGAN, obj_sp, &CubePlayer::touchesBegan);
    message_.connect(Msg::TOUCH_MOVED, obj_sp, &CubePlayer::touchesMoved);
    message_.connect(Msg::TOUCH_ENDED, obj_sp, &CubePlayer::touchesEnded);

    message_.connect(Msg::KEY_DOWN, obj_sp, &CubePlayer::keyDown);
  }
  
  
private:
  bool isActive() const override { return active_; }

  
  ci::AxisAlignedBox3f getBoundingBox() {
    return ci::AxisAlignedBox3f(ci::Vec3f(-size_ / 2,     0, -size_ / 2) + pos_,
                                ci::Vec3f( size_ / 2, size_,  size_ / 2) + pos_);
  }


  // TIPS:CubePlayer::update が private で Entity::update は public という定義が可能
  void update(const Message::Connection& connection, Param& params) {
    double delta_time = boost::any_cast<double>(params.at("deltaTime"));
    
    if (now_rotation_) {
      move_rotate_time_ += delta_time;
      if (move_rotate_time_ >= move_rotate_time_end_) {
        now_rotation_ = false;
        // 表示用情報をここで更新
        rot_ = move_rotate_end_ * rot_;
        pos_ = ci::Vec3f(pos_block_) * size_;

        {
          Param params = {
            { "block_pos", pos_block_ },
          };
          message_.signal(Msg::CUBE_PLAYER_POS, params);
        }

        // まだ移動するか判定
        continueRotationMove();
      }
      else {
        move_rotate_ = move_rotate_start_.slerp(move_rotate_time_ / move_rotate_time_end_,
                                                move_rotate_end_);
      }
    }
    else {
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
          message_.signal(Msg::CUBE_PLAYER_DEAD, params);
          
          active_ = false;
          return;
        }
      }
    }
  }

  
  void draw(const Message::Connection& connection, Param& params) {
    // ci::gl::color(pick_id_.empty() ? color_ : ci::Color(1, 0, 0));
    ci::gl::color(picking_ ? ci::Color(1, 0, 0) : color_);

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

  
  void info(const Message::Connection& connection, Param& params) {
    params["player_active"]    = true;
    params["player_block_pos"] = pos_block_;
    params["player_pos"]       = pos_;
    params["player_rotation"]  = now_rotation_;
  }

  
  bool isBodyPicked(const Touch& touch, Camera& camera) {
    auto ray = camera.generateRay(touch.pos);

    float z_cross[2];
    int num = getBoundingBox().intersect(ray, z_cross);
    return num > 0;
  }


  void touchesBegan(const Message::Connection& connection, Param& params) {
    if (picking_) return;
    
    const auto* touches = boost::any_cast<const std::vector<Touch>* >(params.at("touch"));
    if (touches->size() >= 2) {
      // 同時タッチは無視
      return;
    }

    const auto& touch = (*touches)[0];
    auto* camera = boost::any_cast<Camera* >(params.at("camera"));
    if (touch.prior && isBodyPicked(touch, *camera)) {
      // pick開始
      picking_ = true;
      picking_id_ = touch.id;
      picking_timestamp_ = touch.timestamp;
      picking_timestamp_record_ = false;

      // cubeの上平面との交点
      auto ray = camera->generateRay(touch.pos);

      float cross_z;
      ray.calcPlaneIntersection(ci::Vec3f(0, (pos_block_.y + 1) * size_, 0), ci::Vec3f(0, 1, 0), &cross_z);
      picking_pos_ = ray.calcPosition(cross_z);
      picking_plane_ = ci::Vec3f(0, (pos_block_.y + 1) * size_, 0);
    }
  }

  void touchesMoved(const Message::Connection& connection, Param& params) {
    if (!picking_) return;

    const auto* touches = boost::any_cast<const std::vector<Touch>* >(params.at("touch"));
    for (const auto& touch : *touches) {
      if (touch.id != picking_id_) continue;

      // スワイプ操作は、タッチ開始からではなく、移動開始からの時間
      // で計測する
      if (!picking_timestamp_record_) {
        picking_timestamp_record_ = true;
        picking_timestamp_ = touch.timestamp;
      }
    }
  }
  
  void touchesEnded(const Message::Connection& connection, Param& params) {
    if (!picking_) return;
    
    const auto* touches = boost::any_cast<const std::vector<Touch>* >(params.at("touch"));
    for (const auto& touch : *touches) {
      if (touch.id != picking_id_) continue;

      // cubeの上平面との交点
      auto* camera = boost::any_cast<Camera* >(params.at("camera"));
      auto ray = camera->generateRay(touch.pos);

      float cross_z;
      ray.calcPlaneIntersection(ci::Vec3f(0, (pos_block_.y + 1) * size_, 0), ci::Vec3f(0, 1, 0), &cross_z);
      auto picking_ofs = ray.calcPosition(cross_z) - picking_pos_;

      int move_direction = MOVE_NONE;
      float distance = 0.0f;
      if (std::abs(picking_ofs.z) >= std::abs(picking_ofs.x)) {
        // 縦移動
        if (picking_ofs.z < (-size_ * move_threshold_)) {
          move_direction = MOVE_DOWN;
        }
        else if (picking_ofs.z > (size_ * move_threshold_)) {
          move_direction = MOVE_UP;
        }
        distance = std::abs(picking_ofs.z) - size_ * move_threshold_;
      }
      else {
        // 横移動
        if (picking_ofs.x < (-size_ * move_threshold_)) {
          move_direction = MOVE_RIGHT;
        }
        else if (picking_ofs.x > (size_ * move_threshold_)) {
          move_direction = MOVE_LEFT;
        }
        distance = std::abs(picking_ofs.x) - size_ * move_threshold_;
      }

      if (move_direction == MOVE_NONE) {
        // 移動なければ強制停止
        move_speed_ = 0;
      }
      else {
        assert(distance >= 0.0f);
          
        // 素早い操作で数コマ移動可能
        double touch_time = touch.timestamp - picking_timestamp_;
        // 小さすぎる値での除算を防ぐ
        if (touch_time < (1 / 60.0)) touch_time = 1 / 60.0;
        float speed = (distance / touch_time) * speed_rate_;

        DOUT << "speed:" << speed << std::endl;
          
        int move_speed = std::min(int(speed), max_move_speed_);

        if (now_rotation_) {
          // 移動中なら、向きとスピードだけ更新
          move_direction_ = move_direction;
          move_speed_     = move_speed + 1;
        }
        else {
          // 移動開始
          startRotationMove(move_direction, move_speed);
        }
      }
        
      picking_ = false;
      return;
    }
  }


  void keyDown(const Message::Connection& connection, Param& params) {
    // キー入力で４方向へ回転移動する
    // DEBUG用
    if (now_rotation_) return;
    
    int keycode = boost::any_cast<int>(params.at("keycode"));

    int  move_direction = MOVE_NONE;
    move_pos_block_ = pos_block_;
    switch(keycode) {
    case ci::app::KeyEvent::KEY_UP:
      {
        move_direction = MOVE_UP;
        move_pos_block_.z += 1;
      }
      break;

    case ci::app::KeyEvent::KEY_DOWN:
      {
        move_direction = MOVE_DOWN;
        move_pos_block_.z -= 1;
      }
      break;

    case ci::app::KeyEvent::KEY_LEFT:
      {
        move_direction = MOVE_LEFT;
        move_pos_block_.x += 1;
      }
      break;

    case ci::app::KeyEvent::KEY_RIGHT:
      {
        move_direction = MOVE_RIGHT;
        move_pos_block_.x -= 1;
      }
      break;
    }

    if (move_direction != MOVE_NONE) {
      startRotationMove(move_direction, 0);
    }
  }

  bool startRotationMove(const int direction, const int speed) {
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
        { "block_pos", pos_block_ + move_table[direction] },
      };
      message_.signal(Msg::CUBE_STAGE_HEIGHT, params);

      if (!boost::any_cast<bool>(params["is_cube"])) return false;
      const auto& pos = boost::any_cast<const ci::Vec3i&>(params["height"]);
      if (pos.y > pos_block_.y) return false;
    }

    move_direction_ = direction;
    move_speed_     = speed;

    now_rotation_      = true;
    move_rotate_time_  = 0.0;
    move_rotate_start_ = ci::Quatf::identity();
    move_rotate_end_   = rotate_table[direction];
    move_rotate_       = move_rotate_start_;
    rotate_pivpot_     = pivot_table[direction];

    pos_block_ += move_table[direction];

    // speedが速いと回転スピードも速い
    float speed_rate = (speed < speed_table_.size()) ? speed_table_[speed]
                                                     : speed_table_.back();

    move_rotate_time_end_ = move_rotate_time_end_max_ * speed_rate;
    
    return true;
  }

  bool continueRotationMove() {
    if (move_speed_ == 0) return false;
    move_speed_ -= 1;

    return startRotationMove(move_direction_, move_speed_);
  }
  

  Message& message_;
  bool active_;

  ci::Vec3i pos_block_;
  ci::Vec3f pos_;
  ci::Quatf rot_;

  float size_;
  ci::Color color_;
  ci::Vec3f speed_;

  bool   picking_;
  u_int  picking_id_;
  bool   picking_timestamp_record_;
  double picking_timestamp_;

  float move_threshold_;
  float speed_rate_;
  int   max_move_speed_;
  std::vector<float> speed_table_;

  ci::Vec3f picking_plane_;
  ci::Vec3f picking_pos_;
  ci::Vec3i move_pos_block_;

  
  enum {
    MOVE_NONE = -1,

    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT
  };

  bool now_rotation_;
  int  move_direction_;
  int  move_speed_;

  ci::Quatf move_rotate_;
  ci::Quatf move_rotate_start_;
  ci::Quatf move_rotate_end_;
  ci::Vec3f rotate_pivpot_;

  float move_rotate_time_;
  float move_rotate_time_end_;
  float move_rotate_time_end_max_;
  
};

}
