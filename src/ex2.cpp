#include <iostream>
#include <functional>
#include <vector>
#include <map>
#include <type_traits>
#include <algorithm>

#include "ChanceScript.h"

// [1,3,5] < [1,2,3,4]
// [1,3,5,7] !< [1,3]

void main1()
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

void test6a()
{
  Character player{6};
  Character monster{100};

    Dist<double, std::pair<Character, Character>> r = iterate_matrix_i(
      std::pair{player, monster},
      [](const std::pair<Character, Character>& both)
      {
          auto [player, monster] = both;
          return attacks(player, monster) >> [=](const Character& monster)
          {
            return attacks(monster, player) >> [=](const Character& player)
            {
              return certainly(std::pair{player, monster});
            };
          };
      },
      1000
      );
    for (auto z : r.pdf)
    {
        std::cout << std::get<0>(z.value).hit_points << ' ' << std::get<1>(z.value).hit_points << ' '<< z.prob << std::endl;
    }
}

int main()
{
  test6a();
}
