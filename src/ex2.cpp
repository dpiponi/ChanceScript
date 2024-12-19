#include <iostream>
#include <functional>
#include <vector>
#include <type_traits>
#include <algorithm>

#include "ChanceScript.h"

template<typename T>
void Dump(const Dist<T>& dist)
{
  for (auto z : dist.pdf)
  {
      std::cout << z.value << ' ' << z.prob << std::endl;
  }
}

// [1,3,5] < [1,2,3,4]
// [1,3,5,7] !< [1,3]
template<typename T>
bool subdist(const Dist<T>& a, const Dist<T>& b)
{
  int i = 0;
  int j = 0;
  while (i < a.pdf.size() && j < b.pdf.size())
  {
    if (a.pdf[i].value == b.pdf[j].value)
    {
      ++i;
    } else if (a.pdf[i].value < b.pdf[j].value)
    {
      return false;
    } else
    if (a.pdf[i].value > b.pdf[j].value)
    {
      ++j;
    }
  }
  return i == a.pdf.size();
}

template<typename X, typename F>
Dist<X> do_as_matrix(const Dist<X>& d, const F& f)
{
  for (auto& [v, p] : d.pdf)
  {
    auto row = v >> f;
  }
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
        std::cout << "Explored old" << std::endl;
        Dump(old_r);
        std::cout << "And new" << std::endl;
        Dump(r);
        std::cout << "---" << std::endl;
        do_as_matrix(r, F);
      }
    }
    return r;
}

int main()
{
  auto r = iterate_i(0, [](int x) { return roll(6) >> [=](int y) { return certainly(std::min(9, x + y)); }; }, 3);
  Dump(r);
}
