#include <iostream>
#include <functional>
#include <vector>
#include <map>
#include <type_traits>
#include <algorithm>

template<typename Prob, typename T>
struct atom {
  T value;
  Prob prob;

  auto operator<=>(const atom& other) const
  {
    return value <=> other.value;
  }
};

template<typename Prob, typename T>
struct dist;

template<typename Prob, typename T, typename F>
std::invoke_result_t<F, T> then(const dist<Prob, T> &m, const F& f);

template<typename Prob, typename T, typename F>
dist<Prob, std::invoke_result_t<F, T>> fmap(const dist<Prob, T> &m, const F& f);

template<typename Prob, typename T>
struct dist
{
  dist(std::initializer_list<atom<Prob, T>> pdf_in) : pdf(pdf_in) {}

  std::vector<atom<Prob, T>> pdf;

  void resort()
  {
    sort(pdf.begin(), pdf.end());

    pdf.erase(
        std::remove_if(
          pdf.begin(), pdf.end(), [](const atom<Prob, T>& p) {
            return p.prob == 0;
        }),
        pdf.end()
    );
  }

  template<typename F>
  std::invoke_result_t<F, T> and_then(const F& f)
  {
    return then(*this, f);
  }

  template<typename F>
  dist<Prob, std::invoke_result_t<F, T>> transform(const dist<Prob, T> &m, const F& f) {
    return fmap(*this, f);
  }

  template<typename F>
  std::invoke_result_t<F, T> operator>>(const F& f) const
  {
    return then(*this, f);
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
};

template <typename X>
using ddist = dist<double, X>;

template <typename Prob = double, typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec)
{
    for (const T& elem : vec) {
        os << elem << " ";
    }
    return os;
}

template <typename Prob = double, typename T>
std::ostream& operator<<(std::ostream& os, const atom<Prob, T>& atom)
{
    os << "atom(value: " << atom.value << ", prob: " << atom.prob << ")";
    return os;
}

template <typename Prob = double, typename T>
void sortAndCombine(std::vector<atom<Prob, T>>& atoms) {
    std::sort(atoms.begin(), atoms.end(), [](const atom<Prob, T>& a, const atom<Prob, T>& b) {
        return a.value < b.value;
    });

    std::vector<atom<Prob, T>> combined;
    for (const auto& atom : atoms) {
        if (!combined.empty() && combined.back().value == atom.value) {
            combined.back().prob += atom.prob;
        } else {
            combined.push_back(atom);
        }
    }

    atoms = std::move(combined);
}

template<typename Prob, typename T, typename F>
std::invoke_result_t<F, T> then(const dist<Prob, T> &m, const F& f)
{
    std::invoke_result_t<F, T> result{};
    for (const auto& x : m.pdf)
    {
        auto fx = f(x.value);
        double prob = x.prob;
        for (auto& r : fx.pdf)
        {
            result.pdf.push_back(atom{r.value, prob * r.prob});
        }
    }

    sortAndCombine(result.pdf);
    return result;
}

template<typename Prob, typename T, typename F>
dist<Prob, std::invoke_result_t<F, T>> fmap(const dist<Prob, T> &m, const F& f)
{
    dist<Prob, std::invoke_result_t<F, T>> result{};
    for (const auto& x : m.pdf)
    {
        auto fx = f(x.value);
        result.pdf.push_back(atom{fx, x.prob});
    }

    sortAndCombine(result.pdf);
    return result;
}

template<typename Prob = double>
dist<Prob, int> roll(int n)
{
  dist<Prob, int> result{};
  result.pdf.resize(n);
  for (int i = 0; i < n; ++i)
  {
    result.pdf[i] = atom{i + 1, 1. / n};
  }

  return result;
}

template<typename Prob = double, typename T>
dist<Prob, T> certainly(const T& t)
{
  return dist<Prob, T>{{t, 1.0}};
}

#define DO(b) [&](){ b; }();
#define LET(v, e, b) return then(e, [&](v) { b; })
#define RETURN(x) return certainly(x)

#include <iostream>
#include <vector>

// Function to concatenate two vectors and return the result
template <typename T>
std::vector<T> concatenate(const std::vector<T>& v1, const std::vector<T>& v2) {
    std::vector<T> result;
    result.reserve(v1.size() + v2.size());  // Reserve memory to avoid multiple reallocations
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
  auto r = roll(6) >> [](int x) {
    return roll(6) >> [=](int y) { return certainly(x + y); };
  };
#endif
    for (auto z : r.pdf)
    {
        std::cout << z << std::endl;
    }
}

template <typename Prob = double, typename T>
dist<Prob, std::vector<T>> sequence(const std::vector<dist<Prob, T>> &dists,
                                    int starting_from = 0) {
  if (starting_from >= dists.size())
  {
    return certainly(std::vector<T>{});
  }

  return DO(
    LET(const T& head, dists[starting_from],
    LET(const std::vector<T>& tail, sequence(dists, starting_from + 1),
    RETURN(concatenate(std::vector<T>{head}, tail)))));
}

template<typename Prob = double, typename T, typename F>
dist<Prob, T> fold(const F& f, const T& init, const std::vector<dist<Prob, T>>& dists, int starting_from = 0)
{
  if (starting_from >= dists.size())
  {
    return certainly(init);
  }

  return DO(
    LET(const T& head, dists[starting_from],
    return fold(f, f(init, head), dists, starting_from + 1)));
}

template<typename Prob = double, typename T, typename F>
dist<Prob, T> repeated(const F& f, const T& init, const dist<Prob, T>& dist, int n)
{
  if (n <= 0)
  {
    return certainly(init);
  }

#if 0
  return DO(
    LET(int head, repeated(f, init, dist, n - 1),
    LET(int tail, dist,
    RETURN(f(head, tail)))));
#else
  return repeated(f, init, dist, n - 1) >> [&](int head)
  {
    return dist >> [&](int tail)
    {
      return certainly(f(head, tail));
    };
  };
#endif
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
    dist<double, int> r = repeated([](int x, int y) { return std::max(x, y); }, 0, roll(6), 50);
    for (auto z : r.pdf)
    {
        std::cout << z << std::endl;
    }
}

template<typename Prob = double, typename X, typename F>
dist<Prob, X> iterate(const X& init, const F& f, int n)
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
    struct X {int x; int y; auto operator<=>(const X&) const = default; };
    auto r = iterate(
      X{0, 0},
      [](const auto& xy)
      {
          return roll(6) >> [&xy](int t)
          {
            return certainly(X{10 * xy.x + t, 100 * xy.y + t});
          };
      },
      3
      );
    for (auto z : r.pdf)
    {
        std::cout << z.value.x << ' ' << z.value.y << ' ' << z.prob << std::endl;
    }
}

void test4a()
{
    struct X {int x; int y; auto operator<=>(const X&) const = default; };
    auto r = certainly(X{0, 0});
    for (int i = 0; i < 4; ++i)
    {
      r = r >> [](const auto& xy)
        {
            return roll(6) >> [&xy](int t)
            {
              return certainly(X{10 * xy.x + t, 100 * xy.y + t});
            };
        };
    }
    for (auto z : r.pdf)
    {
        std::cout << z.value.x << ' ' << z.value.y << ' ' << z.prob << std::endl;
    }
}

template<typename Prob = double, typename F, typename... FArgs>
dist<Prob, std::tuple<FArgs...>> iterate_all(const F& f, int n, FArgs... Args)
{
    if (n == 0)
    {
        return certainly(std::tuple<FArgs...>(Args...));
    }
    return iterate_all(f, n - 1, Args...) >> [f](const std::tuple<FArgs...> ArgsTuple) { return std::apply(f, ArgsTuple); };
}

template<typename Prob = double, typename... FArgs>
dist<Prob, std::tuple<FArgs...>> certainly_all(FArgs... Args)
{
        return certainly(std::tuple<FArgs...>(Args...));
}

void test5()
{
    auto r = iterate_all(
      [](int x, int y)
      {
          return roll(6) >> [=](int t)
          {
            return certainly_all(10 * x + t, 100 * y + t);
          };
      },
      3,
          0, 0
      );

    for (auto z : r.pdf)
    {
        std::cout << std::get<0>(z.value) << ' ' << z.prob << std::endl;
    }
}

struct Character
{
  int hit_points;

  auto operator<=>(const Character&) const = default; 
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

template<typename Prob = double>
dist<Prob, Character> attacks(const Character& player, const Character& monster)
{
  if (player.hit_points > 0)
  {
    return roll(20) >> [=](int player_hit_roll)
    {
      return player_hit_roll >= 11 ?
        roll(6) >> [=](int player_damage) { return certainly(monster.damage(player_damage)); }
        : certainly(monster);
    };
  } else {
    return certainly(monster);
  }
}

void test6()
{
  Character player{6};
  Character monster{100};

    auto r = iterate_all(
      [](const Character& player, const Character& monster)
      {
          return attacks(player, monster) >> [=](const Character& monster)
          {
            return attacks(monster, player) >> [=](const Character& player)
            {
              return certainly_all(player, monster);
            };
          };
      },
      100,
      player, monster
      );
    for (auto z : r.pdf)
    {
        std::cout << std::get<0>(z.value).hit_points << ' ' << std::get<1>(z.value).hit_points << ' '<< z.prob << std::endl;
    }
}

template<typename Prob = double>
dist<Prob, std::vector<int>> roll_keep(int r, int k)
{
  if (r == 0)
  {
    return certainly(std::vector<int>{});
  }
  else
  {
    return roll_keep(r - 1, k) >> [=](const std::vector<int>& rolls)
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

int sum(const std::vector<int>& a)
{
  int t = 0;
  for (auto x : a)
  {
    t += x;
  }
  return t;
}

void test7()
{
  auto r = fmap(roll_keep(8, 4), sum);
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

template<typename Prob, typename T>
bool subdist(const dist<Prob, T>& a, const dist<Prob, T>& b)
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
template<typename Prob>
using sparse_vector = std::vector<std::pair<int, Prob>>;
template<typename Prob>
using matrix = std::vector<sparse_vector<Prob>>;

template<typename Prob>
Prob dot_sparse_dense(const sparse_vector<Prob>& s, const std::vector<Prob>& d)
{
  Prob t = 0.0;
  for (const auto [i, x] : s)
  {
    t += x * d[i];
  }
  return t;
}

template<typename Prob>
std::vector<Prob> dot_matrix_vector(const matrix<Prob>& m, const std::vector<Prob>& d)
{
  std::vector<Prob> r;
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

template<typename Prob>
void dump_vector(const std::vector<Prob>& v)
{
  for (int i = 0; i < v.size(); ++i)
  {
    std::cout << v[i] << ' ';
  }
  std::cout << std::endl;
}

template<typename Prob>
void dump_matrix(const matrix<Prob>& m)
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

template<typename Prob, typename X>
struct setup
{
  matrix<Prob> m;
  std::map<X, int> labels;
  std::vector<X> values;
};

template<typename Prob = double, typename X>
dist<Prob, X> convert_to_pdf(setup<Prob, X> *s, const std::vector<Prob>& v)
{
  dist<Prob, X> d{};
  for (int i = 0; i < v.size(); ++i)
  {
    d.pdf.emplace_back(s->values[i], v[i]);
  }
  d.resort();
  return d;
}

template<typename Prob, typename X, typename F>
setup<Prob, X> *build_matrix(const X& x, const F& f)
{
  std::map<X, int> labels;
  int next_label = 0;
  labels[x] = next_label++;
  std::vector<X> values_to_process;
  values_to_process.push_back(x);
  int next_value_to_process = 0;
  matrix<Prob> m;

  sparse_vector<Prob> row;
  while (next_value_to_process < values_to_process.size())
  {
    dist<Prob, X> r = f(values_to_process[next_value_to_process]);
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

  return new setup{m, labels, values_to_process};
}

template<typename Prob = double, typename X, typename F>
dist<Prob, X> iterate_i(const X& init, const F& f, int n)
{
    dist<Prob, X> r = certainly(init);
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

template<typename Prob = double, typename X, typename F>
dist<Prob, X> iterate_matrix_i(const X &init, const F& f, int n)
{
  setup<Prob, X> *s = build_matrix<Prob>(init, f);
  // dump_matrix(s->m);
  int dim = s->values.size();
  std::vector<Prob> v;
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
