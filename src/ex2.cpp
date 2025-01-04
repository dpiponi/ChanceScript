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
	TDDist<FState> do_move(const FState& s) const;
};

struct FCleric : public FCharacter
{
	TDDist<FState> do_move(const FState& s) const;

	int NumCureLightWounds;
};

struct FOgre : public FCharacter
{
	TDDist<FState> do_move(const FState& s) const;
};

struct FState
{
	FFighter Fighter;
	FCleric	 Cleric;
	FOgre	 Ogre;

	auto operator<=>(const FState&) const = default;

#if 1
	template <typename Prob = double, typename AttackerType, typename DefenderType>
	static TDist<Prob, DefenderType>
	do_attackP(int ToHit, const TDist<Prob, int>& damage_roll,
		const AttackerType& Attacker, const DefenderType& Defender)
	{
		if (Attacker.HitPoints > 0)
		{
			return roll(20).and_then(
				[=](int Attacker_hit_roll) {
					return Attacker_hit_roll >= ToHit
						? damage_roll.transform(
							  [=](int Attacker_damage) {
								  auto new_monster = Defender;
								  new_monster.Damage(Attacker_damage);
								  return new_monster;
							  })
						: certainly(Defender);
				});
		}
		else
		{
			return certainly(Defender);
		}
	}
#endif

#if 1
	template <typename AttackerType, typename DefenderType>
	TDDist<FState> attacksP(int ToHit, const TDDist<int>& damage_roll,
		const AttackerType& player, DefenderType(FState::* Defender)) const
	{
		return updateP(*this, Defender,
			do_attackP(ToHit, damage_roll, player, this->*Defender));
	}
#endif
};

const TDDist<int> d8 = roll(8);

TDDist<FState> FFighter::do_move(const FState& s) const
{
	if (s.Ogre.HitPoints > 0 && HitPoints > 0)
	{
		// Level 2 FFighter to hit AC5: 15
		// Longsword Damage 1d8
		return s.attacksP(15, d8, *this, &FState::Ogre);
	}
	else
	{
		return certainly(s);
	}
}

int		  threshold = 7;
const int max_hp = 8;

TDDist<int> MaceDamage = roll(6) + 1;

TDDist<int> cure_light_wounds_hp = roll(8);

TDDist<FState> FCleric::do_move(const FState& s) const
{
	if (s.Ogre.HitPoints > 0 && HitPoints > 0 && s.Fighter.HitPoints > 0
		&& s.Fighter.HitPoints <= threshold && NumCureLightWounds > 0)
	{
		return cure_light_wounds_hp.transform(
			[&s](int heal) {
				FState s_copy = s;
				--s_copy.Cleric.NumCureLightWounds;
				s_copy.Fighter.HitPoints =
					std::min(max_hp, s_copy.Fighter.HitPoints + heal);
				return s_copy;
			});
	}
	else if (s.Ogre.HitPoints > 0 && HitPoints > 0)
	{
		// Level 2 FCleric to hit AC5: 15+
		// Mace d6+1 Damage
		return s.attacksP(15, MaceDamage, *this, &FState::Ogre);
	}
	else
	{
		return certainly(s);
	}
}

TDDist<int> ogre_damage = roll(10);

TDDist<FState> FOgre::do_move(const FState& s) const
{
	if (s.Ogre.HitPoints > 0)
	{
		if (s.Fighter.HitPoints > s.Cleric.HitPoints)
		{
			return s.attacksP(13, ogre_damage, *this, &FState::Fighter);
		}
		else
		{
			return s.attacksP(13, ogre_damage, *this, &FState::Cleric);
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

		TDDist<FState> r = iterate_matrix_i(
			s,
			[](const FState s) {
				return s.Fighter.do_move(s)
					.and_then([](const FState& s) { return s.Cleric.do_move(s); })
					.and_then([](const FState& s) { return s.Ogre.do_move(s); });
			},
			100);

		//    r.chop(1e-10);

		auto q = r.transform(
			[](const FState& s) { return (s.Fighter.HitPoints > 0) + (s.Cleric.HitPoints > 0); });

		std::cout << "threshold " << t << std::endl;
		q.dump();
		// std::cout << q.pdf[1].prob / q.pdf[2].prob << std::endl;
	}
}

int main()
{
	test();
}
