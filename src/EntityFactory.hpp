#pragma once

//
// Entity生成
//

#include "StageWatcher.hpp"
#include "CubePlayer.hpp"
#include "FallCube.hpp"
#include "EntryCube.hpp"
#include "TouchPreview.hpp"


namespace ngs {

class EntityFactory {
  Message& message_;
  Message::ConnectionHolder connection_holder_;

  const ci::JsonTree& params_;
  EntityHolder& entity_holder_;


public:
  EntityFactory(Message& message, const ci::JsonTree& params, EntityHolder& entity_holder) :
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
    createAndAddEntity<StageWatcher>(params_["stage"]);
    createAndAddEntity<TouchPreview>(params_);
  }

  
  void createCubePlayer(const Message::Connection& connection, Param& params) {
    createAndAddEntity<CubePlayer>(params_, boost::any_cast<const ci::Vec3i& >(params["entry_pos"]));
  }
  
  void createFallcube(const Message::Connection& connection, Param& params) {
    const auto& pos   = boost::any_cast<const ci::Vec3i& >(params["entry_pos"]);
    const auto& color = boost::any_cast<const ci::Color& >(params["color"]);
    const float speed = boost::any_cast<float>(params["speed"]);

    createAndAddEntity<FallCube>(params_, pos, speed, color);
  }

  void createEntrycube(const Message::Connection& connection, Param& params) {
    const auto& pos   = boost::any_cast<const ci::Vec3i& >(params["entry_pos"]);
    float offset_y = boost::any_cast<float>(params["offset_y"]);
    float active_time = boost::any_cast<float>(params["active_time"]);
    const auto& color = boost::any_cast<const ci::Color& >(params["color"]);

    createAndAddEntity<EntryCube>(params_, pos, offset_y, active_time, color);
  }

  
  // Entityを生成してHolderに追加
  // FIXME:可変長引数がconst参照になってたりしてる??
  template<typename T, typename... Args>
  void createAndAddEntity(Args&&... args) {
    // boost::shared_ptr<T> obj
    //   = boost::signals2::deconstruct<T>().postconstruct(boost::ref(message_),
    //                                                     std::forward<Args>(args)...);

    boost::shared_ptr<T> obj = boost::shared_ptr<T>(new T(message_));
    obj->setup(obj, std::forward<Args>(args)...);

    entity_holder_.add(obj);
  }
  
};

}
