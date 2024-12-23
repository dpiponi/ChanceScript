#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <type_traits>
#include <vector>

#include "ChanceScript.h"

struct character
{
    int hit_points;

    auto operator<= > (const character &) const = default;
    character damage(int damage) const
    {
        if (damage >= hit_points)
        {
            return character{0};
        }
        else
        {
            return character{hit_points - damage};
        }
    }
};

template <typename Prob = double> dist<Prob, character> attacks(const character &player, const character &monster)
{
    if (player.hit_points > 0)
    {
        return roll(20) >> [=](int player_hit_roll) {
            return player_hit_roll >= 11
                       ? roll(6).and_then([=](int player_damage) { return certainly(monster.damage(player_damage)); })
                       : certainly(monster);
        };
    }
    else
    {
        return certainly(monster);
    }
}

struct state;

struct state
{
    character player;
    character monster;

    auto operator<= > (const state &) const = default;
};

template <typename Prob = double, typename X, typename Y>
dist<Prob, X> update_field(const X &x, Y(X::*field), const dist<Prob, Y> &dy)
{
    return dy >> [=](const Y &y) {
        X x_new = x;
        x_new.*field = y;
        return certainly(x_new);
    };
}

// template<typename Prob = double, typename F>
// auto lift(const F& f)
// {
//   return [](
// }

ddist<state> player_move(const state &s)
{
    auto [player, monster] = s;
    if (player.hit_points > 0 && monster.hit_points > 0)
    {
        return roll(20).and_then([=](int to_hit) {
            if (to_hit >= 11)
            {
                return roll(6).transform([=](int damage) {
                    auto damaged_monster = monster;
                    damaged_monster.hit_points = std::max(0, damaged_monster.hit_points - damage);
                    return state{player, damaged_monster};
                });
            }
            else
            {
                return certainly(s);
            }
        });
    }
    else
    {
        return certainly(s);
    }
}

ddist<state> monster_move(const state &s)
{
    auto [player, monster] = s;
    if (monster.hit_points > 0 && player.hit_points > 0)
    {
        return roll(20).and_then([=](int to_hit) {
            if (to_hit >= 11)
            {
                return roll(6).transform([=](int damage) {
                    auto damaged_player = player;
                    damaged_player.hit_points = std::max(0, damaged_player.hit_points - damage);
                    return state{damaged_player, monster};
                });
            }
            else
            {
                return certainly(s);
            }
        });
    }
    else
    {
        return certainly(s);
    }
}

void test6a()
{
    state s{{6}, {100}};

    ddist r = iterate_matrix_i(s, [](const state s) { return player_move(s).and_then(monster_move); }, 1000);

    for (auto z : r.pdf)
    {
        std::cout << z.value.player.hit_points << ' ' << z.value.monster.hit_points << ' ' << z.prob << std::endl;
    }
}

int main()
{
    test6a();
}
