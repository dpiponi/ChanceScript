#include <iostream>

#include "ChanceScript.h"

auto TimeToHitZero(int N)
{
    struct FState
    {
        int N;
        int Count;

        auto operator<=>(const FState& Other) const = default;
    };

    auto Dist = Certainly(FState{ N, 0 });

    for (int T = 0; T < N; ++T)
    {
        Dist = Dist.AndThen(
            [](const auto& State)
            {
                if (State.N <= 0)
                {
                    return Certainly(State);
                }
                else
                {
                    return Roll(6).Transform(
                        [&State](int Value)
                        {
                            return FState{ std::max(0, State.N - Value),
                                           State.Count + 1 };
                        });
                }
            });
    }

    return Dist.Transform([](const auto& State) { return State.Count; });
}

int main()
{
    const int Start = 1000;
    auto      Dist = TimeToHitZero(Start);

    for (auto [Value, Prob] : Dist)
    {
        std::cout << "The probability of taking " << Value << " steps is "
                  << Prob << std::endl;
    }
}
