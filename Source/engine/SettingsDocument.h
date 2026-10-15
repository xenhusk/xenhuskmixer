#pragma once

#include "Mixer.h"

#include <memory>

namespace xenhusk
{

std::unique_ptr<juce::XmlElement> settingsToXml(const Mixer& mixer);
bool applySettingsXml(Mixer& mixer, const juce::XmlElement& root);
bool loadSettingsFile(Mixer& mixer, const juce::File& file);
bool saveSettingsFile(const Mixer& mixer, const juce::File& file);

} // namespace xenhusk
