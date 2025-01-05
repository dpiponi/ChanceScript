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
