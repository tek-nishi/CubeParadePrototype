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

  SETUP_GAME,
  
  PARADE_START,
  PARADE_FINISH,

  // 各種情報を収集
  GATHER_INFORMATION,

  // Playerの位置が確定した
  CUBE_PLAYER_POS,
  
  CUBE_PLAYER_DEAD,

  CUBE_STAGE_INFO,
  CUBE_STAGE_HEIGHT,

  // カメラ位置を決めるための情報収集
  CAMERAVIEW_INFO,

  // 光源用ステージ位置
  STAGE_POS,
  
  // Entity生成
  CREATE_CUBEPLAYER,
  CREATE_FALLCUBE,
  CREATE_ENTRYCUBE,

  LIGHT_ENABLE,
  LIGHT_DISABLE,

  
  SOUND_PLAY,
  SOUND_STOP,


  TOUCHPREVIEW_TOGGLE,
};


struct PlayerInfo {
  u_int id;
  
  ci::Vec3i block_pos;
  ci::Vec3f pos;
  
  bool now_rotation;
};


}
