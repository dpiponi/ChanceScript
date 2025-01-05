#include <iostream>

#include "ChanceScript.h"

int F(const int X, const int Y)
{
    return 10 * X + Y;
}

int main()
{
    auto Dist = Roll(6).AndThen(
        [](const int FirstRoll)
        {
            return Roll(6).Transform([FirstRoll](const int SecondRoll)
                                     { return F(FirstRoll, SecondRoll); });
        });

    for (auto [Value, Prob] : Dist)
    {
        std::cout << "The probability of rolling " << Value << " is " << Prob
                  << std::endl;
    }
}
