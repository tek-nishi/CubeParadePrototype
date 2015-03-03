#pragma once

//
// Entity生成
//

#include "StageWatcher.hpp"
#include "CubePlayer.hpp"
#include "FallCube.hpp"
#include "EntryCube.hpp"
#include "TouchPreview.hpp"
#include "Light.hpp"
#include "Stage.hpp"


namespace ngs {

class EntityFactory {
  Message& message_;
  Message::ConnectionHolder connection_holder_;

  ci::JsonTree& params_;
  EntityHolder& entity_holder_;


public:
  EntityFactory(Message& message, ci::JsonTree& params, EntityHolder& entity_holder) :
    message_(message),
    params_(params),
    entity_holder_(entity_holder)
  {
    connection_holder_.add(message.connect(Msg::SETUP_GAME, this, &EntityFactory::setupGame));

    connection_holder_.add(message.connect(Msg::CREATE_CUBEPLAYER, this, &EntityFactory::createCubePlayer));
    connection_holder_.add(message.connect(Msg::CREATE_FALLCUBE, this, &EntityFactory::createFallcube));
    connection_holder_.add(message.connect(Msg::CREATE_ENTRYCUBE, this, &EntityFactory::createEntrycube));
  }


private:
  // TIPS:コピー不可
  EntityFactory(const EntityFactory&) = delete;
  EntityFactory& operator=(const EntityFactory&) = delete;

  
  void setupGame(const Message::Connection& connection, Param& params) {
    createAndAddEntity<Light>();
    createAndAddEntity<Stage>();
    createAndAddEntity<StageWatcher>();
    createAndAddEntity<TouchPreview>();
  }

  
  void createCubePlayer(const Message::Connection& connection, Param& params) {
    createAndAddEntity<CubePlayer>(boost::any_cast<const ci::Vec3i& >(params["entry_pos"]));
  }
  
  void createFallcube(const Message::Connection& connection, Param& params) {
    const auto& pos   = boost::any_cast<const ci::Vec3i& >(params["entry_pos"]);
    const auto& color = boost::any_cast<const ci::Color& >(params["color"]);
    const float speed = boost::any_cast<float>(params["speed"]);

    createAndAddEntity<FallCube>(pos, speed, color);
  }

  void createEntrycube(const Message::Connection& connection, Param& params) {
    const auto& pos   = boost::any_cast<const ci::Vec3i& >(params["entry_pos"]);
    auto offset_y = boost::any_cast<float>(params["offset_y"]);
    auto active_time = boost::any_cast<double>(params["active_time"]);
    const auto& color = boost::any_cast<const ci::Color& >(params["color"]);

    createAndAddEntity<EntryCube>(pos, offset_y, active_time, color);
  }

  
  // Entityを生成してHolderに追加
  // FIXME:可変長引数がconst参照になってたりしてる??
#if 0
  template<typename T, typename... Args>
  void createAndAddEntity(Args&&... args) {
    // boost::shared_ptr<T> obj
    //   = boost::signals2::deconstruct<T>().postconstruct(boost::ref(message_),
    //                                                     std::forward<Args>(args)...);

    boost::shared_ptr<T> obj = boost::shared_ptr<T>(new T(message_));
    obj->setup(obj, std::forward<Args>(args)...);

    entity_holder_.add(obj);
  }
#endif
  
  template<typename T, typename... Args>
  void createAndAddEntity(Args&&... args) {
    boost::shared_ptr<T> obj = boost::shared_ptr<T>(new T(message_, params_));
    obj->setup(obj, std::forward<Args>(args)...);

    entity_holder_.add(obj);
  }
  
};

}
