#pragma once

#include "engine/AudioEngine.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace xenhusk
{

class StripColumn : public juce::Component,
                    private juce::Slider::Listener,
                    private juce::Button::Listener
{
public:
    StripColumn(Mixer& mixer, int index);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void refreshFromMixer();

    static constexpr int preferredWidth = 150;
    static constexpr int preferredHeight = 900;

private:
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    void configureKnob(juce::Slider& slider, const juce::String& title, float minimum, float maximum, float resetValue);
    void configureToggle(juce::TextButton& button, const juce::String& text);

    Mixer& mixer;
    int index = 0;
    bool updating = false;
    juce::Rectangle<int> meterArea;

    juce::TextEditor name;
    juce::Label intelliLabel, dynamicsLabel, toneLabel, fxLabel, routeLabel;
    juce::TextButton eqButton, postButton, soloButton, monoButton, muteButton;
    juce::TextButton assign[Mixer::busCount];
    juce::Slider colorKnob, voiceKnob, echoKnob, compKnob, gateKnob, denoiseKnob;
    juce::Slider bassKnob, trebleKnob, reverbKnob, delayKnob, fader;
};

class MasterPanel : public juce::Component,
                    private juce::Slider::Listener,
                    private juce::Button::Listener
{
public:
    MasterPanel(Mixer& mixer, AudioEngine& engine);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void refreshFromMixer();
    void updateTransport();

    std::function<void()> onChooseFile;
    std::function<void()> onMenu;

private:
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    void configureKnob(juce::Slider& slider, const juce::String& title);

    Mixer& mixer;
    AudioEngine& engine;
    bool updating = false;
    juce::Rectangle<int> meterArea[Mixer::busCount];

    juce::TextButton playButton, stopButton, recordButton, fileButton;
    juce::Label timeLabel, fileLabel, returnLabel, playIntoLabel, recordLabel;
    juce::ComboBox playbackStrip, recordBus;
    juce::Slider reverbReturn, delayReturn;
    juce::Label busName[Mixer::busCount];
    juce::TextButton busEq[Mixer::busCount];
    juce::TextButton busMono[Mixer::busCount];
    juce::TextButton busMute[Mixer::busCount];
    juce::Slider busBass[Mixer::busCount];
    juce::Slider busTreble[Mixer::busCount];
    juce::Slider busFader[Mixer::busCount];
};

} // namespace xenhusk
