#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <type_traits>
#include <vector>

#include "ChanceScript.h"

template <typename X, typename Y>
X update(const X &x, Y(X::*y), const Y &y_new)
{
    X x_copy(x);
    x_copy.*y = y_new;
    return x_copy;
}

template <typename P = double, typename X, typename Y>
dist<P, X> updateP(const X &x, Y(X::*y), const dist<P, Y> &ys_new)
{
    return ys_new >> [&](const Y &y_new)
    {
        X x_copy(x);
        x_copy.*y = y_new;
        return certainly(x_copy);
    };
}

struct character
{
    int hit_points;

    auto operator<=>(const character &) const = default;
    character damage(int damage) const
    {
        if (damage >= hit_points)
        {
            return character{0};
        }
        else
        {
            return character{std::max(0, hit_points - damage)};
        }
    }
};

struct state
{
    character player;
    character monster;

    auto operator<=>(const state &) const = default;

    template <typename Prob = double>
    static dist<Prob, character> do_attackP(const character &player,
                                            const character &monster)
    {
        if (player.hit_points > 0)
        {
            return roll(20).and_then(
                [=](int player_hit_roll)
                {
                    return player_hit_roll >= 11
                               ? roll(6).transform(
                                     [=](int player_damage)
                                     { return monster.damage(player_damage); })
                               : certainly(monster);
                });
        }
        else
        {
            return certainly(monster);
        }
    }

    ddist<state> attacksP(character(state::*player),
                          character(state::*monster)) const
    {
        return updateP(*this, monster,
                       do_attackP(this->*player, this->*monster));
    }
};

template <typename Prob = double, typename X, typename Y>
dist<Prob, X> update_field(const X &x, Y(X::*field), const dist<Prob, Y> &dy)
{
    return dy >> [=](const Y &y)
    {
        X x_new = x;
        x_new.*field = y;
        return certainly(x_new);
    };
}

ddist<state> player_move(const state &s)
{
    if (s.monster.hit_points > 0 && s.player.hit_points > 0)
    {
        return s.attacksP(&state::player, &state::monster);
    }
    else
    {
        return certainly(s);
    }
}

ddist<state> monster_move(const state &s)
{
    if (s.monster.hit_points > 0 && s.player.hit_points > 0)
    {
        return s.attacksP(&state::monster, &state::player);
    }
    else
    {
        return certainly(s);
    }
}

void test6a()
{
    state s{{6}, {100}};

    ddist<state> r = iterate_matrix_i(
        s, [](const state s) { return player_move(s).and_then(monster_move); },
        1000);

    for (auto z : r.pdf)
    {
        std::cout << z.value.player.hit_points << ' '
                  << z.value.monster.hit_points << ' ' << z.prob << std::endl;
    }
}

int main()
{
    test6a();
}
