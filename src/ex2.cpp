#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <type_traits>
#include <vector>

#include "ChanceScript.h"

struct FState;

const TDDist<int> SwordDamage = Roll(8);
const TDDist<int> MaceDamage = Roll(6) + 1;
const TDDist<int> OgreDamage = Roll(10);

const int FighterToHitAC5 = 15;
const int ClericToHitAC5 = 15;

const int FighterFullHP = 16;
const int ClericFullHP = 12;

TDDist<int> CureLightWoundsHP = Roll(8);

// When does cleric self-heal?
const int CureThreshold = 7;

// Ogre AC 5, 4d+1 HP Damage d10 (MM p.75)
//  to hit AC 2: 13
// Cleric 2nd level, to hit AC 5: 15+. (DMG p.74)
// Fighter 2nd level, to hit AC 5: 15+. (DMG p.74)
struct FCharacter
{
    bool IsAlive() const { return HitPoints > 0; }

    int HitPoints;

public:
    FCharacter(int InHitPoints) : HitPoints(InHitPoints) {}

    auto operator<=>(const FCharacter&) const = default;

    void ReceiveDamage(int Damage)
    {
        HitPoints = std::max(0, HitPoints - Damage);
    }

    virtual TDDist<FState> DoMove(const FState& State) const = 0;
};

struct FFighter : public FCharacter
{
    FFighter(int InHitPoints) : FCharacter(InHitPoints) {}

    auto operator<=>(const FFighter&) const = default;

    TDDist<FState> DoMove(const FState& State) const override;
};

struct FCleric : public FCharacter
{
    FCleric(int InHitPoints, int InNumCureLightWounds)
        : FCharacter(InHitPoints), NumCureLightWounds(InNumCureLightWounds)
    {
    }

    TDDist<FState> DoMove(const FState& State) const override;

    auto operator<=>(const FCleric&) const = default;

    int NumCureLightWounds;
};

struct FOgre : public FCharacter
{
    FOgre(int InHitPoints) : FCharacter(InHitPoints) {}

    auto operator<=>(const FOgre&) const = default;

    TDDist<FState> DoMove(const FState& State) const override;
};

// XXX
// Work on dividing up responsibilities between characters and state.
struct FState
{
    FFighter Fighter;
    FCleric  Cleric;
    FOgre    Ogre1;
    FOgre    Ogre2;

    auto operator<=>(const FState&) const = default;

    bool GameOver() const
    {
        return !Ogre1.IsAlive() && !Ogre2.IsAlive() ||
               !Fighter.IsAlive() && !Cleric.IsAlive();
    }

    template<typename Prob, typename DefenderType>
    static TDist<Prob, DefenderType> DoHitP(const TDist<Prob, int>& DamageRoll,
                                            const FCharacter&       Attacker,
                                            const DefenderType&     Defender)
    {
        return DamageRoll.Transform(
            [&Defender](int AttackerDamage)
            {
                DefenderType DefenderCopy = Defender;
                DefenderCopy.ReceiveDamage(AttackerDamage);
                return DefenderCopy;
            });
    }

    template<typename Prob, typename DefenderType>
    static TDist<Prob, DefenderType>
    DoAttackP(int ToHit, const TDist<Prob, int>& DamageRoll,
              const FCharacter& Attacker, const DefenderType& Defender)
    {
        if (Attacker.HitPoints > 0)
        {
            // Note it is more efficient to perform the `Roll(2) >= ToHit` first
            // than put the comparison inside the `.AndThen()` as this results
            // in the lambda in `.AndThen()` being called just twice instead
            // of 20 times. Always reduce the space as early as possible.
            return (Roll(20) >= ToHit)
                .AndThen(
                    [&DamageRoll, &Attacker, &Defender](
                        const bool bAttackerHits)
                    {
                        return bAttackerHits
                                   ? DoHitP(DamageRoll, Attacker, Defender)
                                   : Certainly(Defender);
                    });
        }
        else
        {
            return Certainly(Defender);
        }
    }

    template<typename AttackerType, typename DefenderType>
    TDDist<FState> AttacksP(int ToHit, const TDDist<int>& DamageRoll,
                            const AttackerType& Attacker,
                            DefenderType(FState::* Defender)) const
    {
        TDDist<DefenderType> AttackedDefender =
            DoAttackP(ToHit, DamageRoll, Attacker, this->*Defender);
        return UpdateFieldP(*this, Defender, AttackedDefender);
    }
};

TDDist<FState> FFighter::DoMove(const FState& State) const
{
    if (!State.GameOver() && HitPoints > 0)
    {
        if (!State.Ogre2.IsAlive() ||
            State.Ogre1.IsAlive() &&
                State.Ogre1.HitPoints < State.Ogre2.HitPoints)
        {
            return State.AttacksP(
                FighterToHitAC5, SwordDamage, *this, &FState::Ogre1);
        }
        else
        {
            return State.AttacksP(
                FighterToHitAC5, SwordDamage, *this, &FState::Ogre2);
        }
    }
    else
    {
        return Certainly(State);
    }
}

TDDist<FState> FCleric::DoMove(const FState& State) const
{
    if (!State.GameOver() && HitPoints > 0)
    {
        if (State.Cleric.HitPoints > 0 &&
            State.Cleric.HitPoints <= CureThreshold && NumCureLightWounds > 0)
        {
            // Cure
            return CureLightWoundsHP.Transform(
                [State](int WoundsHealed)
                {
                    FState StateCopy = State;
                    --StateCopy.Cleric.NumCureLightWounds;
                    StateCopy.Cleric.HitPoints =
                        std::min(ClericFullHP,
                                 StateCopy.Cleric.HitPoints + WoundsHealed);
                    return StateCopy;
                });
        }
        else if (State.Ogre2.HitPoints == 0 ||
                 State.Ogre1.HitPoints > 0 &&
                     State.Ogre1.HitPoints < State.Ogre2.HitPoints)
        {
            return State.AttacksP(
                ClericToHitAC5, MaceDamage, *this, &FState::Ogre1);
        }
        else
        {
            return State.AttacksP(
                ClericToHitAC5, MaceDamage, *this, &FState::Ogre2);
        }
    }
    else
    {
        return Certainly(State);
    }
}

TDDist<FState> FOgre::DoMove(const FState& State) const
{
    if (!State.GameOver() && HitPoints > 0)
    {
        if (State.Cleric.HitPoints == 0 ||
            State.Fighter.HitPoints > 0 &&
                State.Fighter.HitPoints < State.Cleric.HitPoints)
        {
            return State.AttacksP(13, OgreDamage, *this, &FState::Fighter);
        }
        else
        {
            return State.AttacksP(13, OgreDamage, *this, &FState::Cleric);
        }
    }
    else
    {
        return Certainly(State);
    }
}

void test()
{
    FState State{
        FFighter(FighterFullHP), FCleric(ClericFullHP, 2), FOgre(19), FOgre(19)
    };

#if 1
    TDDist<FState> r = iterate_matrix_i(
        State,
        [](const FState State)
        {
            return State.Fighter.DoMove(State)
                .AndThen([](const FState& State)
                         { return State.Cleric.DoMove(State); })
                .AndThen([](const FState& State)
                         { return State.Ogre1.DoMove(State); })
                .AndThen([](const FState& State)
                         { return State.Ogre2.DoMove(State); });
        },
        50);
#else
    TDDist<FState> r = iterate_matrix_inf(
        State,
        [](const FState State)
        {
            return State.Fighter.DoMove(State)
                .AndThen([](const FState& State)
                         { return State.Cleric.DoMove(State); })
                .AndThen([](const FState& State)
                         { return State.Ogre1.DoMove(State); })
                .AndThen([](const FState& State)
                         { return State.Ogre2.DoMove(State); });
        });
#endif

    //    r.chop(1e-10);

    auto q = r.Transform(
        [](const FState& State)
        { return State.Fighter.IsAlive() + (State.Cleric.IsAlive()); });

    std::cout << "How many players survive after 50 rounds?" << std::endl;

    q.dump();
}

int main()
{
    test();
}
