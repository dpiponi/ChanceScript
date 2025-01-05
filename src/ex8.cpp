#include <iostream>

#include "ChanceScript.h"

auto TimeToHitZero(int N)
{
    auto Dist = certainly(std::make_pair(N, 0));

    for (int T = 0; T < N; ++T)
    {
        Dist = Dist.AndThen(
            [](const auto& State)
            {
                auto [N, Count] = State;
                if (N <= 0)
                {
                    return certainly(std::make_pair(0, Count));
                }
                else
                {
                    return Roll(6).Transform(
                        [N, Count](int Value)
                        {
                            return std::make_pair(std::max(0, N - Value),
                                                  Count + 1);
                        });
                }
            });
    }

    return Dist.Transform([](const auto& State) { return std::get<1>(State); });
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
