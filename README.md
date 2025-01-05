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
        std::cout << "The probability of rolling " << Value << " is " << Prob << std::endl;
    }
}
```
