#pragma once

//
// 実行環境
//

namespace ngs {

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

  // Stage生成
  SETUP_STAGE,
  RESET_STAGE,
  
  PARADE_START,
  PARADE_FINISH,

  ALL_STAGE_CLEAR,
  
  // 各種情報を収集
  GATHER_INFORMATION,

  // 確定したPlayerの位置をbroadcast
  CUBE_PLAYER_POS,
  
  CUBE_PLAYER_DEAD,

  // 開始位置、ゴール位置などのStage情報をbroadcast
  POST_STAGE_INFO,
  CUBE_STAGE_HEIGHT,

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
