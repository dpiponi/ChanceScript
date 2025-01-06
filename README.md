ChanceScript
------------

ChanceScript is a small C++ library supporting probabilistic programming.  This design is modelled on Haskell monads. Monadic programming isn't the easiest approach to programming in C++ but there is some precedent, for example in the use of `.transform` and `and_then` in the interface for `std::optional`. In fact, this code uses this interface (but renamed to `.Transform` and `.AndThen` as I'm using something close to Epic Games style.)

It's not clear C++ is a reasonable language for this as it stands so consider this all very experimental...

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
where `F` is some mathematical function. But `X` and `Y` are distributions, not values, so we can't apply ordinary mathematical functions directly. Note in particular that in order to tabulate probabilities the library needs to iterate through all possible values, so somehow the function `F` needs to be called with all 36 combinations of rolls of two dice. There is no way to implement this with code looking like the snippet above. (In Haskell, through the magic of monads, the 'remainder' of a block of imperative code is just a function so we can call it as many times as we like with whatever values we like.)

In order to chain random operations we use `.AndThen()`. This is similar to `.Transform()` except that for each of the values passed into the provided lambda a _distribution_ is returned. There is also a special distribution that is concentrated on one value, `Certainly(Value)`. This means that the following code snippets are equivalent:

```
Dist.Transform([](...)
{
    ...;
    return Result;
})
```
```
Dist.AndThen([](...)
{
    ...;
    return Certainly(Result);
}
```
But of course `.AndThen()` can return any distribution, not just `Certainly(...)`.

Note that `.AndThen()` corresponds to Haskell's `>>=`, `.Transform()` corresponds to `fmap` and `Certainly` corresponds to `return`.

We can now write:

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

Note that internally the `TDist<>` template keeps elements sorted so we need a definition of the spaceship operator `<=>`.
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
```

Here's an example showing how it is possible to write simulations:
```
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
```

NOTES
-----
I have much faster code for certain operations that isn't yet incorporated into this library:

1. Faster addition of random variables using convolution
2. Much faster `min` or `max` of random variables by multiplying CDFs.

There is also unpolished code for special operations like computing the result of certain simulations with absorbing states in the limit as the number of steps goes to infinity.

In almost every case it isn't hard to hand-craft code that solves the same problem maybe 10,000x faster. For example compare the random walk with [code I wrote at Google](https://github.com/tensorflow/probability/blob/main/tensorflow_probability/python/experimental/marginalize/marginalizable_test.py). But the important thing about this code is that it's a *forward* simulation whereas hand-crafted code tends to be written backwards making it hard to leverage existing libraries.

See `src/ex2.cpp` for a complex example simulating an AD&D 1e combat involving a spellcasting cleric and fighter against a pair of ogres.
