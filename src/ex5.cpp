#include <iostream>

#include "ChanceScript.h"

int main()
{
    auto Dist = Roll(6) + Roll(6);

    for (auto [Value, Prob] : Dist)
    {
        std::cout << "The probability of rolling " << Value << " is " << Prob
                  << std::endl;
    }
}
