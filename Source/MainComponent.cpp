#include "MainComponent.h"

#include "engine/SettingsDocument.h"
#include "ui/Details.h"

namespace xenhusk
{

MainComponent::MainComponent()
{
    setOpaque(true);
    setLookAndFeel(&lookAndFeel);
    juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);
    juce::Desktop::getInstance().addDarkModeSettingListener(this);

    titleLabel.setText("Xenhusk Mixer", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Segoe UI", 20.0f, juce::Font::bold).withKerningFactor(-0.02f)));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);

    statusLabel.setFont(juce::Font(juce::FontOptions("Segoe UI", 13.0f, juce::Font::plain)));
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(statusLabel);

    hintLabel.setText("Right-click EQ, dynamics, or an effect for its parameters. Save and Load keep the mix. Closing the window leaves it in the tray. A1 is the hardware output.",
                      juce::dontSendNotification);
    hintLabel.setFont(juce::Font(juce::FontOptions("Segoe UI", 13.0f, juce::Font::plain)));
    hintLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(hintLabel);

    resetButton.setButtonText("Reset");
    resetButton.setComponentID("reset");
    resetButton.addListener(this);
    addAndMakeVisible(resetButton);

    menuButton.setButtonText("Menu");
    menuButton.setComponentID("reset");
    menuButton.addListener(this);
    addAndMakeVisible(menuButton);

    saveButton.setButtonText("Save");
    loadButton.setButtonText("Load");
    saveButton.setComponentID("reset");
    loadButton.setComponentID("reset");
    saveButton.addListener(this);
    loadButton.addListener(this);
    addAndMakeVisible(saveButton);
    addAndMakeVisible(loadButton);

    for (int strip = 0; strip < Mixer::stripCount; ++strip)
    {
        strips[strip] = std::make_unique<StripColumn>(engine.getMixer(), strip);
        stripHolder.addAndMakeVisible(*strips[strip]);
    }

    master = std::make_unique<MasterPanel>(engine.getMixer(), engine);
    master->onMenu = [this] { showDeviceMenu(); };
    master->onChooseFile = [this] { choosePlaybackFile(); };
    addAndMakeVisible(*master);

    stripViewport.setViewedComponent(&stripHolder, false);
    stripViewport.setScrollBarsShown(true, true);
    addAndMakeVisible(stripViewport);

    deviceManager.addAudioCallback(&engine);
    deviceError = deviceManager.initialiseWithDefaultDevices(Mixer::stripCount * 2, 2);
    updateStatus();
    startTimerHz(15);
    setSize(1880, 1000);
}

MainComponent::~MainComponent()
{
    stopTimer();
    juce::Desktop::getInstance().removeDarkModeSettingListener(this);
    setLookAndFeel(nullptr);
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    stripViewport.setViewedComponent(nullptr, false);
    deviceManager.removeAudioCallback(&engine);
    deviceManager.closeAudioDevice();
}

void MainComponent::paint(juce::Graphics& g)
{
    const auto& palette = lookAndFeel.getPalette();
    g.fillAll(palette.background);
    g.setColour(palette.panel);
    g.fillRect(topBar);
    g.setColour(palette.hairline);
    g.fillRect(topBar.getX(), topBar.getBottom(), topBar.getWidth(), 1);
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();
    topBar = bounds.removeFromTop(64);
    auto header = topBar.reduced(24, 0);
    titleLabel.setBounds(header.removeFromLeft(180));
    resetButton.setBounds(header.removeFromRight(84).withSizeKeepingCentre(80, 32));
    menuButton.setBounds(header.removeFromRight(84).withSizeKeepingCentre(80, 32));
    loadButton.setBounds(header.removeFromRight(84).withSizeKeepingCentre(80, 32));
    saveButton.setBounds(header.removeFromRight(84).withSizeKeepingCentre(80, 32));
    statusLabel.setBounds(header.reduced(16, 0));
    hintLabel.setBounds(bounds.removeFromBottom(36).reduced(24, 6));

    master->setBounds(bounds.removeFromRight(640).reduced(8));
    stripViewport.setBounds(bounds.reduced(8));

    const int width = StripColumn::preferredWidth * Mixer::stripCount;
    const int height = juce::jmax(640, stripViewport.getHeight());
    stripHolder.setSize(width, height);

    for (int strip = 0; strip < Mixer::stripCount; ++strip)
        strips[strip]->setBounds(strip * StripColumn::preferredWidth, 0, StripColumn::preferredWidth, height);
}

void MainComponent::timerCallback()
{
    updateStatus();
    master->updateTransport();
    stripHolder.repaint();
    master->repaint();
}

void MainComponent::buttonClicked(juce::Button* button)
{
    if (button == &resetButton)
        resetMixer();
    else if (button == &menuButton)
        showDeviceMenu();
    else if (button == &saveButton)
        saveSettings();
    else if (button == &loadButton)
        loadSettings();
}

void MainComponent::darkModeSettingChanged()
{
    lookAndFeel.refreshPalette();
    sendLookAndFeelChange();
    repaint();
}

void MainComponent::updateStatus()
{
    const auto name = engine.getDeviceName();
    juce::String status = name.isEmpty()
                            ? (deviceError.isNotEmpty() ? deviceError : juce::String("No audio device"))
                            : name + "  ·  " + juce::String(juce::roundToInt(engine.getSampleRate()))
                                  + " Hz  ·  " + juce::String(engine.getBufferSize()) + " samples";

    if (engine.isProAudioActive())
        status += "  ·  Pro Audio";

    statusLabel.setText(status, juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId,
                          name.isEmpty() ? lookAndFeel.getPalette().text : lookAndFeel.getPalette().secondary);
    titleLabel.setColour(juce::Label::textColourId, lookAndFeel.getPalette().text);
    hintLabel.setColour(juce::Label::textColourId, lookAndFeel.getPalette().secondary);
}

void MainComponent::resetMixer()
{
    engine.getMixer().resetToDefault();
    for (auto& strip : strips)
        strip->refreshFromMixer();
    master->refreshFromMixer();
}

void MainComponent::choosePlaybackFile()
{
    chooser = std::make_unique<juce::FileChooser>("Choose an audio file", juce::File{}, "*.wav;*.aiff;*.flac;*.mp3");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                         [this](const juce::FileChooser& dialog)
                         {
                             const auto file = dialog.getResult();
                             if (file.existsAsFile())
                                 engine.loadPlaybackFile(file);
                         });
}

void MainComponent::refreshConsole()
{
    for (auto& strip : strips)
        strip->refreshFromMixer();
    master->refreshFromMixer();
}

void MainComponent::restartAudioEngine()
{
    deviceManager.restartLastAudioDevice();
    deviceError.clear();
    if (deviceManager.getCurrentAudioDevice() == nullptr)
        deviceError = "No audio device";
    updateStatus();
}

void MainComponent::showDeviceMenu()
{
    showSystemPage(deviceManager, engine.getMixer(), [this] { saveSettings(); }, [this] { loadSettings(); }, [this] { refreshConsole(); });
}

void MainComponent::saveSettings()
{
    auto folder = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Xenhusk Mixer");
    folder.createDirectory();
    settingsChooser = std::make_unique<juce::FileChooser>("Save mix", folder.getChildFile("mix.xml"), "*.xml");
    settingsChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                                 [this](const juce::FileChooser& dialog)
                                 {
                                     const auto file = dialog.getResult();
                                     if (file != juce::File())
                                         saveSettingsFile(engine.getMixer(), file);
                                 });
}

void MainComponent::loadSettings()
{
    settingsChooser = std::make_unique<juce::FileChooser>("Load mix", juce::File{}, "*.xml");
    settingsChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                 [this](const juce::FileChooser& dialog)
                                 {
                                     const auto file = dialog.getResult();
                                     if (file.existsAsFile() && loadSettingsFile(engine.getMixer(), file))
                                         refreshConsole();
                                 });
}

} // namespace xenhusk
