#pragma once

#include "AppleMixerLookAndFeel.h"
#include "engine/AudioEngine.h"
#include "ui/Console.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace xenhusk
{

class MainComponent : public juce::Component,
                      private juce::Timer,
                      private juce::Button::Listener,
                      private juce::DarkModeSettingListener
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void restartAudioEngine();

private:
    void timerCallback() override;
    void buttonClicked(juce::Button* button) override;
    void darkModeSettingChanged() override;
    void updateStatus();
    void resetMixer();
    void choosePlaybackFile();
    void showDeviceMenu();
    void saveSettings();
    void loadSettings();
    void refreshConsole();

    AppleMixerLookAndFeel lookAndFeel;
    AudioEngine engine;
    juce::AudioDeviceManager deviceManager;
    juce::String deviceError;
    std::unique_ptr<juce::FileChooser> chooser;
    std::unique_ptr<juce::FileChooser> settingsChooser;

    juce::Label titleLabel, statusLabel, hintLabel;
    juce::TextButton resetButton;
    juce::TextButton menuButton;
    juce::TextButton saveButton;
    juce::TextButton loadButton;
    juce::Viewport stripViewport;
    juce::Component stripHolder;
    std::unique_ptr<StripColumn> strips[Mixer::stripCount];
    std::unique_ptr<MasterPanel> master;
    juce::Rectangle<int> topBar;
};

} // namespace xenhusk
