#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace xenhusk
{

struct Palette
{
    juce::Colour background;
    juce::Colour panel;
    juce::Colour text;
    juce::Colour secondary;
    juce::Colour accent;
    juce::Colour hairline;
    juce::Colour track;
    juce::Colour thumb;
};

class AppleMixerLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AppleMixerLookAndFeel();

    void refreshPalette();
    const Palette& getPalette() const noexcept { return palette; }

    void drawButtonBackground(juce::Graphics& g,
                              juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g,
                        juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    void drawRotarySlider(juce::Graphics& g,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPosProportional,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPos,
                          float minSliderPos,
                          float maxSliderPos,
                          juce::Slider::SliderStyle style,
                          juce::Slider& slider) override;

    juce::Font getLabelFont(juce::Label& label) override;
    juce::Font getComboBoxFont(juce::ComboBox& box) override;
    juce::Font getPopupMenuFont() override;

private:
    Palette palette;
};

} // namespace xenhusk
