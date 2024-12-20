#include <iostream>
#include <functional>
#include <vector>
#include <map>
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

using prob = double;
using sparse_vector = std::vector<std::pair<int, prob>>;
using matrix = std::vector<sparse_vector>;

prob dot_sparse_dense(const sparse_vector& s, const std::vector<prob>& d)
{
  prob t = 0.0;
  for (const auto [i, x] : s)
  {
    t += x * d[i];
  }
  return t;
}

std::vector<prob> dot_matrix_vector(const matrix& m, const std::vector<prob>& d)
{
  std::vector<prob> r;
  r.resize(d.size());
  for (int i = 0; i < d.size(); ++i) { r[i] = 0.0; }
  for (int i = 0; i < d.size(); ++i)
  {
    for (auto [j, x] : m[i])
    {
      r[j] += x * d[i];
    }
  }
  return r;
}

void dump_vector(const std::vector<prob>& v)
{
  for (int i = 0; i < v.size(); ++i)
  {
    std::cout << v[i] << ' ';
  }
  std::cout << std::endl;
}

void dump_matrix(const matrix& m)
{
  for (int i = 0; i < m.size(); ++i)
  {
    std::cout << i << ':' << std::endl;
    for (auto& [label, prob] : m[i])
    {
      std::cout << " (" << label << "," << prob << ')';
    }
    std::cout << std::endl;
  }
}

template<typename X, typename F>
matrix build_matrix(const X& x, const F& f)
{
  std::map<X, int> labels;
  int next_label = 0;
  labels[x] = next_label++;
  std::vector<X> values_to_process;
  values_to_process.push_back(x);
  int next_value_to_process = 0;
  matrix m;

  sparse_vector row;
  while (next_value_to_process < values_to_process.size())
  {
    Dist<X> r = f(values_to_process[next_value_to_process]);
    row.clear();
    for (auto& [value, prob] : r.pdf)
    {
      if (labels.contains(value))
      {
        int label = labels[value];
        row.emplace_back(label, prob);
      }
      else
      {
        labels[value] = next_label;
        row.emplace_back(next_label, prob);
        values_to_process.push_back(value);
        ++next_label;
      }
    }
    m.push_back(row);
    ++next_value_to_process;
  }

  return m;
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
#if 0
  auto r = iterate_i(0, [](int x) { return roll(6) >> [=](int y) { return certainly(std::min(1, x + y)); }; }, 3);
  for (auto z : r.pdf)
  {
      std::cout << z.value << ' ' << z.prob << std::endl;
  }
#endif
  auto r = [](int x) { return roll(6) >> [=](int y) { return certainly(std::max(0, x - y)); }; };
  matrix m = build_matrix(6, r);
  dump_matrix(m);
  std::vector<prob> v{1, 0, 0, 0, 0, 0, 0};
  dump_vector(v);
  for (int i = 0; i < 6; ++i)
  {
    v = dot_matrix_vector(m, v);
    dump_vector(v);
  }
}
