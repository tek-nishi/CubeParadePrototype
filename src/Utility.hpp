#pragma once

//
// お役立ち関数群
//

namespace ngs {

u_int getUniqueNumber() {
  static u_int unique_number = 0;
  
  return ++unique_number;
}

}
