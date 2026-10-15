#pragma once

#include "engine/Mixer.h"

#include <functional>

namespace juce
{
class AudioDeviceManager;
}

namespace xenhusk
{

enum class DetailPage
{
    eq,
    busEq,
    comp,
    gate,
    denoise,
    pan,
    reverb,
    delay
};

void showDetail(DetailPage page, Mixer& mixer, int index, std::function<void()> onChanged);
void showSystemPage(juce::AudioDeviceManager& devices,
                    Mixer& mixer,
                    std::function<void()> onSave,
                    std::function<void()> onLoad,
                    std::function<void()> onChanged);

} // namespace xenhusk
