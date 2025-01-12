#include <iostream>
#include <memory>

#include "ChanceScript.h"

auto AppendRoll(FSampler& Sampler, int K, const TDDist<std::vector<int>>& Dist)
{
    std::vector<int> A = Sampler(Dist);
    int B = Sampler(Roll(6));

    if (A.size() < K || B > A.front())
    {
        A.insert(std::lower_bound(A.begin(), A.end(), B), B);
        if (A.size() > K)
        {
            A.erase(A.begin());
        }
    }

    return A;
}

auto RollKeep(int Roll, int Keep)
{
    auto Dist = Certainly(std::vector<int>{});

    for (int R = 0; R < Roll; ++R)
    {
        Dist = MakeDDist(AppendRoll, Keep, Dist);
    }

    return Dist;
}

int main()
{
    auto Dist = RollKeep(100, 4);

    for (auto [Value, Prob] : Dist)
    {
        std::cout << "The probability of "; // << Value;
#if 1
        for (const auto& e : Value)
        {
            std::cout << e << ' ';
        }
#endif
        std::cout << " is " << Prob << std::endl;
    }
}
