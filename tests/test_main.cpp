#include <gtest/gtest.h>  

#include "ChanceScript.h"

TEST(ChanceScript, test1) {
  auto d = roll(6);
  double total = 0.0;
  for (auto [x, p] : d.pdf)
  {
    total += p;
  }
  EXPECT_FLOAT_EQ(total, 1.0); 
}

TEST(ChanceScript, test2) {
  int x(12);

  auto d = iterate_matrix_i(
    x, [](const int x) {
      return roll(6) >> [x](const int reduction) {
        return certainly(std::max(0, x - reduction));
      };
    }, 2);

  for (auto [x, p] : d.pdf)
  {
    EXPECT_FLOAT_EQ(p, std::min(x + 1, 11 - x) / 36.);
  }
}

TEST(ChanceScript, test3) {
  auto d = roll(6).transform([](int x) { return 5 - (x - 1) / 2; });

  for (auto [x, p] : d.pdf)
  {
    EXPECT_FLOAT_EQ(p, 1 / 3.);
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
