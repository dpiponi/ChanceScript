#include <iostream>

#include "ChanceScript.h"

int main()
{
    auto Dist = Roll(6).Transform([](int Value) { return Value / 2; });

    for (auto [Value, Prob] : Dist)
    {
        std::cout << "The probability of rolling " << Value << " is " << Prob
                  << std::endl;
    }
}
