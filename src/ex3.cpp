// Examples from:
// https://www.reddit.com/r/3d6/comments/9skfiz/anydice_tutorial_part_1_basics_and_damage/

#include <numeric>

#include "ChanceScript.h"

void example1()
{
  std::cout << "Example 1" << std::endl;
  roll(6).dump();
}

void example2()
{
  std::cout << "Example 2" << std::endl;
  (roll(6) + 2).dump();
}

void example3()
{
  std::cout << "Example 3" << std::endl;
  std::cout << "2d6 vary independently of each other" << std::endl;
  roll(2, 6).dump();

  std::cout << "2*d6 depends only on one die roll" << std::endl;
  (2 * roll(6)).dump();

  std::cout << "variables capture entire distributions" << std::endl;
  auto x = roll(6);
  (x + x).dump();
}

void example4()
{
  auto d20 = roll(20);
  auto d = reduce(3, cs::max(), d20);
  d.dump();
}

void example5()
{
  std::cout << "Example 5" << std::endl;
  auto d = highest_n(roll(6), 100, 4);
  d.dump();
}

int main()
{
  example1();
  example2();
  example3();
  example4();
  example5();
}
