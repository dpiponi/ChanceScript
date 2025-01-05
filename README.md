ChanceScript
------------

ChanceScript is a small C++ library supporting probabilistic programming.  This design is modelled on Haskell monads. Monadic programming isn't the easiest approach to programming in C++ but there is some precedent, for example in the use of `.transform` and `and_then` in the interface for `std::optional`. In fact, this code uses this interface (but renamed to `.Transform` and `.AndThen` as I'm using something close to my employer's style guide even though this is personal code.)

Here is a small example program:

```
#include <iostream>

#include "ChanceScript.h"

int main()
{
    auto Dist = Roll(6);

    for (auto [Value, Prob] : Dist)
    {
        std::cout << "The probability of rolling " << Value << " is " << Prob
                  << std::endl;
    }
}
```

Operators are overloaded so we can sum two rolls like so:

```
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
```

Besides using operator overloads we can transform the elements of a probability distribution using a lambda and it will update the probabilities accordingly:
```
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
```

Under the hood the library needs to explore all possibilities so it can tabulate the results. This leads to a programming challenge. We'd like to be able to write code like so:

```
auto X = Roll(6);
auto Y = Roll(6);
auto Z = F(X, Y);
```
where `F` is some mathematical function. But `X` and `Y` are distributions, not values, so we can't apply ordinary mathematical functions directly. Note in particular that in order to tabulate probabilities the library needs to iterate through all possible values, so somehow the function `F` needs to be called with all 36 combinations of rolls of two dice. There is no way to implement this with code looking like the snippet above. (In Haskell, through the magic of monads the 'remainder' of a block of imperative code is just a function so we can call it as many times as we like with whatever values we like.)

Instead we can write:
```
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
```

A more complex example is starting with some number, say 1000, and counting how many rolls of a die we have to subtract before hitting zero. One challenge here is that if we work iteratively we have two variables: the number we are counting down from and the number of rolls so far. At any step we can't treat these two variables independently as there is a non-trivial joint distribution on the pair. So our iteration involves a distribution on a state of type `std::pair<int, int>` or a custom type. Note, however, that this code is reasonably fast. The number of ways to roll a sequence of dice until we hit zero is unimaginably large but we don't need to explore all of this space to get an answer.
```
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

    auto Dist = certainly(FState{ N, 0 });

    for (int T = 0; T < N; ++T)
    {
        Dist = Dist.AndThen(
            [](const auto& State)
            {
                if (State.N <= 0)
                {
                    return certainly(State);
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
```
