#include <iostream>

#include "ChanceScript.h"

// See https://www.explainxkcd.com/wiki/index.php/3015:_D%26D_Combinatorics

auto GrabArrows(int NumArrows, int NumCursed, int NumToPick)
{
    using FQuiver = std::vector<bool>;

    FQuiver StartQuiver(NumArrows, false);
    std::fill(StartQuiver.begin(), StartQuiver.begin() + NumCursed, true);

    auto Quivers = Certainly(StartQuiver);

    for (int I = 0; I < NumToPick; ++I)
    {
        Quivers = Quivers.AndThen(
            [](FQuiver Quiver)
            {
                int NumArrowsLeft = Quiver.size();
                return Roll(NumArrowsLeft)
                    .Transform(
                        [&Quiver](const int Pick)
                        {
                            // Remove arrow and replace it with one from end
                            FQuiver NewQuiver = Quiver;
                            NewQuiver[Pick - 1] = NewQuiver.back();
                            NewQuiver.pop_back();
                            return NewQuiver;
                        });
            });
    }

    auto WasACursedOnePicked = Quivers.Transform(
        [NumCursed](const FQuiver& Quiver)
        { return std::count(Quiver.begin(), Quiver.end(), true) < NumCursed; });

    return WasACursedOnePicked;
}

int main()
{
    auto Dist = GrabArrows(10, 5, 2);

    for (auto [Value, Prob] : Dist)
    {
        std::cout << "The probability of " << (Value ? "" : "not ")
                  << "picking a cursed arrow is " << Prob << std::endl;
    }
}
