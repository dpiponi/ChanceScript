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
	TDDist<FState> do_move(const FState& State) const;
};

struct FCleric : public FCharacter
{
	TDDist<FState> do_move(const FState& State) const;

	int NumCureLightWounds;
};

struct FOgre : public FCharacter
{
	TDDist<FState> do_move(const FState& State) const;
};

struct FState
{
	FFighter Fighter;
	FCleric	 Cleric;
	FOgre	 Ogre1;
	FOgre	 Ogre2;

	auto operator<=>(const FState&) const = default;

	bool GameOver() const
	{
		return Ogre1.HitPoints == 0 && Ogre2.HitPoints == 0 || Fighter.HitPoints == 0 && Cleric.HitPoints == 0;
	}
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

const TDDist<int> SwordDamage = roll(8);

TDDist<FState> FFighter::do_move(const FState& State) const
{
	if (!State.GameOver() && HitPoints > 0)
	{
		if (State.Ogre1.HitPoints > State.Ogre2.HitPoints)
		{
			// Level 2 FFighter to hit AC5: 15
			// Longsword Damage 1d8
			return State.attacksP(15, SwordDamage, *this, &FState::Ogre1);
		}
		else
		{
			return State.attacksP(15, SwordDamage, *this, &FState::Ogre2);
		}
	}
	else
	{
		return certainly(State);
	}
}

int		  threshold = 7;
const int max_hp = 8;

TDDist<int> MaceDamage = roll(6) + 1;

TDDist<int> cure_light_wounds_hp = roll(8);

TDDist<FState> FCleric::do_move(const FState& State) const
{
	if (!State.GameOver() && HitPoints > 0)
	{
		if (State.Fighter.HitPoints <= threshold && NumCureLightWounds > 0)
		{
			return cure_light_wounds_hp.transform(
				[&State](int heal) {
					FState s_copy = State;
					--s_copy.Cleric.NumCureLightWounds;
					s_copy.Fighter.HitPoints =
						std::min(max_hp, s_copy.Fighter.HitPoints + heal);
					return s_copy;
				});
		}
		else if (State.Ogre1.HitPoints > State.Ogre2.HitPoints)
		{
			// Level 2 FCleric to hit AC5: 15+
			// Mace d6+1 Damage
			return State.attacksP(15, MaceDamage, *this, &FState::Ogre1);
		}
		else
		{
			return State.attacksP(15, MaceDamage, *this, &FState::Ogre2);
		}
	}
	else
	{
		return certainly(State);
	}
}

TDDist<int> ogre_damage = roll(10);

TDDist<FState> FOgre::do_move(const FState& State) const
{
	if (!State.GameOver() && HitPoints > 0)
	{
		if (State.Fighter.HitPoints > State.Cleric.HitPoints)
		{
			return State.attacksP(13, ogre_damage, *this, &FState::Fighter);
		}
		else
		{
			return State.attacksP(13, ogre_damage, *this, &FState::Cleric);
		}
	}
	else
	{
		return certainly(State);
	}
}

void test()
{
	for (int t = 0; t <= 8; ++t)
	{
		threshold = t;

		FState State{ { 9 }, { { 7 }, 2 }, { 15 }, { 15 } };

		TDDist<FState> r = iterate_matrix_i(
			State,
			[](const FState State) {
				return State.Fighter.do_move(State)
					.and_then([](const FState& State) { return State.Cleric.do_move(State); })
					.and_then([](const FState& State) { return State.Ogre1.do_move(State); })
					.and_then([](const FState& State) { return State.Ogre2.do_move(State); });
			},
			50);

		//    r.chop(1e-10);

		auto q = r.transform(
			[](const FState& State) { return (State.Fighter.HitPoints > 0) + (State.Cleric.HitPoints > 0); });

		std::cout << "threshold " << t << std::endl;
		q.dump();
		// std::cout << q.pdf[1].prob / q.pdf[2].prob << std::endl;
	}
}

int main()
{
	test();
}
