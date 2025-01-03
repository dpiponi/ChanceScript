#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <type_traits>
#include <vector>

#include "ChanceScript.h"

// Ogre AC 5, 4d+1 HP damage d10 (MM p.75)
//  to hit AC 2: 13
// Cleric 2nd level, to hit AC 5: 15+. (DMG p.74)
// Fighter 2nd level, to hit AC 5: 15+. (DMG p.74)
struct character
{
  int hit_points;

  auto operator<=>(const character &) const = default;
  void damage(int d) { hit_points = std::max(0, hit_points - d); }
};

struct state;

struct fighter : public character
{
  ddist<state> do_move(const state &s) const;
};

struct cleric : public character
{
  ddist<state> do_move(const state &s) const;

  int num_cure_light_wounds;
};

struct monster : public character
{
  ddist<state> do_move(const state &s) const;
};

struct state
{
  fighter player1;
  cleric player2;
  monster monster0;

  auto operator<=>(const state &) const = default;

#if 1
  template <typename Prob = double, typename Attacker, typename Defender>
  static dist<Prob, Defender>
  do_attackP(int to_hit, const dist<Prob, int> &damage_roll,
             const Attacker &player, const Defender &monster)
  {
    if (player.hit_points > 0)
    {
      return roll(20).and_then(
          [=](int player_hit_roll)
          {
            return player_hit_roll >= to_hit
                       ? damage_roll.transform(
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
  ddist<state> attacksP(int to_hit, const ddist<int> &damage_roll,
                        const Attacker &player, Defender(state::*monster)) const
  {
    return updateP(*this, monster,
                   do_attackP(to_hit, damage_roll, player, this->*monster));
  }
#endif
};

const ddist<int> d8 = roll(8);

ddist<state> fighter::do_move(const state &s) const
{
  if (s.monster0.hit_points > 0 && hit_points > 0)
  {
    // Level 2 fighter to hit AC5: 15
    // Longsword damage 1d8
    return s.attacksP(15, d8, *this, &state::monster0);
  }
  else
  {
    return certainly(s);
  }
}

int threshold = 7;
const int max_hp = 8;

ddist<int> mace_damage = roll(6) + 1;

ddist<state> cleric::do_move(const state &s) const
{
  if (s.monster0.hit_points > 0 && hit_points > 0 && s.player1.hit_points > 0
      && s.player1.hit_points <= threshold && num_cure_light_wounds > 0)
  {
    return roll(8).transform(
        [&s](int heal)
        {
          state s_copy = s;
          --s_copy.player2.num_cure_light_wounds;
          s_copy.player1.hit_points =
              std::min(max_hp, s_copy.player1.hit_points + heal);
          return s_copy;
        });
  }
  else if (s.monster0.hit_points > 0 && hit_points > 0)
  {
    // Level 2 cleric to hit AC5: 15+
    // Mace d6+1 damage
    return s.attacksP(15, mace_damage, *this, &state::monster0);
  }
  else
  {
    return certainly(s);
  }
}

ddist<int> ogre_damage = roll(10);

ddist<state> monster::do_move(const state &s) const
{
  if (s.monster0.hit_points > 0)
  {
    if (s.player1.hit_points > s.player2.hit_points)
    {
      return s.attacksP(13, ogre_damage, *this, &state::player1);
    }
    else
    {
      return s.attacksP(13, ogre_damage, *this, &state::player2);
    }
  }
  else
  {
    return certainly(s);
  }
}

void test()
{
  for (int t = 0; t <= 8; ++t)
  {
    threshold = t;

    state s{{9}, {{7}, 2}, {15}};

    ddist<state> r = iterate_matrix_i(
        s,
        [](const state s)
        {
          return s.player1.do_move(s)
              .and_then([](const state &s) { return s.player2.do_move(s); })
              .and_then([](const state &s) { return s.monster0.do_move(s); });
        },
        100);

    //    r.chop(1e-10);

    auto q = r.transform(
        [](const state &s)
        { return (s.player1.hit_points > 0) + (s.player2.hit_points > 0); });

    std::cout << "threshold " << t << std::endl;
    q.dump();
    // std::cout << q.pdf[1].prob / q.pdf[2].prob << std::endl;
  }
}

int main()
{
  test();
}
