#pragma once

//
// オブジェクト間メッセージ
//

#include <boost/any.hpp>
#include <boost/signals2.hpp>
#include <boost/signals2/deconstruct.hpp>
#include <boost/shared_ptr.hpp>
#include <map>
#include <memory>
#include <string>


namespace ngs {

// メッセージ用引数
using Param = std::map<std::string, boost::any>;


class Message {

public:
  Message() = default;
  
  
  using Connection = boost::signals2::connection;
  // VS2013でエラー
  // using Callback = void(Param&);
  using SignalType = boost::signals2::signal<void(Param&)>;


  // boost::shared_ptr、std::shared_ptr、pointer、lambda式と、
  // 型に合わせて登録関数を定義
  template <typename T, typename F>
  Connection connect(const int msg, boost::shared_ptr<T> object, F callback) {
    return siglans_[msg].connect_extended(SignalType::extended_slot_type(callback, object.get(), _1, _2).track(object));
  }

  template <typename T, typename F>
  Connection connect(const int msg, std::shared_ptr<T> object, F callback) {
    return siglans_[msg].connect_extended(SignalType::extended_slot_type(callback, object.get(), _1, _2).track_foreign(object));
  }

  template<typename T, typename F>
  Connection connect(const int msg, T* object, F callback) {
    return siglans_[msg].connect_extended(boost::bind(callback, object, _1, _2));
  }
  
  template<typename F>
  Connection connect(const int msg, F callback) {
    return siglans_[msg].connect_extended(callback);
  }  


  void signal(const int msg, Param& params) {
    siglans_[msg](params);
  }

  void signal(const int msg, Param&& params) {
    signal(msg, params);
  }


  // Messageを自動的に切断するヘルパー
  class ConnectionHolder {
    std::vector<Connection> connections_;

    // TIPS:コピー不可
    ConnectionHolder(const ConnectionHolder&) = delete;
    ConnectionHolder& operator=(const ConnectionHolder&) = delete;

  public:
    ConnectionHolder() = default;
    
    void add(Connection connection) {
      connections_.push_back(connection);
    }

    ~ConnectionHolder() {
      for (auto& connection : connections_) {
        connection.disconnect();
      }
    }
  };

  
private:
  // TIPS:コピー不可
  Message(const Message&) = delete;
  Message& operator=(const Message&) = delete;

  std::map<int, SignalType> siglans_;
  
};

}
