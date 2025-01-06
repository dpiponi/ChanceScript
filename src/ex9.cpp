#include <iostream>

#include "ChanceScript.h"

auto RandomWalk(int N)
{
    struct FState
    {
        int X;
        int Y;

        auto operator<=>(const FState& Other) const = default;
    };

    auto Dist = Certainly(FState{ 0, 0 });

    for (int T = 0; T < N; ++T)
    {
        Dist = Dist.AndThen(
            [](const auto& State)
            {
                return Roll(4).Transform(
                    [&State](int Direction)
                    {
                        switch (Direction)
                        {
                            case 1:
                                return FState{ State.X - 1, State.Y };
                            case 2:
                                return FState{ State.X + 1, State.Y };
                            case 3:
                                return FState{ State.X, State.Y - 1 };
                            case 4:
                                return FState{ State.X, State.Y + 1 };
                            default:
                                std::unreachable();
                        }
                    });
            });
    }

    return Dist;
}

int main()
{
    const int Start = 100;
    auto      Dist = RandomWalk(Start);

    for (auto [Value, Prob] : Dist)
    {
        std::cout << "The probability of ending at (" << Value.X << ','
                  << Value.Y << ") is " << Prob << std::endl;
    }
}
