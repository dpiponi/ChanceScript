#pragma once

#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <numeric>
#include <type_traits>
#include <vector>

#include "Utilities.h"
#include "Dist.h"

template <typename P = double, typename T> TDist<P, T> certainly(const T& t)
{
    return TDist<P, T>{ { t, 1.0 } };
}

template <typename P = double> TDist<P, int> Roll(int n)
{
    TDist<P, int> result{};
    result.PDF.resize(n);
    for (int i = 0; i < n; ++i)
    {
        result.PDF[i] = FAtom{ i + 1, 1. / n };
    }

    return result;
}

template <typename P = double> TDist<P, int> Roll(int r, int n)
{
    TDist<P, int> d = certainly(0);
    for (int i = 0; i < r; ++i)
    {
        d = d + Roll(n);
    }
    return d;
}

// Keeps lowest
template <typename X, typename Compare>
void insert_and_keep_sorted(std::vector<X>& vec, X newElement, size_t maxSize,
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
        if (pos != vec.end() &&
            (pos - vec.begin() < maxSize || newElement < vec.back()))
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

template <typename X> X sum(const std::vector<X>& v)
{
    return std::accumulate(v.begin(), v.end(), 0);
}

// Roll `Roll` times keeping `keep` highest given by `compare`.
// Eg. if `compare` is `std::greater` it keeps the greatest rolls
// in the usual sense.
template <typename P, typename X, typename Compare = std::greater<X>>
TDist<P, std::vector<X>> highest_n(const TDist<P, X>& dst, int Roll, int keep,
                                   Compare compare = std::greater<X>())
{
    TDist<P, std::vector<X>> so_far(certainly(std::vector<X>{}));

    for (int i = 0; i < Roll; ++i)
    {
        so_far = so_far.AndThen(
            [keep, compare, &dst](const std::vector<X>& v)
            {
                return dst.Transform(
                    [v, keep, compare](const X& x)
                    {
                        std::vector<X> v_copy = v;
                        insert_and_keep_sorted(
                            v_copy,
                            x,
                            keep,
                            compare); //[](const X& a, const X& b) { return a >
                                      // b; });
                        return v_copy;
                    });
            });
    }

    return so_far;
}

#define DO(b) [&]() { b; }();
#define LET(v, e, b) return e.AndThen([&](v) { b; })
#define RETURN(x) return certainly(x)

// Function to concatenate two vectors and return the result
template <typename T>
std::vector<T> concatenate(const std::vector<T>& v1, const std::vector<T>& v2)
{
    std::vector<T> result;
    result.reserve(v1.size() +
                   v2.size()); // Reserve memory to avoid multiple reallocations
    result.insert(result.end(), v1.begin(), v1.end());
    result.insert(result.end(), v2.begin(), v2.end());
    return result;
}

void test1()
{
#if 0
    auto r = DO(
               LET(int x, Roll(6),
               LET(int y, Roll(6),
               RETURN(x + y);
               )));
#else
    auto r = Roll(6) >> [](int x)
    { return Roll(6) >> [=](int y) { return certainly(x + y); }; };
#endif
    for (auto z : r.PDF)
    {
        std::cout << z << std::endl;
    }
}

template <typename P = double, typename T>
TDist<P, std::vector<T>> sequence(const std::vector<TDist<P, T>>& dists,
                                  int starting_from = 0)
{
    if (starting_from >= dists.size())
    {
        return certainly(std::vector<T>{});
    }

    return DO(LET(const T& head,
                  dists[starting_from],
                  LET(const std::vector<T>& tail,
                      sequence(dists, starting_from + 1),
                      RETURN(concatenate(std::vector<T>{ head }, tail)))));
}

template <typename P = double, typename T, typename F>
TDist<P, T> fold(const F& f, const T& init,
                 const std::vector<TDist<P, T>>& dists, int starting_from = 0)
{
    if (starting_from >= dists.size())
    {
        return certainly(init);
    }

    return DO(LET(const T& head,
                  dists[starting_from],
                  return fold(f, f(init, head), dists, starting_from + 1)));
}

namespace cs
{

    class max
    {
    public:
        template <typename X> X operator()(const X& a, const X& b) const
        {
            return a > b ? a : b;
        }
    };

} // namespace cs

template <typename P = double, typename T, typename U, typename F>
TDist<P, T> repeated(const F& f, const T& init, const TDist<P, U>& TDist, int n)
{
    if (n <= 0)
    {
        return certainly(init);
    }

    return repeated(f, init, TDist, n - 1) >> [&](int head)
    { return TDist >> [&](int tail) { return certainly(f(head, tail)); }; };
}

// Roll n times using distribution `d` and fold
// the resutls together using `f`.
template <typename P, typename F, typename T>
TDist<P, T> reduce(int n, const F& f, TDist<P, T>& d)
{
    auto e = d;
    for (int i = 1; i < n; ++i)
    {
        e = e.AndThen(
            [&f, &d](const T& v)
            { return d.Transform([&v, &f](const T& w) { return f(v, w); }); });
    }
    return e;
}

void test2()
{
    std::vector<TDist<double, int>> rolls{ Roll(6), Roll(6), Roll(6), Roll(6) };
    TDist<double, std::vector<int>> r = sequence(rolls);
    for (auto z : r.PDF)
    {
        std::cout << z << std::endl;
    }
}

void test3()
{
    TDist<double, int> r =
        repeated([](int x, int y) { return std::max(x, y); }, 0, Roll(6), 50);
    for (auto z : r.PDF)
    {
        std::cout << z << std::endl;
    }
}

template <typename P = double, typename X, typename F>
TDist<P, X> iterate(const X& init, const F& f, int n)
{
    TDist<double, X> r = certainly(init);
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
        int  x;
        int  y;
        auto operator<=>(const X&) const = default;
    };
    auto r = iterate(
        X{ 0, 0 },
        [](const auto& xy)
        {
            return Roll(6) >> [&xy](int t)
            { return certainly(X{ 10 * xy.x + t, 100 * xy.y + t }); };
        },
        3);
    for (auto z : r.PDF)
    {
        std::cout << z.Value.x << ' ' << z.Value.y << ' ' << z.Prob
                  << std::endl;
    }
}

void test4a()
{
    struct X
    {
        int  x;
        int  y;
        auto operator<=>(const X&) const = default;
    };
    auto r = certainly(X{ 0, 0 });
    for (int i = 0; i < 4; ++i)
    {
        r = r >> [](const auto& xy)
        {
            return Roll(6) >> [&xy](int t)
            { return certainly(X{ 10 * xy.x + t, 100 * xy.y + t }); };
        };
    }
    for (auto z : r.PDF)
    {
        std::cout << z.Value.x << ' ' << z.Value.y << ' ' << z.Prob
                  << std::endl;
    }
}

template <typename P = double, typename F, typename... FArgs>
TDist<P, std::tuple<FArgs...>> iterate_all(const F& f, int n, FArgs... Args)
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
TDist<P, std::tuple<FArgs...>> certainly_all(FArgs... Args)
{
    return certainly(std::tuple<FArgs...>(Args...));
}

void test5()
{
    auto r = iterate_all(
        [](int x, int y)
        {
            return Roll(6) >> [=](int t)
            { return certainly_all(10 * x + t, 100 * y + t); };
        },
        3,
        0,
        0);

    for (auto z : r.PDF)
    {
        std::cout << std::get<0>(z.Value) << ' ' << z.Prob << std::endl;
    }
}

struct Character
{
    int hit_points;

    auto      operator<=>(const Character&) const = default;
    Character damage(int damage) const
    {
        if (damage >= hit_points)
        {
            return Character{ 0 };
        }
        else
        {
            return Character{ hit_points - damage };
        }
    }
};

template <typename P = double>
TDist<P, Character> attacks(const Character& player, const Character& monster)
{
    if (player.hit_points > 0)
    {
        return Roll(20) >> [=](int player_hit_roll)
        {
            return player_hit_roll >= 11 ? Roll(6) >> [=](int player_damage)
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
    Character player{ 6 };
    Character monster{ 100 };

    auto r = iterate_all(
        [](const Character& player, const Character& monster)
        {
            return attacks(player, monster) >> [=](const Character& monster)
            {
                return attacks(monster, player) >> [=](const Character& player)
                { return certainly_all(player, monster); };
            };
        },
        100,
        player,
        monster);
    for (auto z : r.PDF)
    {
        std::cout << std::get<0>(z.Value).hit_points << ' '
                  << std::get<1>(z.Value).hit_points << ' ' << z.Prob
                  << std::endl;
    }
}

// Roll d6 `r` times but keep top `k`.
template <typename P = double>
TDist<P, std::vector<int>> roll_keep(int r, int k)
{
    if (r == 0)
    {
        return certainly(std::vector<int>{});
    }
    else
    {
        return roll_keep(r - 1, k) >> [=](const std::vector<int>& rolls)
        {
            return Roll(6) >> [=](int n)
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

    int sum(const std::vector<int>& a)
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
    auto r = roll_keep(8, 4).Transform(cs::sum);
    for (auto z : r.PDF)
    {
        std::cout << z.Value << ' ' << z.Prob << std::endl;
    }
}

#if 0
int main()
{
  test7();
}
#endif

template <typename P, typename T>
bool subdist(const TDist<P, T>& a, const TDist<P, T>& b)
{
    int i = 0;
    int j = 0;
    while (i < a.PDF.size() && j < b.PDF.size())
    {
        if (a.PDF[i].Value < b.PDF[i].Value)
        {
            return false;
        }
        ++i;
        ++j;
    }
    return i < a.PDF.size();
}

// using Prob = double;
template <typename P> using SparseVector = std::vector<std::pair<int, P>>;
template <typename P> using TMatrix = std::vector<SparseVector<P>>;

#include "Matrix.h"

template <typename P>
P dot_sparse_dense(const SparseVector<P>& s, const std::vector<P>& d)
{
    P t = 0.0;
    for (const auto [i, x] : s)
    {
        t += x * d[i];
    }
    return t;
}

template <typename P>
std::vector<P> DotMatrixVector(const TMatrix<P>& m, const std::vector<P>& d)
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

template <typename P> void dump_vector(const std::vector<P>& v)
{
    for (int i = 0; i < v.size(); ++i)
    {
        std::cout << v[i] << ' ';
    }
    std::cout << std::endl;
}

template <typename P> void dump_matrix(const TMatrix<P>& m)
{
    for (int i = 0; i < m.size(); ++i)
    {
        std::cout << i << ':' << std::endl;
        for (auto& [label, Prob] : m[i])
        {
            std::cout << " (" << label << "," << Prob << ')';
        }
        std::cout << std::endl;
    }
}

template <typename P, typename X> struct setup
{
    TMatrix<P>       m;
    std::map<X, int> labels;
    std::vector<X>   values;
};

template <typename P = double, typename X>
TDist<P, X> convert_to_pdf(setup<P, X>* s, const std::vector<P>& v)
{
    TDist<P, X> d{};
    for (int i = 0; i < v.size(); ++i)
    {
        d.PDF.emplace_back(s->values[i], v[i]);
    }
    d.Sort();
    d.remove_zero();

    return d;
}

template <typename P, typename X, typename F>
setup<P, X>* BuildMatrix(const X& x, const F& f)
{
    std::map<X, int> labels;
    int              next_label = 0;
    labels[x] = next_label++;
    std::vector<X> values_to_process;
    values_to_process.push_back(x);
    int        next_value_to_process = 0;
    TMatrix<P> m;

    SparseVector<P> row;
    while (next_value_to_process < values_to_process.size())
    {
        TDist<P, X> r = f(values_to_process[next_value_to_process]);
        row.clear();
        for (auto& [Value, Prob] : r.PDF)
        {
            if (labels.contains(Value))
            {
                int label = labels[Value];
                row.emplace_back(label, Prob);
            }
            else
            {
                labels[Value] = next_label;
                row.emplace_back(next_label, Prob);
                values_to_process.push_back(Value);
                ++next_label;
            }
            std::sort(row.begin(),
                      row.end(),
                      [](const std::pair<int, P>& a, const std::pair<int, P>& b)
                      { return a.first < b.first; });
        }
        m.push_back(row);
        ++next_value_to_process;
    }

    return new setup{ m, labels, values_to_process };
}

template <typename P> P MinDiagonal(const TMatrix<P>& Matrix)
{
    int Dim = Matrix.size();
    P   Minimum = 1.0;

    for (int RowNumber = 0; RowNumber < Dim; ++RowNumber)
    {
        const SparseVector<P>& Row = Matrix[RowNumber];
        auto DiagonalElement = std::lower_bound(
            Row.begin(),
            Row.end(),
            RowNumber,
            [](const std::pair<int, P>& a, int b) { return a.first < b; });

        if (DiagonalElement != Row.end() && DiagonalElement->first == RowNumber)
        {
            Minimum = std::min(Minimum, DiagonalElement->second);
        }
        else
        {
            return 0;
        }
    }
    return Minimum;
}

template <typename P = double, typename X, typename F>
TDist<P, X> iterate_i(const X& init, const F& f, int n)
{
    TDist<P, X> r = certainly(init);
    for (int i = 0; i < n; ++i)
    {
        const auto old_r = r;
        r = old_r >> f;
        std::cout << old_r.PDF.size() << '/' << r.PDF.size() << std::endl;
        if (subdist(r, old_r))
        {
            std::cout << "Explored" << std::endl;
        }
    }
    return r;
}

template <typename P = double, typename X, typename F>
TDist<P, X> iterate_matrix_i(const X& init, const F& f, int n)
{
    setup<P, X>* s = BuildMatrix<P>(init, f);
    std::cout << "Made TMatrix" << std::endl;
    std::cout << "Matrix min = " << MinDiagonal(s->m) << std::endl;
    // dump_matrix(s->m);
    int            dim = s->values.size();
    std::vector<P> v;
    v.resize(dim);
    std::fill(v.begin(), v.end(), 0);
    v[0] = 1;
    for (int i = 0; i < n; ++i)
    {
        v = DotMatrixVector(s->m, v);
    }
    auto p = convert_to_pdf(s, v);
    return p;
}

template <typename P = double, typename X, typename F>
TDist<P, X> iterate_matrix_inf(const X& init, const F& f)
{
    setup<P, X>* s = BuildMatrix<P>(init, f);
    std::cout << "Made TMatrix" << std::endl;
    std::cout << "Matrix min = " << MinDiagonal(s->m) << std::endl;
    // dump_matrix(s->m);
    int            dim = s->values.size();
    std::vector<P> v;
    v.resize(dim);
    std::fill(v.begin(), v.end(), 0);
    v[0] = 1;
    v = Solve(s->m, v);
#if 0
    for (int i = 0; i < n; ++i)
    {
        v = DotMatrixVector(s->m, v);
    }
#endif
    auto p = convert_to_pdf(s, v);
    return p;
}

template <typename X, typename Y>
X update(const X& x, Y(X::* y), const Y& y_new)
{
    X x_copy(x);
    x_copy.*y = y_new;
    return x_copy;
}

template <typename P = double, typename X, typename Y>
TDist<P, X> UpdateFieldP(const X& x, Y(X::* y), const TDist<P, Y>& ys_new)
{
    return ys_new.Transform(
        [&](const Y& y_new)
        {
            X x_copy(x);
            x_copy.*y = y_new;
            return x_copy;
        });
}
