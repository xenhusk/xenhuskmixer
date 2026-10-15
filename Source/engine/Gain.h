#pragma once

#include <cmath>

namespace xenhusk
{

inline float dbToLinear(float db) noexcept
{
    if (db <= -100.0f)
        return 0.0f;

    return std::pow(10.0f, db / 20.0f);
}

inline float linearToDb(float linear) noexcept
{
    if (linear <= 0.0f)
        return -100.0f;

    return 20.0f * std::log10(linear);
}

} // namespace xenhusk
