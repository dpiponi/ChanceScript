#include <iostream>
#include <memory>

#include "ChanceScript.h"

int SumOf(FSampler& Sampler, int N)
{
    int Total = 0;

    for (int i = 0; i < N; ++i)
    {
        Total += Sampler(Roll(6));
    }

    return Total;
}

int main()
{
    auto Dist = MakeDDist(SumOf, 6);

    for (auto [Value, Prob] : Dist)
    {
        std::cout << "The probability of " << Value << " is " << Prob
                  << std::endl;
    }
}
