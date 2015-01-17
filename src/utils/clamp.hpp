#ifndef CLAMP_HPP_
#define CLAMP_HPP_

#include <algorithm>

// Forces the input value between the lower and upper bound specified.
template<class T>
T clamp(T value, T lowerBound, T upperBound)
{
    return std::max(lowerBound, std::min(value, upperBound));
}

#endif