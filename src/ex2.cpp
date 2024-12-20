#include <iostream>
#include <functional>
#include <vector>
#include <map>
#include <type_traits>
#include <algorithm>

#include "ChanceScript.h"

// [1,3,5] < [1,2,3,4]
// [1,3,5,7] !< [1,3]

int main()
{
  auto r = [](int x) {
    return roll(6) >> [=](int y) {
      return certainly(std::max(0, x - y));
    };
  };

  auto p = iterate_matrix_i(1000, r, 285);

  p.dump();
  p.check();
}
