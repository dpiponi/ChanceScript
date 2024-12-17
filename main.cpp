#include <iostream>
#include <functional>
#include <vector>
#include <type_traits>
#include <algorithm>

template<typename T>
struct Atom
{
  T value;
  double prob;

  auto operator<=>(const Atom& other) const
  {
    return value <=> other.value;
  }
};

template<typename T>
struct Dist;

template<typename T, typename F>
std::invoke_result_t<F, T> then(const Dist<T> &m, const F& f);

template<typename T>
struct Dist
{
  Dist(std::initializer_list<Atom<T>> pdf_in) : pdf(pdf_in) { }

  std::vector<Atom<T>> pdf;

  template<typename F>
  std::invoke_result_t<F, T> operator>>(const F& f) const
  {
    return then(*this, f);
  }
};

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec)
{
    for (const T& elem : vec) {
        os << elem << " ";
    }
    return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const Atom<T>& atom)
{
    os << "Atom(value: " << atom.value << ", prob: " << atom.prob << ")";
    return os;
}

template <typename T>
void sortAndCombine(std::vector<Atom<T>>& atoms) {
    std::sort(atoms.begin(), atoms.end(), [](const Atom<T>& a, const Atom<T>& b) {
        return a.value < b.value;
    });

    std::vector<Atom<T>> combined;
    for (const auto& atom : atoms) {
        if (!combined.empty() && combined.back().value == atom.value) {
            combined.back().prob += atom.prob;
        } else {
            combined.push_back(atom);
        }
    }

    atoms = std::move(combined);
}

template<typename T, typename F>
std::invoke_result_t<F, T> then(const Dist<T> &m, const F& f)
{
    std::invoke_result_t<F, T> result{};
    for (const auto& x : m.pdf)
    {
        auto fx = f(x.value);
        double prob = x.prob;
        for (auto& r : fx.pdf)
        {
            result.pdf.push_back(Atom{r.value, prob * r.prob});
        }
    }

    //std::sort(result.begin(), result.end());
    sortAndCombine(result.pdf);
    return result;
}

Dist<int> roll(int n)
{
  Dist<int> result{};
  result.pdf.resize(n);
  for (int i = 0; i < n; ++i)
  {
    result.pdf[i] = Atom{i + 1, 1. / n};
  }

  return result;
}

template<typename T>
Dist<T> certainly(const T& t)
{
  return Dist<T>{{t, 1.0}};
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
    auto r = roll(6) >> [](int x)
             {
               return roll(6) >> [=](int y)
               {
                 return certainly(x + y);
               };
             };
#endif
    for (auto z : r.pdf)
    {
        std::cout << z << std::endl;
    }
}

template<typename T>
Dist<std::vector<T>> sequence(const std::vector<Dist<T>>& dists, int starting_from = 0)
{
  if (starting_from >= dists.size())
  {
    return certainly(std::vector<T>{});
  }

  return DO(
    LET(const T& head, dists[starting_from],
    LET(const std::vector<T>& tail, sequence(dists, starting_from + 1),
    RETURN(concatenate(std::vector<T>{head}, tail)))));
}

template<typename T, typename F>
Dist<T> fold(const F& f, const T& init, const std::vector<Dist<T>>& dists, int starting_from = 0)
{
  if (starting_from >= dists.size())
  {
    return certainly(init);
  }

  return DO(
    LET(const T& head, dists[starting_from],
    return fold(f, f(init, head), dists, starting_from + 1)));
}

template<typename T, typename F>
Dist<T> repeated(const F& f, const T& init, const Dist<T>& dist, int n)
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
    std::vector<Dist<int>> rolls{roll(6), roll(6), roll(6), roll(6)};
    Dist<std::vector<int>> r = sequence(rolls);
    for (auto z : r.pdf)
    {
        std::cout << z << std::endl;
    }
}

void test3()
{
    Dist<int> r = repeated([](int x, int y) { return std::max(x, y); }, 0, roll(6), 50);
    for (auto z : r.pdf)
    {
        std::cout << z << std::endl;
    }
}

template<typename X, typename F>
Dist<X> iterate(const X& init, const F& f, int n)
{
    if (n == 0)
    {
        return certainly(init);
    }
    return iterate(init, f, n - 1) >> f;
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

template<typename F, typename... FArgs>
Dist<std::tuple<FArgs...>> iterate_all(const F& f, int n, FArgs... Args)
{
    if (n == 0)
    {
        return certainly(std::tuple<FArgs...>(Args...));
    }
    return iterate_all(f, n - 1, Args...) >> [f](const std::tuple<FArgs...> ArgsTuple) { return std::apply(f, ArgsTuple); };
}

template<typename... FArgs>
Dist<std::tuple<FArgs...>> certainly_all(FArgs... Args)
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

void test6()
{
  Character player{10};
  Character monster{10};

    auto r = iterate_all(
      [](const Character& player, const Character& monster)
      {
          return roll(20) >> [=](int player_hit_roll)
          {
            if (player_hit_roll >= 11)
            {
              return roll(6) >> [=](int player_damage)
              {
                return certainly_all(player, monster.damage(player_damage));
              };
            }
            else
            {
              return certainly_all(player, monster);
            }
          };
      },
      10,
      player, monster
      );
    for (auto z : r.pdf)
    {
        std::cout << std::get<0>(z.value).hit_points << ' ' << std::get<1>(z.value).hit_points << ' '<< z.prob << std::endl;
    }
}

int main()
{
  test6();
}
