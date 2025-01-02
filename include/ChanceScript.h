#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <numeric>
#include <type_traits>
#include <vector>

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &vec)
{
  for (const T &elem : vec)
  {
    os << elem << " ";
  }
  return os;
}

template <typename P, typename T>
struct atom
{
  T value;
  P prob;

  auto operator<=>(const atom &other) const { return value <=> other.value; }
};

template <typename P, typename T>
struct dist;

#if 0
template<typename P, typename T, typename F>
std::invoke_result_t<F, T> then(const dist<P, T> &m, const F& f);

template<typename P, typename T, typename F>
dist<P, std::invoke_result_t<F, T>> fmap(const dist<P, T> &m, const F& f);
#endif

template <typename T1, typename T2>
using AddResultType = decltype(std::declval<T1>() + std::declval<T2>());

template <typename T1, typename T2>
using TimesResultType = decltype(std::declval<T1>() * std::declval<T2>());

template <typename P, typename T>
struct dist
{
  dist(std::initializer_list<atom<P, T>> pdf_in)
      : pdf(pdf_in)
  {
  }

  template <typename U>
  dist<P, AddResultType<T, U>> operator+(const dist<P, U> &other) const
  {
    return and_then([&other](int x)
                    { return other.transform([x](int y) { return x + y; }); });
  }

  template <typename U>
  dist<P, AddResultType<T, U>> operator+(const U &other) const
  {
    return transform([&other](int x) { return x + other; });
  }

  void combine()
  {
    std::vector<atom<P, T>> combined;
    for (const auto &atom : pdf)
    {
      if (!combined.empty() && combined.back().value == atom.value)
      {
        combined.back().prob += atom.prob;
      }
      else
      {
        combined.push_back(atom);
      }
    }

    pdf = std::move(combined);
  }

  void sort() { std::sort(pdf.begin(), pdf.end()); }

  void remove_zero()
  {
    pdf.erase(std::remove_if(pdf.begin(), pdf.end(),
                             [](const atom<P, T> &p) { return p.prob == 0; }),
              pdf.end());
  }

  void chop(P eps)
  {
    pdf.erase(std::remove_if(pdf.begin(), pdf.end(), [eps](const atom<P, T> &p)
                             { return abs(p.prob) < eps; }),
              pdf.end());
  }

  void canonicalise()
  {
    sort();
    combine();
    remove_zero();
  }

  template <typename F>
  std::invoke_result_t<F, T> and_then(const F &f) const
  {
    std::invoke_result_t<F, T> result{};
    for (const auto &x : pdf)
    {
      auto fx = f(x.value);
      double prob = x.prob;
      for (auto &r : fx.pdf)
      {
        result.pdf.push_back(atom{r.value, prob * r.prob});
      }
    }

    result.canonicalise();

    return result;
  }

  template <typename F>
  dist<P, std::invoke_result_t<F, T>> transform(const F &f) const
  {
    dist<P, std::invoke_result_t<F, T>> result{};
    for (const auto &x : pdf)
    {
      auto fx = f(x.value);
      result.pdf.push_back(atom{fx, x.prob});
    }

    result.canonicalise();

    return result;
  }

  template <typename F>
  std::invoke_result_t<F, T> operator>>(const F &f) const
  {
    return this->and_then(f);
  }

  void check() const
  {
    double total = 0.0;

    for (const auto &[value, prob] : pdf)
    {
      total += prob;
    }

    std::cout << "total = " << total << std::endl;
  }

  void dump() const
  {
    for (const auto &[value, prob] : pdf)
    {
      std::cout << value << ": " << prob << std::endl;
    }
  }

  std::vector<atom<P, T>> pdf;
};

template <typename P, typename T, typename U>
dist<P, AddResultType<T, U>> operator+(const T &t, const dist<P, U> &du)
{
  return du.transform([t](const U &u) { return t + u; });
}

template <typename P, typename T, typename U>
dist<P, TimesResultType<T, U>> operator*(const T &t, const dist<P, U> &du)
{
  return du.transform([t](const U &u) { return t * u; });
}

template <typename X>
using ddist = dist<double, X>;

template <typename P = double, typename T>
std::ostream &operator<<(std::ostream &os, const atom<P, T> &atom)
{
  os << "atom(value: " << atom.value << ", prob: " << atom.prob << ")";
  return os;
}

template <typename P = double, typename T>
void sort_and_combine(std::vector<atom<P, T>> &atoms)
{
  std::sort(atoms.begin(), atoms.end(),
            [](const atom<P, T> &a, const atom<P, T> &b)
            { return a.value < b.value; });

  std::vector<atom<P, T>> combined;
  for (const auto &atom : atoms)
  {
    if (!combined.empty() && combined.back().value == atom.value)
    {
      combined.back().prob += atom.prob;
    }
    else
    {
      combined.push_back(atom);
    }
  }

  atoms = std::move(combined);
}

template <typename P = double, typename T>
dist<P, T> certainly(const T &t)
{
  return dist<P, T>{{t, 1.0}};
}

template <typename P = double>
dist<P, int> roll(int n)
{
  dist<P, int> result{};
  result.pdf.resize(n);
  for (int i = 0; i < n; ++i)
  {
    result.pdf[i] = atom{i + 1, 1. / n};
  }

  return result;
}

template <typename P = double>
dist<P, int> roll(int r, int n)
{
  dist<P, int> d = certainly(0);
  for (int i = 0; i < r; ++i)
  {
    d = d + roll(n);
  }
  return d;
}

// Keeps lowest
template <typename X, typename Compare>
void insert_and_keep_sorted(std::vector<X> &vec, X newElement, size_t maxSize,
                            Compare compare)
{
  // Find the position to insert the new element
  auto pos = std::lower_bound(vec.begin(), vec.end(), newElement, compare);

  // If the vector is already at maxSize, check if the new element should be
  // inserted
  if (vec.size() >= maxSize)
  {
    // If the new element is smaller than the largest element, replace the
    // largest element
    if (pos != vec.end()
        && (pos - vec.begin() < maxSize || newElement < vec.back()))
    {
      vec.insert(pos, newElement); // Insert the new element
      vec.pop_back();              // Remove the largest element
    }
  }
  else
  {
    // If the vector is not at max size, simply insert the new element
    vec.insert(pos, newElement);
  }
}

template <typename X>
X sum(const std::vector<X> &v)
{
  return std::accumulate(v.begin(), v.end(), 0);
}

// Roll `roll` times keeping `keep` highest given by `compare`.
// Eg. if `compare` is `std::greater` it keeps the greatest rolls
// in the usual sense.
template <typename P, typename X, typename Compare = std::greater<X>>
dist<P, std::vector<X>> highest_n(const dist<P, X> &dst, int roll, int keep,
                                  Compare compare = std::greater<X>())
{
  dist<P, std::vector<X>> so_far(certainly(std::vector<X>{}));

  for (int i = 0; i < roll; ++i)
  {
    so_far = so_far.and_then(
        [keep, compare, &dst](const std::vector<X> &v)
        {
          return dst.transform(
              [v, keep, compare](const X &x)
              {
                std::vector<X> v_copy = v;
                insert_and_keep_sorted(
                    v_copy, x, keep,
                    compare); //[](const X& a, const X& b) { return a > b; });
                return v_copy;
              });
        });
  }

  return so_far;
}

#define DO(b) [&]() { b; }();
#define LET(v, e, b) return e.and_then([&](v) { b; })
#define RETURN(x) return certainly(x)

// Function to concatenate two vectors and return the result
template <typename T>
std::vector<T> concatenate(const std::vector<T> &v1, const std::vector<T> &v2)
{
  std::vector<T> result;
  result.reserve(v1.size()
                 + v2.size()); // Reserve memory to avoid multiple reallocations
  result.insert(result.end(), v1.begin(), v1.end());
  result.insert(result.end(), v2.begin(), v2.end());
  return result;
}

void test1()
{
#if 0
    auto r = DO(
               LET(int x, roll(6),
               LET(int y, roll(6),
               RETURN(x + y);
               )));
#else
  auto r = roll(6) >> [](int x)
  { return roll(6) >> [=](int y) { return certainly(x + y); }; };
#endif
  for (auto z : r.pdf)
  {
    std::cout << z << std::endl;
  }
}

template <typename P = double, typename T>
dist<P, std::vector<T>> sequence(const std::vector<dist<P, T>> &dists,
                                 int starting_from = 0)
{
  if (starting_from >= dists.size())
  {
    return certainly(std::vector<T>{});
  }

  return DO(
      LET(const T &head, dists[starting_from],
          LET(const std::vector<T> &tail, sequence(dists, starting_from + 1),
              RETURN(concatenate(std::vector<T>{head}, tail)))));
}

template <typename P = double, typename T, typename F>
dist<P, T> fold(const F &f, const T &init, const std::vector<dist<P, T>> &dists,
                int starting_from = 0)
{
  if (starting_from >= dists.size())
  {
    return certainly(init);
  }

  return DO(LET(const T &head, dists[starting_from],
                return fold(f, f(init, head), dists, starting_from + 1)));
}

namespace cs
{

class max
{
public:
  template <typename X>
  X operator()(const X &a, const X &b) const
  {
    return a > b ? a : b;
  }
};

} // namespace cs

template <typename P = double, typename T, typename U, typename F>
dist<P, T> repeated(const F &f, const T &init, const dist<P, U> &dist, int n)
{
  if (n <= 0)
  {
    return certainly(init);
  }

  return repeated(f, init, dist, n - 1) >> [&](int head)
  { return dist >> [&](int tail) { return certainly(f(head, tail)); }; };
}

// Roll n times using distribution `d` and fold
// the resutls together using `f`.
template <typename P, typename F, typename T>
dist<P, T> reduce(int n, const F &f, dist<P, T> &d)
{
  auto e = d;
  for (int i = 1; i < n; ++i)
  {
    e = e.and_then(
        [&f, &d](const T &v)
        { return d.transform([&v, &f](const T &w) { return f(v, w); }); });
  }
  return e;
}

void test2()
{
  std::vector<dist<double, int>> rolls{roll(6), roll(6), roll(6), roll(6)};
  dist<double, std::vector<int>> r = sequence(rolls);
  for (auto z : r.pdf)
  {
    std::cout << z << std::endl;
  }
}

void test3()
{
  dist<double, int> r =
      repeated([](int x, int y) { return std::max(x, y); }, 0, roll(6), 50);
  for (auto z : r.pdf)
  {
    std::cout << z << std::endl;
  }
}

template <typename P = double, typename X, typename F>
dist<P, X> iterate(const X &init, const F &f, int n)
{
  dist<double, X> r = certainly(init);
  for (int i = 0; i < n; ++i)
  {
    r = r >> f;
  }
  return r;
}

void test4()
{
  struct X
  {
    int x;
    int y;
    auto operator<=>(const X &) const = default;
  };
  auto r = iterate(
      X{0, 0},
      [](const auto &xy)
      {
        return roll(6) >> [&xy](int t)
        { return certainly(X{10 * xy.x + t, 100 * xy.y + t}); };
      },
      3);
  for (auto z : r.pdf)
  {
    std::cout << z.value.x << ' ' << z.value.y << ' ' << z.prob << std::endl;
  }
}

void test4a()
{
  struct X
  {
    int x;
    int y;
    auto operator<=>(const X &) const = default;
  };
  auto r = certainly(X{0, 0});
  for (int i = 0; i < 4; ++i)
  {
    r = r >> [](const auto &xy)
    {
      return roll(6) >> [&xy](int t)
      { return certainly(X{10 * xy.x + t, 100 * xy.y + t}); };
    };
  }
  for (auto z : r.pdf)
  {
    std::cout << z.value.x << ' ' << z.value.y << ' ' << z.prob << std::endl;
  }
}

template <typename P = double, typename F, typename... FArgs>
dist<P, std::tuple<FArgs...>> iterate_all(const F &f, int n, FArgs... Args)
{
  if (n == 0)
  {
    return certainly(std::tuple<FArgs...>(Args...));
  }
  return iterate_all(f, n - 1, Args...) >>
         [f](const std::tuple<FArgs...> ArgsTuple)
  { return std::apply(f, ArgsTuple); };
}

template <typename P = double, typename... FArgs>
dist<P, std::tuple<FArgs...>> certainly_all(FArgs... Args)
{
  return certainly(std::tuple<FArgs...>(Args...));
}

void test5()
{
  auto r = iterate_all(
      [](int x, int y)
      {
        return roll(6) >> [=](int t)
        { return certainly_all(10 * x + t, 100 * y + t); };
      },
      3, 0, 0);

  for (auto z : r.pdf)
  {
    std::cout << std::get<0>(z.value) << ' ' << z.prob << std::endl;
  }
}

struct Character
{
  int hit_points;

  auto operator<=>(const Character &) const = default;
  Character damage(int damage) const
  {
    if (damage >= hit_points)
    {
      return Character{0};
    }
    else
    {
      return Character{hit_points - damage};
    }
  }
};

template <typename P = double>
dist<P, Character> attacks(const Character &player, const Character &monster)
{
  if (player.hit_points > 0)
  {
    return roll(20) >> [=](int player_hit_roll)
    {
      return player_hit_roll >= 11 ? roll(6) >> [=](int player_damage)
      { return certainly(monster.damage(player_damage)); }
                                   : certainly(monster);
    };
  }
  else
  {
    return certainly(monster);
  }
}

void test6()
{
  Character player{6};
  Character monster{100};

  auto r = iterate_all(
      [](const Character &player, const Character &monster)
      {
        return attacks(player, monster) >> [=](const Character &monster)
        {
          return attacks(monster, player) >> [=](const Character &player)
          { return certainly_all(player, monster); };
        };
      },
      100, player, monster);
  for (auto z : r.pdf)
  {
    std::cout << std::get<0>(z.value).hit_points << ' '
              << std::get<1>(z.value).hit_points << ' ' << z.prob << std::endl;
  }
}

// Roll d6 `r` times but keep top `k`.
template <typename P = double>
dist<P, std::vector<int>> roll_keep(int r, int k)
{
  if (r == 0)
  {
    return certainly(std::vector<int>{});
  }
  else
  {
    return roll_keep(r - 1, k) >> [=](const std::vector<int> &rolls)
    {
      return roll(6) >> [=](int n)
      {
        auto new_rolls = rolls;
        new_rolls.push_back(n);
        std::sort(new_rolls.begin(), new_rolls.end());
        if (new_rolls.size() > k)
        {
          new_rolls.resize(k);
        }
        return certainly(new_rolls);
      };
    };
  }
}

namespace cs
{

int sum(const std::vector<int> &a)
{
  int t = 0;
  for (auto x : a)
  {
    t += x;
  }
  return t;
}

} // namespace cs

void test7()
{
  auto r = roll_keep(8, 4).transform(cs::sum);
  for (auto z : r.pdf)
  {
    std::cout << z.value << ' ' << z.prob << std::endl;
  }
}

#if 0
int main()
{
  test7();
}
#endif

template <typename P, typename T>
bool subdist(const dist<P, T> &a, const dist<P, T> &b)
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

// using prob = double;
template <typename P>
using sparse_vector = std::vector<std::pair<int, P>>;
template <typename P>
using matrix = std::vector<sparse_vector<P>>;

template <typename P>
P dot_sparse_dense(const sparse_vector<P> &s, const std::vector<P> &d)
{
  P t = 0.0;
  for (const auto [i, x] : s)
  {
    t += x * d[i];
  }
  return t;
}

template <typename P>
std::vector<P> dot_matrix_vector(const matrix<P> &m, const std::vector<P> &d)
{
  std::vector<P> r;
  r.resize(d.size());
  for (int i = 0; i < d.size(); ++i)
  {
    r[i] = 0.0;
  }
  for (int i = 0; i < d.size(); ++i)
  {
    for (auto [j, x] : m[i])
    {
      r[j] += x * d[i];
    }
  }
  return r;
}

template <typename P>
void dump_vector(const std::vector<P> &v)
{
  for (int i = 0; i < v.size(); ++i)
  {
    std::cout << v[i] << ' ';
  }
  std::cout << std::endl;
}

template <typename P>
void dump_matrix(const matrix<P> &m)
{
  for (int i = 0; i < m.size(); ++i)
  {
    std::cout << i << ':' << std::endl;
    for (auto &[label, prob] : m[i])
    {
      std::cout << " (" << label << "," << prob << ')';
    }
    std::cout << std::endl;
  }
}

template <typename P, typename X>
struct setup
{
  matrix<P> m;
  std::map<X, int> labels;
  std::vector<X> values;
};

template <typename P = double, typename X>
dist<P, X> convert_to_pdf(setup<P, X> *s, const std::vector<P> &v)
{
  dist<P, X> d{};
  for (int i = 0; i < v.size(); ++i)
  {
    d.pdf.emplace_back(s->values[i], v[i]);
  }
  d.sort();
  d.remove_zero();

  return d;
}

template <typename P, typename X, typename F>
setup<P, X> *build_matrix(const X &x, const F &f)
{
  std::map<X, int> labels;
  int next_label = 0;
  labels[x] = next_label++;
  std::vector<X> values_to_process;
  values_to_process.push_back(x);
  int next_value_to_process = 0;
  matrix<P> m;

  sparse_vector<P> row;
  while (next_value_to_process < values_to_process.size())
  {
    dist<P, X> r = f(values_to_process[next_value_to_process]);
    row.clear();
    for (auto &[value, prob] : r.pdf)
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

  return new setup{m, labels, values_to_process};
}

template <typename P = double, typename X, typename F>
dist<P, X> iterate_i(const X &init, const F &f, int n)
{
  dist<P, X> r = certainly(init);
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

template <typename P = double, typename X, typename F>
dist<P, X> iterate_matrix_i(const X &init, const F &f, int n)
{
  setup<P, X> *s = build_matrix<P>(init, f);
  // dump_matrix(s->m);
  int dim = s->values.size();
  std::vector<P> v;
  v.resize(dim);
  std::fill(v.begin(), v.end(), 0);
  v[0] = 1;
  for (int i = 0; i < n; ++i)
  {
    v = dot_matrix_vector(s->m, v);
  }
  auto p = convert_to_pdf(s, v);
  return p;
}
