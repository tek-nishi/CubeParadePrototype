#pragma once

//
// 実行環境
//

namespace ngs {

// 解像度: iPhone4相当
enum Frame {
  WIDTH = 960,
  HEIGHT = 640
};

enum Msg {
  UPDATE,
  POST_UPDATE,

  DRAW,
  DRAW_2D,

  TOUCH_BEGAN,
  TOUCH_MOVED,
  TOUCH_ENDED,

  KEY_DOWN,

  PARADE_START,
  PARADE_FINISH,

  // Playerの位置が確定した
  CUBE_PLAYER_POS,
  
  CUBE_PLAYER_DEAD,

  CUBE_STAGE_INFO,
  CUBE_STAGE_HEIGHT,

  // カメラ位置を決めるための情報収集
  CAMERAVIEW_INFO,

  // 光源用ステージ位置
  STAGE_POS,
  
  // 落下Cube生成
  CREATE_FALLCUBE,
  CREATE_ENTRYCUBE,

  LIGHT_ENABLE,
  LIGHT_DISABLE,

  
  SOUND_PLAY,
  SOUND_STOP,


  TOUCHPREVIEW_TOGGLE,
};

// タッチイベント識別ID
enum { MOUSE_EVENT_ID = 0xffffffff };

struct Touch {
  // OSXとWindowsとiOSの挙動の違いを吸収するwork-around
  // OSXとWindowsはmouseでの入力ならtrue
  // iOSは常にtrue
  bool prior;
  
  double timestamp;
  
  u_int id;
  ci::Vec2f pos;
  ci::Vec2f prev_pos;

  bool operator== (const Touch& rhs) const {
    return id == rhs.id;
  }

  bool operator== (const u_int rhs_id) const {
    return id == rhs_id;
  }
  
};

}
