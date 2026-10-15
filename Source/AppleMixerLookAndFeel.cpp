#include "AppleMixerLookAndFeel.h"

#include <cmath>

namespace xenhusk
{
namespace
{

Palette lightPalette()
{
    return {
        juce::Colour(0xffF5F5F7),
        juce::Colour(0xffFFFFFF),
        juce::Colour(0xff1D1D1F),
        juce::Colour(0xff6E6E73),
        juce::Colour(0xff0071E3),
        juce::Colour(0xffD2D2D7),
        juce::Colour(0xffE8E8ED),
        juce::Colour(0xffFFFFFF)
    };
}

Palette darkPalette()
{
    return {
        juce::Colour(0xff1D1D1F),
        juce::Colour(0xff2C2C2E),
        juce::Colour(0xffF5F5F7),
        juce::Colour(0xffA1A1A6),
        juce::Colour(0xff0A84FF),
        juce::Colour(0xff3A3A3C),
        juce::Colour(0xff3A3A3C),
        juce::Colour(0xffF5F5F7)
    };
}

} // namespace

AppleMixerLookAndFeel::AppleMixerLookAndFeel()
{
    refreshPalette();
}

void AppleMixerLookAndFeel::refreshPalette()
{
    palette = juce::Desktop::getInstance().isDarkModeActive() ? darkPalette() : lightPalette();

    setColour(juce::Slider::textBoxTextColourId, palette.text);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxHighlightColourId, palette.accent.withAlpha(0.24f));
    setColour(juce::Label::textColourId, palette.text);
    setColour(juce::ComboBox::textColourId, palette.text);
    setColour(juce::ComboBox::backgroundColourId, palette.panel);
    setColour(juce::ComboBox::outlineColourId, palette.hairline);
    setColour(juce::ComboBox::arrowColourId, palette.secondary);
    setColour(juce::PopupMenu::backgroundColourId, palette.panel);
    setColour(juce::PopupMenu::textColourId, palette.text);
    setColour(juce::TextEditor::textColourId, palette.text);
    setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::TextEditor::outlineColourId, palette.hairline);
    setColour(juce::TextEditor::highlightColourId, palette.accent.withAlpha(0.24f));
    setColour(juce::ScrollBar::thumbColourId, palette.secondary);
    setColour(juce::TextButton::textColourOffId, palette.text);
    setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    setColour(juce::ResizableWindow::backgroundColourId, palette.background);
    setColour(juce::ListBox::backgroundColourId, palette.background);
    setColour(juce::ListBox::textColourId, palette.text);
    setColour(juce::ListBox::outlineColourId, palette.hairline);
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
}

void AppleMixerLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                                 juce::Button& button,
                                                 const juce::Colour&,
                                                 bool shouldDrawButtonAsHighlighted,
                                                 bool shouldDrawButtonAsDown)
{
    const bool toggle = button.getComponentID() == "toggle";
    const bool on = toggle && button.getToggleState();

    auto fill = on ? palette.accent : palette.panel;

    if (shouldDrawButtonAsDown)
        fill = fill.darker(on ? 0.18f : 0.08f);
    else if (shouldDrawButtonAsHighlighted)
        fill = fill.darker(0.04f);

    const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(fill);
    g.fillRoundedRectangle(bounds, 8.0f);

    if (! on)
    {
        g.setColour(palette.hairline);
        g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
    }
}

void AppleMixerLookAndFeel::drawButtonText(juce::Graphics& g,
                                           juce::TextButton& button,
                                           bool,
                                           bool)
{
    const bool toggleOn = button.getComponentID() == "toggle" && button.getToggleState();
    g.setFont(juce::Font(juce::FontOptions("Segoe UI", 12.0f, juce::Font::plain)));
    g.setColour(toggleOn ? juce::Colours::white : palette.text);
    g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(4, 0), juce::Justification::centred, 1);
}

void AppleMixerLookAndFeel::drawLinearSlider(juce::Graphics& g,
                                             int x,
                                             int y,
                                             int width,
                                             int height,
                                             float sliderPos,
                                             float,
                                             float,
                                             juce::Slider::SliderStyle,
                                             juce::Slider& slider)
{
    if (height > width)
    {
        const float trackWidth = 4.0f;
        const float centreX = (float) x + (float) width * 0.5f;
        const auto track = juce::Rectangle<float>(centreX - trackWidth * 0.5f, (float) y, trackWidth, (float) height);
        g.setColour(palette.track);
        g.fillRoundedRectangle(track, 2.0f);

        const float thumbY = juce::jlimit((float) y, (float) y + (float) height, sliderPos);
        g.setColour(palette.accent);
        g.fillRoundedRectangle(track.withTop(thumbY), 2.0f);

        auto thumbColour = slider.isMouseButtonDown() ? palette.thumb.darker(0.12f) : palette.thumb;
        juce::Rectangle<float> thumb(12.0f, 12.0f);
        thumb.setCentre(centreX, thumbY);
        g.setColour(thumbColour);
        g.fillEllipse(thumb);
        g.setColour(palette.hairline);
        g.drawEllipse(thumb.reduced(0.5f), 1.0f);
        return;
    }

    auto bounds = juce::Rectangle<int>(x, y, width, height);
    constexpr int valueHeight = 14;
    constexpr int groupHeight = valueHeight + 16;
    auto group = bounds.withSizeKeepingCentre(bounds.getWidth(), juce::jmin(groupHeight, bounds.getHeight()));
    auto valueArea = group.removeFromTop(valueHeight);
    const bool atUnity = std::abs(slider.getValue()) < 0.05;
    const bool silent = slider.getValue() <= (double) -99.95;
    g.setFont(juce::Font(juce::FontOptions("Segoe UI", 11.0f, juce::Font::plain)));
    g.setColour(atUnity ? palette.accent : (silent ? palette.secondary : palette.text));
    g.drawFittedText(slider.getTextFromValue(slider.getValue()), valueArea, juce::Justification::centred, 1);

    const float trackHeight = 4.0f;
    const float centreY = (float) group.getCentreY();
    const auto track = juce::Rectangle<float>((float) group.getX(), centreY - trackHeight * 0.5f, (float) group.getWidth(), trackHeight);

    g.setColour(palette.track);
    g.fillRoundedRectangle(track, 2.0f);

    const float thumbX = juce::jlimit((float) x, (float) x + (float) width, sliderPos);
    if (thumbX > track.getX())
    {
        g.setColour(palette.accent);
        g.fillRoundedRectangle(track.withRight(thumbX), 2.0f);
    }

    const float range = (float) (slider.getMaximum() - slider.getMinimum());
    if (range > 0.0f)
    {
        const float unityX = (float) x + (float) width * ((0.0f - (float) slider.getMinimum()) / range);
        g.setColour(palette.secondary);
        g.fillRect(unityX, track.getY() - 3.0f, 1.0f, track.getHeight() + 6.0f);
    }

    auto thumbColour = palette.thumb;
    if (slider.isMouseButtonDown())
        thumbColour = thumbColour.darker(0.12f);

    juce::Rectangle<float> thumb(12.0f, 12.0f);
    thumb.setCentre(thumbX, centreY);
    g.setColour(thumbColour);
    g.fillEllipse(thumb);
    g.setColour(palette.hairline);
    g.drawEllipse(thumb.reduced(0.5f), 1.0f);

    if (slider.hasKeyboardFocus(false))
    {
        g.setColour(palette.accent);
        g.drawRoundedRectangle(slider.getLocalBounds().toFloat().reduced(1.0f), 6.0f, 1.5f);
    }
}

void AppleMixerLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                              int x,
                                              int y,
                                              int width,
                                              int height,
                                              float sliderPosProportional,
                                              float rotaryStartAngle,
                                              float rotaryEndAngle,
                                              juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) juce::jmax(0, height - 14)).reduced(2.0f);
    const float side = juce::jmin(bounds.getWidth(), bounds.getHeight());
    bounds = bounds.withSizeKeepingCentre(side, side);
    const float radius = side * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    g.setColour(palette.track);
    g.drawEllipse(bounds.reduced(2.0f), 2.0f);

    juce::Path arc;
    arc.addCentredArc(centre.x, centre.y, radius - 2.0f, radius - 2.0f, 0.0f, rotaryStartAngle, angle, true);
    g.setColour(palette.accent);
    g.strokePath(arc, juce::PathStrokeType(2.0f));

    juce::Path pointer;
    pointer.addRectangle(-1.0f, -radius + 6.0f, 2.0f, radius * 0.45f);
    g.setColour(slider.isMouseButtonDown() ? palette.text.darker(0.12f) : palette.text);
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
}

juce::Font AppleMixerLookAndFeel::getLabelFont(juce::Label& label)
{
    if (dynamic_cast<juce::Slider*>(label.getParentComponent()) != nullptr)
        return juce::Font(juce::FontOptions("Segoe UI", 11.0f, juce::Font::plain));

    return juce::Font(juce::FontOptions("Segoe UI", 13.0f, juce::Font::plain));
}

juce::Font AppleMixerLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return juce::Font(juce::FontOptions("Segoe UI", 13.0f, juce::Font::plain));
}

juce::Font AppleMixerLookAndFeel::getPopupMenuFont()
{
    return juce::Font(juce::FontOptions("Segoe UI", 13.0f, juce::Font::plain));
}

} // namespace xenhusk
