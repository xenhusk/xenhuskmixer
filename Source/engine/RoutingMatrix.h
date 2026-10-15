#pragma once

#include "Gain.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <atomic>

namespace xenhusk
{

class RoutingMatrix
{
public:
    static constexpr int stripCount = 8;
    static constexpr int busCount = 8;
    static constexpr float silenceDb = -100.0f;
    static constexpr float unityDb = 0.0f;
    static constexpr float minimumDb = -100.0f;
    static constexpr float maximumDb = 12.0f;

    RoutingMatrix();

    void setGainDb(int strip, int bus, float db);
    float getGainDb(int strip, int bus) const;
    float getLinearGain(int strip, int bus) const;

    void setStripMuted(int strip, bool muted);
    void setBusMuted(int bus, bool muted);
    bool isStripMuted(int strip) const;
    bool isBusMuted(int bus) const;

    void resetToDefault();
    static float defaultGainDb(int strip, int bus) noexcept;

    void process(const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output) const;

private:
    bool inRange(int strip, int bus) const noexcept;

    float gainDb[stripCount][busCount] {};
    std::atomic<float> linear[stripCount][busCount];
    std::atomic<bool> stripMute[stripCount];
    std::atomic<bool> busMute[busCount];
};

} // namespace xenhusk
