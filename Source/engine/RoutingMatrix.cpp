#include "RoutingMatrix.h"

#include <algorithm>

namespace xenhusk
{

RoutingMatrix::RoutingMatrix()
{
    resetToDefault();
}

bool RoutingMatrix::inRange(int strip, int bus) const noexcept
{
    return strip >= 0 && strip < stripCount && bus >= 0 && bus < busCount;
}

float RoutingMatrix::defaultGainDb(int strip, int bus) noexcept
{
    return strip == bus ? unityDb : silenceDb;
}

void RoutingMatrix::setGainDb(int strip, int bus, float db)
{
    if (! inRange(strip, bus))
        return;

    db = std::clamp(db, minimumDb, maximumDb);
    gainDb[strip][bus] = db;
    linear[strip][bus].store(dbToLinear(db), std::memory_order_relaxed);
}

float RoutingMatrix::getGainDb(int strip, int bus) const
{
    if (! inRange(strip, bus))
        return silenceDb;

    return gainDb[strip][bus];
}

float RoutingMatrix::getLinearGain(int strip, int bus) const
{
    if (! inRange(strip, bus))
        return 0.0f;

    return linear[strip][bus].load(std::memory_order_relaxed);
}

void RoutingMatrix::setStripMuted(int strip, bool muted)
{
    if (strip < 0 || strip >= stripCount)
        return;

    stripMute[strip].store(muted, std::memory_order_relaxed);
}

void RoutingMatrix::setBusMuted(int bus, bool muted)
{
    if (bus < 0 || bus >= busCount)
        return;

    busMute[bus].store(muted, std::memory_order_relaxed);
}

bool RoutingMatrix::isStripMuted(int strip) const
{
    if (strip < 0 || strip >= stripCount)
        return false;

    return stripMute[strip].load(std::memory_order_relaxed);
}

bool RoutingMatrix::isBusMuted(int bus) const
{
    if (bus < 0 || bus >= busCount)
        return false;

    return busMute[bus].load(std::memory_order_relaxed);
}

void RoutingMatrix::resetToDefault()
{
    for (int strip = 0; strip < stripCount; ++strip)
    {
        stripMute[strip].store(false, std::memory_order_relaxed);

        for (int bus = 0; bus < busCount; ++bus)
        {
            if (strip == 0)
                busMute[bus].store(false, std::memory_order_relaxed);

            setGainDb(strip, bus, defaultGainDb(strip, bus));
        }
    }
}

void RoutingMatrix::process(const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output) const
{
    const int numSamples = juce::jmin(input.getNumSamples(), output.getNumSamples());
    output.clear(0, numSamples);

    for (int bus = 0; bus < busCount; ++bus)
    {
        if (bus >= output.getNumChannels() || busMute[bus].load(std::memory_order_relaxed))
            continue;

        auto* out = output.getWritePointer(bus);

        for (int strip = 0; strip < stripCount; ++strip)
        {
            if (strip >= input.getNumChannels() || stripMute[strip].load(std::memory_order_relaxed))
                continue;

            const float gain = linear[strip][bus].load(std::memory_order_relaxed);

            if (gain == 0.0f)
                continue;

            juce::FloatVectorOperations::addWithMultiply(out, input.getReadPointer(strip), gain, numSamples);
        }
    }
}

} // namespace xenhusk
