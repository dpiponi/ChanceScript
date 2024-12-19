#include <iostream>
#include <functional>
#include <vector>
#include <type_traits>
#include <algorithm>

#include "ChanceScript.h"

// [1,3,5] < [1,2,3,4]
// [1,3,5,7] !< [1,3]
template<typename T>
bool subdist(const Dist<T>& a, const Dist<T>& b)
{
  int i = 0;
  int j = 0;
  while (i < a.pdf.size() && j < b.pdf.size())
  {
    if (a.pdf[i].value < b.pdf[i].value)
    {
      return false;
    }
    ++i;
    ++j;
  }
  return i < a.pdf.size();
}

template<typename X, typename F>
Dist<X> iterate_i(const X& init, const F& f, int n)
{
    Dist<X> r = certainly(init);
    for (int i = 0; i < n; ++i)
    {
      const auto old_r = r;
      r = old_r >> f;
      std::cout << old_r.pdf.size() << '/' << r.pdf.size() << std::endl;
      if (subdist(r, old_r))
      {
        std::cout << "Explored" << std::endl;
      }
    }
    return r;
}

int main()
{
  auto r = iterate_i(0, [](int x) { return roll(6) >> [=](int y) { return certainly(std::min(1, x + y)); }; }, 3);
  for (auto z : r.pdf)
  {
      std::cout << z.value << ' ' << z.prob << std::endl;
  }
}
