#pragma once

#include <algorithm>
#include <cmath>

namespace xenhusk
{

struct Biquad
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;

    void update(int type, double sampleRate, float frequency, float gainDb, float q) noexcept
    {
        const double rate = sampleRate > 0.0 ? sampleRate : 48000.0;
        const double freq = std::clamp((double) frequency, 20.0, rate * 0.45);
        const double quality = std::clamp((double) q, 0.1, 20.0);
        const double w0 = 2.0 * 3.14159265358979323846 * freq / rate;
        const double cosW = std::cos(w0);
        const double sinW = std::sin(w0);
        const double alpha = sinW / (2.0 * quality);
        const double A = std::pow(10.0, (double) gainDb / 40.0);

        double nb0 = 1.0, nb1 = 0.0, nb2 = 0.0, na0 = 1.0, na1 = 0.0, na2 = 0.0;

        if (type == 4)
        {
            nb0 = (1.0 + cosW) * 0.5;
            nb1 = -(1.0 + cosW);
            nb2 = (1.0 + cosW) * 0.5;
            na0 = 1.0 + alpha;
            na1 = -2.0 * cosW;
            na2 = 1.0 - alpha;
        }
        else if (type == 3)
        {
            nb0 = (1.0 - cosW) * 0.5;
            nb1 = 1.0 - cosW;
            nb2 = (1.0 - cosW) * 0.5;
            na0 = 1.0 + alpha;
            na1 = -2.0 * cosW;
            na2 = 1.0 - alpha;
        }
        else if (type == 1 || type == 2)
        {
            const double shelf = 1.0;
            const double shelfAlpha = sinW * 0.5 * std::sqrt((A + 1.0 / A) * (1.0 / shelf - 1.0) + 2.0);
            const double twoSqrt = 2.0 * std::sqrt(A) * shelfAlpha;
            if (type == 1)
            {
                nb0 = A * ((A + 1.0) - (A - 1.0) * cosW + twoSqrt);
                nb1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosW);
                nb2 = A * ((A + 1.0) - (A - 1.0) * cosW - twoSqrt);
                na0 = (A + 1.0) + (A - 1.0) * cosW + twoSqrt;
                na1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosW);
                na2 = (A + 1.0) + (A - 1.0) * cosW - twoSqrt;
            }
            else
            {
                nb0 = A * ((A + 1.0) + (A - 1.0) * cosW + twoSqrt);
                nb1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosW);
                nb2 = A * ((A + 1.0) + (A - 1.0) * cosW - twoSqrt);
                na0 = (A + 1.0) - (A - 1.0) * cosW + twoSqrt;
                na1 = 2.0 * ((A - 1.0) - (A + 1.0) * cosW);
                na2 = (A + 1.0) - (A - 1.0) * cosW - twoSqrt;
            }
        }
        else
        {
            nb0 = 1.0 + alpha * A;
            nb1 = -2.0 * cosW;
            nb2 = 1.0 - alpha * A;
            na0 = 1.0 + alpha / A;
            na1 = -2.0 * cosW;
            na2 = 1.0 - alpha / A;
        }

        const double inv = na0 == 0.0 ? 1.0 : 1.0 / na0;
        b0 = (float) (nb0 * inv);
        b1 = (float) (nb1 * inv);
        b2 = (float) (nb2 * inv);
        a1 = (float) (na1 * inv);
        a2 = (float) (na2 * inv);
    }

    float process(float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    float magnitudeDb(float frequency, double sampleRate) const noexcept
    {
        const double w = 2.0 * 3.14159265358979323846 * (double) frequency / sampleRate;
        const double c1 = std::cos(w);
        const double s1 = std::sin(w);
        const double c2 = std::cos(2.0 * w);
        const double s2 = std::sin(2.0 * w);
        const double nr = (double) b0 + (double) b1 * c1 + (double) b2 * c2;
        const double ni = -(double) b1 * s1 - (double) b2 * s2;
        const double dr = 1.0 + (double) a1 * c1 + (double) a2 * c2;
        const double di = -(double) a1 * s1 - (double) a2 * s2;
        const double mag = std::sqrt((nr * nr + ni * ni) / std::max(1.0e-20, dr * dr + di * di));
        return (float) (20.0 * std::log10(std::max(mag, 1.0e-8)));
    }
};

} // namespace xenhusk
