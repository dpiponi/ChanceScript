#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <type_traits>
#include <vector>

#include "ChanceScript.h"

// Ogre AC 5, 4d+1 HP Damage d10 (MM p.75)
//  to hit AC 2: 13
// Cleric 2nd level, to hit AC 5: 15+. (DMG p.74)
// Fighter 2nd level, to hit AC 5: 15+. (DMG p.74)
struct FCharacter
{
	int HitPoints;

	auto operator<=>(const FCharacter&) const = default;
	void Damage(int d) { HitPoints = std::max(0, HitPoints - d); }
};

struct FState;

struct FFighter : public FCharacter
{
	ddist<FState> do_move(const FState& s) const;
};

struct FCleric : public FCharacter
{
	ddist<FState> do_move(const FState& s) const;

	int NumCureLightWounds;
};

struct FOgre : public FCharacter
{
	ddist<FState> do_move(const FState& s) const;
};

struct FState
{
	FFighter player1;
	FCleric	player2;
	FOgre monster0;

	auto operator<=>(const FState&) const = default;

#if 1
	template <typename Prob = double, typename Attacker, typename Defender>
	static dist<Prob, Defender>
	do_attackP(int to_hit, const dist<Prob, int>& damage_roll,
		const Attacker& player, const Defender& FOgre)
	{
		if (player.HitPoints > 0)
		{
			return roll(20).and_then(
				[=](int player_hit_roll) {
					return player_hit_roll >= to_hit
						? damage_roll.transform(
							  [=](int player_damage) {
								  auto new_monster = FOgre;
								  new_monster.Damage(player_damage);
								  return new_monster;
							  })
						: certainly(FOgre);
				});
		}
		else
		{
			return certainly(FOgre);
		}
	}
#endif

#if 1
	template <typename Attacker, typename Defender>
	ddist<FState> attacksP(int to_hit, const ddist<int>& damage_roll,
		const Attacker& player, Defender(FState::* FOgre)) const
	{
		return updateP(*this, FOgre,
			do_attackP(to_hit, damage_roll, player, this->*FOgre));
	}
#endif
};

const ddist<int> d8 = roll(8);

ddist<FState> FFighter::do_move(const FState& s) const
{
	if (s.monster0.HitPoints > 0 && HitPoints > 0)
	{
		// Level 2 FFighter to hit AC5: 15
		// Longsword Damage 1d8
		return s.attacksP(15, d8, *this, &FState::monster0);
	}
	else
	{
		return certainly(s);
	}
}

int		  threshold = 7;
const int max_hp = 8;

ddist<int> mace_damage = roll(6) + 1;

ddist<int> cure_light_wounds_hp = roll(8);

ddist<FState> FCleric::do_move(const FState& s) const
{
	if (s.monster0.HitPoints > 0 && HitPoints > 0 && s.player1.HitPoints > 0
		&& s.player1.HitPoints <= threshold && NumCureLightWounds > 0)
	{
		return cure_light_wounds_hp.transform(
			[&s](int heal) {
				FState s_copy = s;
				--s_copy.player2.NumCureLightWounds;
				s_copy.player1.HitPoints =
					std::min(max_hp, s_copy.player1.HitPoints + heal);
				return s_copy;
			});
	}
	else if (s.monster0.HitPoints > 0 && HitPoints > 0)
	{
		// Level 2 FCleric to hit AC5: 15+
		// Mace d6+1 Damage
		return s.attacksP(15, mace_damage, *this, &FState::monster0);
	}
	else
	{
		return certainly(s);
	}
}

ddist<int> ogre_damage = roll(10);

ddist<FState> FOgre::do_move(const FState& s) const
{
	if (s.monster0.HitPoints > 0)
	{
		if (s.player1.HitPoints > s.player2.HitPoints)
		{
			return s.attacksP(13, ogre_damage, *this, &FState::player1);
		}
		else
		{
			return s.attacksP(13, ogre_damage, *this, &FState::player2);
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

		FState s{ { 9 }, { { 7 }, 2 }, { 15 } };

		ddist<FState> r = iterate_matrix_i(
			s,
			[](const FState s) {
				return s.player1.do_move(s)
					.and_then([](const FState& s) { return s.player2.do_move(s); })
					.and_then([](const FState& s) { return s.monster0.do_move(s); });
			},
			100);

		//    r.chop(1e-10);

		auto q = r.transform(
			[](const FState& s) { return (s.player1.HitPoints > 0) + (s.player2.HitPoints > 0); });

		std::cout << "threshold " << t << std::endl;
		q.dump();
		// std::cout << q.pdf[1].prob / q.pdf[2].prob << std::endl;
	}
}

int main()
{
	test();
}
