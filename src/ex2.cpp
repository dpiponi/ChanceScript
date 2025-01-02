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
  void damage(int d) { hit_points = std::max(0, hit_points - d); }
};

struct state;

struct player : public character
{
#if 0
  player damage(int damage) const
  {
    if (damage >= hit_points)
    {
      return player{0};
    }
    else
    {
      return player{std::max(0, hit_points - damage)};
    }
  }
#endif
  ddist<state> do_move(const state &s) const;
};

struct monster : public character
{
#if 0
  monster damage(int damage) const
  {
    if (damage >= hit_points)
    {
      return monster{0};
    }
    else
    {
      return monster{std::max(0, hit_points - damage)};
    }
  }
#endif
  ddist<state> do_move(const state &s) const;
};

struct state
{
  player player1;
  player player2;
  monster monster0;

  auto operator<=>(const state &) const = default;

#if 1
  template <typename Prob = double, typename Attacker, typename Defender>
  static dist<Prob, Defender> do_attackP(const Attacker &player,
                                         const Defender &monster)
  {
    if (player.hit_points > 0)
    {
      return roll(20).and_then(
          [=](int player_hit_roll)
          {
            return player_hit_roll >= 11
                       ? roll(6).transform(
                             [=](int player_damage)
                             {
                               auto new_monster = monster;
                               new_monster.damage(player_damage);
                               return new_monster;
                             })
                       : certainly(monster);
          });
    }
    else
    {
      return certainly(monster);
    }
  }
#endif

#if 1
  template <typename Attacker, typename Defender>
  ddist<state> attacksP(const Attacker &player, Defender(state::*monster)) const
  {
    return updateP(*this, monster, do_attackP(player, this->*monster));
  }
#endif

#if 0
  ddist<state> player_moveP(player(state::*player)) const
  {
    return (this->*player).do_move(*this);
  }

  ddist<state> monster_moveP() const
  {
    return monster0.do_move(*this);
  }
#endif
};

ddist<state> player::do_move(const state &s) const
{
  if (s.monster0.hit_points > 0 && hit_points > 0)
  {
    return s.attacksP(*this, &state::monster0);
  }
  else
  {
    return certainly(s);
  }
}

ddist<state> monster::do_move(const state &s) const
{
  if (s.monster0.hit_points > 0)
  {
    if (s.player1.hit_points > s.player2.hit_points)
    {
      return s.attacksP(*this, &state::player1);
    }
    else
    {
      return s.attacksP(*this, &state::player2);
    }
  }
  else
  {
    return certainly(s);
  }
}

void test6a()
{
  state s{{8}, {8}, {50}};

  ddist<state> r = iterate_matrix_i(
      s,
      [](const state s)
      {
        return s.player1.do_move(s)
            .and_then([](const state &s)
                      { return s.player2.do_move(s); })
            .and_then([](const state &s) { return s.monster0.do_move(s); });
      },
      100);

  //    r.chop(1e-10);

#if 1
  auto q = r.transform(
      [](const state &s)
      { return (s.player1.hit_points > 0) + (s.player2.hit_points > 0); });

  q.dump();
  std::cout << q.pdf[1].prob / q.pdf[2].prob << std::endl;
#endif
#if 0
    for (auto z : r.pdf)
    {
        std::cout << z.value.player1.hit_points << ' '
                  << z.value.player2.hit_points << ' '
                  << z.value.monster.hit_points << ' ' << z.prob << std::endl;
    }
#endif
}

int main()
{
  test6a();
}
