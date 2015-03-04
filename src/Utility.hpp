#pragma once

//
// お役立ち関数群
//

namespace ngs {

u_int getUniqueNumber() {
  static u_int unique_number = 0;
  
  return ++unique_number;
}

// 配列の要素数を取得
template <typename T>
std::size_t elemsof(const T& t) {
  return std::distance(std::begin(t), std::end(t));
}

}
