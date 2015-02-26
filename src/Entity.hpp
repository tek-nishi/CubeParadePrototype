#pragma once

//
// クラスの実体を管理
//

#include <memory>
#include <deque>


namespace ngs {

// 管理するクラスはこれを継承する
struct Entity {
  virtual ~Entity() = default;

  virtual bool isActive() const = 0;
};


class EntityHolder {
  // FIXME:typedefするのではなく、テンプレートの引数にしたい
  using EntityPtr = boost::shared_ptr<Entity>;

public:
  void add(EntityPtr entity) {
    entities_.push_back(entity);
  }

  void update() {
    size_t num = entities_.size();
    while (num > 0) {
      // 先頭のを取り出し、有効なら最後尾に追加する
      auto& entity = entities_.front();

      // 参照で取り出しているので、先に追加
      if (entity->isActive()) entities_.push_back(entity);

      entities_.pop_front();
      
      num -= 1;
    }
  }

  
private:
  std::deque<EntityPtr> entities_;
  
};

}
