#pragma once

#include <vector>

template <typename T>
std::ostream& operator<<(std::ostream& OStream, const std::vector<T>& Vector)
{
    for (const T& Element : Vector)
    {
        OStream << Element << " ";
    }
    return OStream;
}
