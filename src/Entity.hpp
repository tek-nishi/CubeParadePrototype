#pragma once

//
// クラスの実体を管理
//

#include <memory>
#include <vector>
#include <boost/range/algorithm_ext/erase.hpp>


namespace ngs {

// 管理するクラスはこれを継承する
struct Entity {
  virtual ~Entity() = default;

  virtual bool isActive() const = 0;
};


class EntityHolder {
  // FIXME:typedefするのではなく、テンプレートの引数にしたい
  using EntityPtr = boost::shared_ptr<Entity>;

  std::vector<EntityPtr> entities_;

  
public:
  void add(EntityPtr entity) {
    entities_.push_back(entity);
  }

  void eraseInactiveEntity() {
    boost::remove_erase_if(entities_,
                           [](EntityPtr& e) {
                             return !e->isActive();
                           });
  }
};

}
