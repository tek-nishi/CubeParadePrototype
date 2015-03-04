#pragma once

//
// プレイヤー操作のCube
//

#include "cinder/Vector.h"
#include "cinder/AxisAlignedBox.h"
#include "Message.hpp"
#include "Camera.hpp"
#include "Entity.hpp"
#include "Utility.hpp"


namespace ngs {

class CubePlayer : public Entity {
  Message& message_;
  const ci::JsonTree& params_;

  bool active_;

  u_int id_;
  
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

  bool  now_rotation_;
  bool  begin_rotation_;
  int   move_direction_;
  u_int move_speed_;

  ci::Quatf move_rotate_;
  ci::Quatf move_rotate_start_;
  ci::Quatf move_rotate_end_;
  ci::Vec3f rotate_pivpot_;

  float move_rotate_time_;
  float move_rotate_time_end_;
  float move_rotate_time_end_max_;

  
public:
  explicit CubePlayer(Message& message, ci::JsonTree& params) :
    message_(message),
    params_(params),
    active_(true),
    id_(getUniqueNumber()),
    picking_(false),
    now_rotation_(false),
    begin_rotation_(false)
  { }


  // FIXME:コンストラクタではshared_ptrが決まっていないための措置
  void setup(boost::shared_ptr<CubePlayer> obj_sp,
             const ci::Vec3i& entry_pos_block) {

    rot_       = ci::Quatf::identity();
    size_      = params_["cube.size"].getValue<float>();
    pos_block_ = entry_pos_block;
    pos_       = ci::Vec3f(entry_pos_block) * size_;
    color_     = Json::getColor<float>(params_["cubePlayer.color"]);
    
    move_rotate_time_end_max_ = params_["cubePlayer.moveRotateTime"].getValue<float>();
    
    move_threshold_ = params_["cubePlayer.moveThreshold"].getValue<float>();
    speed_rate_     = params_["cubePlayer.speedRate"].getValue<float>();
    max_move_speed_ = params_["cubePlayer.maxMoveSpeed"].getValue<int>();
    speed_table_ = Json::getArray<float>(params_["cubePlayer.moveSpeed"]);
    
    // 必要なメッセージを受け取るように指示
    // TIPS:オブジェクトが消滅すると自動的に解除される
    message_.connect(Msg::UPDATE, obj_sp, &CubePlayer::update);
    message_.connect(Msg::DRAW, obj_sp, &CubePlayer::draw);
    
    message_.connect(Msg::GATHER_INFORMATION, obj_sp, &CubePlayer::gatherInfo);
    message_.connect(Msg::CUBE_PLAYER_CHECK_FINISH, obj_sp, &CubePlayer::postPlayerZ);
    
    message_.connect(Msg::RESET_STAGE, obj_sp, &CubePlayer::inactive);

    message_.connect(Msg::TOUCH_BEGAN, obj_sp, &CubePlayer::touchesBegan);
    message_.connect(Msg::TOUCH_MOVED, obj_sp, &CubePlayer::touchesMoved);
    message_.connect(Msg::TOUCH_ENDED, obj_sp, &CubePlayer::touchesEnded);

    message_.connect(Msg::KEY_DOWN, obj_sp, &CubePlayer::keyDown);
  }
  
  
private:
  bool isActive() const override { return active_; }

  
  void inactive(const Message::Connection& connection, Param& params) {
    active_ = false;
  }

  // TIPS:CubePlayer::update が private で Entity::update は public という定義が可能
  void update(const Message::Connection& connection, Param& params) {
    double delta_time = boost::any_cast<double>(params.at("deltaTime"));

    if (begin_rotation_) {
      auto& information = boost::any_cast<std::vector<PlayerInfo>& >(params["playerInfo"]);
      if (startRotationMove(information)) {
        updateInformation(pos_block_, information);
      }
      begin_rotation_ = false;
    }

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
        auto& information = boost::any_cast<std::vector<PlayerInfo>& >(params["playerInfo"]);
        if (continueRotationMove(information)) {
          updateInformation(pos_block_, information);
        }
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

  void gatherInfo(const Message::Connection& connection, Param& params) {
    auto& informations = boost::any_cast<std::vector<PlayerInfo>& >(params["playerInfo"]);

    PlayerInfo info = {
      id_,
      pos_block_,
      pos_,
      now_rotation_
    };
    
    informations.push_back(std::move(info));
  }

  void postPlayerZ(const Message::Connection& connection, Param& params) {
    auto& player_z = boost::any_cast<std::vector<int>& >(params["playerZ"]);
    player_z.push_back(pos_block_.z);
  }

  
  void touchesBegan(const Message::Connection& connection, Param& params) {
    if (picking_) return;

    auto* touches = boost::any_cast<std::vector<Touch>* >(params.at("touch"));
#if 0
    if (touches->size() >= 2) {
      // 同時タッチは無視
      return;
    }
#endif

    auto* camera = boost::any_cast<Camera* >(params.at("camera"));
    for (auto& touch : *touches) {
      if (!touch.prior || touch.handled) continue;
      
      if (isBodyPicked(touch, *camera)) {
        // pick開始
        touch.handled = true;
        
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
  }

  void touchesMoved(const Message::Connection& connection, Param& params) {
    if (!picking_) return;

    const auto* touches = boost::any_cast<std::vector<Touch>* >(params.at("touch"));
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
    
    const auto* touches = boost::any_cast<std::vector<Touch>* >(params.at("touch"));
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
        double speed = (distance / touch_time) * speed_rate_;

        DOUT << "speed:" << speed << std::endl;
          
        int move_speed = std::min(int(speed), max_move_speed_);

        if (now_rotation_) {
          // 移動中なら、向きとスピードだけ更新
          move_direction_ = move_direction;
          move_speed_     = move_speed + 1;
        }
        else {
          // 移動開始
          begin_rotation_ = true;
          move_direction_ = move_direction;
          move_speed_     = move_speed;
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
      begin_rotation_ = true;
      move_direction_ = move_direction;
      move_speed_     = 0;
    }
  }
  
  
  ci::AxisAlignedBox3f getBoundingBox() {
    return std::move(ci::AxisAlignedBox3f(ci::Vec3f(-size_ / 2,     0, -size_ / 2) + pos_,
                                          ci::Vec3f( size_ / 2, size_,  size_ / 2) + pos_));
  }
  
  bool isBodyPicked(const Touch& touch, Camera& camera) {
    auto ray = camera.generateRay(touch.pos);

    float z_cross[2];
    int num = getBoundingBox().intersect(ray, z_cross);
    return num > 0;
  }

  bool startRotationMove(const std::vector<PlayerInfo>& information) {
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

    begin_rotation_ = false;

    now_rotation_      = true;
    move_rotate_time_  = 0.0;
    move_rotate_start_ = ci::Quatf::identity();
    move_rotate_end_   = rotate_table[move_direction_];
    move_rotate_       = move_rotate_start_;
    rotate_pivpot_     = pivot_table[move_direction_];

    pos_block_ += move_table[move_direction_];

    // speedが速いと回転スピードも速い
    float speed_rate = (move_speed_ < speed_table_.size()) ? speed_table_[move_speed_]
                                                           : speed_table_.back();

    move_rotate_time_end_ = move_rotate_time_end_max_ * speed_rate;
    
    return true;
  }

  bool continueRotationMove(const std::vector<PlayerInfo>& information) {
    if (move_speed_ == 0) return false;
    move_speed_ -= 1;

    return startRotationMove(information);
  }

  bool searchOtherPlayer(const ci::Vec3i block_pos, const std::vector<PlayerInfo>& information) {
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

  void updateInformation(const ci::Vec3i block_pos, std::vector<PlayerInfo>& information) {
    for (auto& info : information) {
      if (info.id != id_) continue;

      info.block_pos = block_pos;
      break;
    }
  }

  
  
};

}
