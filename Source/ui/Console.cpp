#include "Console.h"

#include "AppleMixerLookAndFeel.h"
#include "ui/Details.h"

namespace xenhusk
{
namespace
{

const juce::StringArray busNames { "A1", "A2", "A3", "A4", "A5", "B1", "B2", "B3" };
const juce::StringArray stripNames { "Mic", "Music", "In 3", "In 4", "In 5", "Virtual 1", "Virtual 2", "Virtual 3" };

juce::Font uiFont(float size, bool bold = false)
{
    return juce::Font(juce::FontOptions("Segoe UI", size, bold ? juce::Font::bold : juce::Font::plain));
}

void styleLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setFont(uiFont(11.0f, true));
    label.setJustificationType(juce::Justification::centred);
    label.setInterceptsMouseClicks(false, false);
}

} // namespace

StripColumn::StripColumn(Mixer& source, int stripIndex)
    : mixer(source), index(stripIndex)
{
    name.setText(stripNames[index]);
    name.setFont(uiFont(13.0f, true));
    name.setJustification(juce::Justification::centred);
    name.setIndents(4, 2);
    addAndMakeVisible(name);

    styleLabel(intelliLabel, "IntelliPan");
    styleLabel(dynamicsLabel, "Dynamics");
    styleLabel(toneLabel, "Tone");
    styleLabel(fxLabel, "Special FX");
    styleLabel(routeLabel, "Route");
    for (auto* label : { &intelliLabel, &dynamicsLabel, &toneLabel, &fxLabel, &routeLabel })
        addAndMakeVisible(label);

    configureToggle(eqButton, "EQ");
    configureToggle(postButton, "Post");
    configureToggle(soloButton, "Solo");
    configureToggle(monoButton, "Mono");
    configureToggle(muteButton, "Mute");
    postButton.setToggleState(true, juce::dontSendNotification);

    for (int bus = 0; bus < Mixer::busCount; ++bus)
    {
        configureToggle(assign[bus], busNames[bus]);
        assign[bus].setToggleState(bus == 0, juce::dontSendNotification);
    }

    configureKnob(colorKnob, "Color", -1.0, 1.0, 0.0);
    configureKnob(voiceKnob, "Voice", 0.0, 1.0, 0.0);
    configureKnob(echoKnob, "Echo", 0.0, 1.0, 0.0);
    configureKnob(compKnob, "Comp", 0.0, 1.0, 0.0);
    configureKnob(gateKnob, "Gate", 0.0, 1.0, 0.0);
    configureKnob(denoiseKnob, "Denoise", 0.0, 1.0, 0.0);
    configureKnob(bassKnob, "Bass", -1.0, 1.0, 0.0);
    configureKnob(trebleKnob, "Treble", -1.0, 1.0, 0.0);
    configureKnob(reverbKnob, "Reverb", 0.0, 1.0, 0.0);
    configureKnob(delayKnob, "Delay", 0.0, 1.0, 0.0);

    fader.setSliderStyle(juce::Slider::LinearVertical);
    fader.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 16);
    fader.setRange(Mixer::minimumDb, Mixer::maximumDb, 0.1);
    fader.setNumDecimalPlacesToDisplay(1);
    fader.setTextValueSuffix(" dB");
    fader.setValue(0.0, juce::dontSendNotification);
    fader.setDoubleClickReturnValue(true, 0.0);
    fader.setVelocityBasedMode(false);
    fader.setSliderSnapsToMousePosition(true);
    fader.setTitle("Fader");
    fader.addListener(this);
    addAndMakeVisible(fader);
    name.onTextChange = [this] { mixer.setStripName(index, name.getText()); };
    mixer.setStripName(index, name.getText());
    addMouseListener(this, true);
    eqButton.setTooltip("Right-click for parameters");
    for (auto* slider : { &bassKnob, &trebleKnob, &compKnob, &gateKnob, &denoiseKnob, &colorKnob, &voiceKnob, &echoKnob, &reverbKnob, &delayKnob })
        slider->setTooltip("Right-click for parameters");
}

void StripColumn::configureKnob(juce::Slider& slider, const juce::String& title, float minimum, float maximum, float resetValue)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange(minimum, maximum, 0.01);
    slider.setValue(resetValue, juce::dontSendNotification);
    slider.setDoubleClickReturnValue(true, resetValue);
    slider.setVelocityBasedMode(false);
    slider.setTitle(title);
    slider.setTooltip(title);
    slider.addListener(this);
    addAndMakeVisible(slider);
}

void StripColumn::configureToggle(juce::TextButton& button, const juce::String& text)
{
    button.setButtonText(text);
    button.setComponentID("toggle");
    button.setClickingTogglesState(true);
    button.setTitle(text);
    button.addListener(this);
    addAndMakeVisible(button);
}

void StripColumn::refreshFromMixer()
{
    updating = true;
    name.setText(mixer.getStripName(index), juce::dontSendNotification);
    eqButton.setToggleState(mixer.isEqEnabled(index), juce::dontSendNotification);
    postButton.setToggleState(mixer.isPostFx(index), juce::dontSendNotification);
    soloButton.setToggleState(mixer.isStripSolo(index), juce::dontSendNotification);
    monoButton.setToggleState(mixer.isStripMono(index), juce::dontSendNotification);
    muteButton.setToggleState(mixer.isStripMuted(index), juce::dontSendNotification);
    colorKnob.setValue(mixer.getColor(index), juce::dontSendNotification);
    voiceKnob.setValue(mixer.getVoice(index), juce::dontSendNotification);
    echoKnob.setValue(mixer.getEcho(index), juce::dontSendNotification);
    compKnob.setValue(mixer.getComp(index), juce::dontSendNotification);
    gateKnob.setValue(mixer.getGate(index), juce::dontSendNotification);
    denoiseKnob.setValue(mixer.getDenoise(index), juce::dontSendNotification);
    bassKnob.setValue(mixer.getBass(index), juce::dontSendNotification);
    trebleKnob.setValue(mixer.getTreble(index), juce::dontSendNotification);
    reverbKnob.setValue(mixer.getReverbSend(index), juce::dontSendNotification);
    delayKnob.setValue(mixer.getDelaySend(index), juce::dontSendNotification);
    fader.setValue(mixer.getFaderDb(index), juce::dontSendNotification);

    for (int bus = 0; bus < Mixer::busCount; ++bus)
        assign[bus].setToggleState(mixer.isAssigned(index, bus), juce::dontSendNotification);

    updating = false;
}

void StripColumn::paint(juce::Graphics& g)
{
    const auto* look = dynamic_cast<AppleMixerLookAndFeel*>(&getLookAndFeel());
    const auto panelColour = look != nullptr ? look->getPalette().panel : juce::Colour(0xff2C2C2E);
    const auto line = look != nullptr ? look->getPalette().hairline : juce::Colour(0xff3A3A3C);
    const auto accent = look != nullptr ? look->getPalette().accent : juce::Colour(0xff0A84FF);
    const auto text = look != nullptr ? look->getPalette().text : juce::Colour(0xffF5F5F7);
    const auto track = look != nullptr ? look->getPalette().track : juce::Colour(0xff3A3A3C);

    auto panel = getLocalBounds().toFloat().reduced(4.0f);
    g.setColour(panelColour);
    g.fillRoundedRectangle(panel, 12.0f);
    g.setColour(line);
    g.drawRoundedRectangle(panel, 12.0f, 1.0f);

    g.setColour(track);
    g.fillRoundedRectangle(meterArea.toFloat(), 2.0f);
    const float level = juce::jlimit(0.0f, 1.0f, mixer.getStripMeter(index));
    auto fill = meterArea;
    fill = fill.removeFromBottom(juce::roundToInt((float) meterArea.getHeight() * level));
    g.setColour(accent);
    g.fillRoundedRectangle(fill.toFloat(), 2.0f);

    g.setColour(text);
    g.setFont(uiFont(10.0f));
    auto caption = [&g](juce::Slider& slider, const juce::String& text)
    {
        auto bounds = slider.getBounds();
        g.drawFittedText(text, bounds.getX(), bounds.getBottom() - 14, bounds.getWidth(), 14, juce::Justification::centred, 1);
    };
    caption(colorKnob, "Color");
    caption(voiceKnob, "Voice");
    caption(echoKnob, "Echo");
    caption(compKnob, "Comp");
    caption(gateKnob, "Gate");
    caption(denoiseKnob, "Denoise");
    caption(bassKnob, "Bass");
    caption(trebleKnob, "Treble");
    caption(reverbKnob, "Reverb");
    caption(delayKnob, "Delay");
}

void StripColumn::lookAndFeelChanged()
{
    auto* look = dynamic_cast<AppleMixerLookAndFeel*>(&getLookAndFeel());
    if (look == nullptr)
        return;

    const auto& palette = look->getPalette();
    name.setColour(juce::TextEditor::textColourId, palette.text);
    name.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    name.setColour(juce::TextEditor::outlineColourId, palette.hairline);
    name.setColour(juce::TextEditor::focusedOutlineColourId, palette.accent);
    name.setColour(juce::TextEditor::highlightColourId, palette.accent.withAlpha(0.24f));
    name.applyFontToAllText(uiFont(13.0f, true));
}

void StripColumn::resized()
{
    auto area = getLocalBounds().reduced(10, 8);
    name.setBounds(area.removeFromTop(26));
    area.removeFromTop(6);
    eqButton.setBounds(area.removeFromTop(26).withSizeKeepingCentre(72, 26));

    auto placePair = [&area](juce::Label& label, juce::Slider& left, juce::Slider& right)
    {
        area.removeFromTop(4);
        label.setBounds(area.removeFromTop(14));
        auto row = area.removeFromTop(48);
        left.setBounds(row.removeFromLeft(row.getWidth() / 2).reduced(4, 0));
        right.setBounds(row.reduced(4, 0));
    };

    placePair(intelliLabel, colorKnob, voiceKnob);
    echoKnob.setBounds(area.removeFromTop(48).withSizeKeepingCentre(72, 44));
    placePair(dynamicsLabel, compKnob, gateKnob);
    denoiseKnob.setBounds(area.removeFromTop(48).withSizeKeepingCentre(72, 44));
    placePair(toneLabel, bassKnob, trebleKnob);
    placePair(fxLabel, reverbKnob, delayKnob);
    postButton.setBounds(area.removeFromTop(26).withSizeKeepingCentre(72, 26));

    area.removeFromTop(4);
    routeLabel.setBounds(area.removeFromTop(14));
    for (int rowIndex = 0; rowIndex < 2; ++rowIndex)
    {
        auto row = area.removeFromTop(24);
        const int count = rowIndex == 0 ? 5 : 3;
        const int width = row.getWidth() / count;
        for (int column = 0; column < count; ++column)
            assign[rowIndex * 5 + column].setBounds(row.removeFromLeft(width).reduced(1, 0));
    }

    area.removeFromTop(6);
    auto faderRow = area.removeFromTop(juce::jmax(120, area.getHeight() - 34));
    meterArea = faderRow.removeFromRight(8).reduced(0, 16);
    fader.setBounds(faderRow.reduced(28, 0));
    auto buttons = area.removeFromTop(28);
    const int buttonWidth = buttons.getWidth() / 3;
    soloButton.setBounds(buttons.removeFromLeft(buttonWidth).reduced(1, 0));
    monoButton.setBounds(buttons.removeFromLeft(buttonWidth).reduced(1, 0));
    muteButton.setBounds(buttons.reduced(1, 0));
}

void StripColumn::mouseDown(const juce::MouseEvent& event)
{
    if (! event.mods.isPopupMenu())
        return;

    const auto refresh = [this] { refreshFromMixer(); };
    auto* control = event.eventComponent;
    if (control == &eqButton || control == &bassKnob || control == &trebleKnob)
        showDetail(DetailPage::eq, mixer, index, refresh);
    else if (control == &compKnob)
        showDetail(DetailPage::comp, mixer, index, refresh);
    else if (control == &gateKnob)
        showDetail(DetailPage::gate, mixer, index, refresh);
    else if (control == &denoiseKnob)
        showDetail(DetailPage::denoise, mixer, index, refresh);
    else if (control == &colorKnob || control == &voiceKnob || control == &echoKnob)
        showDetail(DetailPage::pan, mixer, index, refresh);
    else if (control == &reverbKnob)
        showDetail(DetailPage::reverb, mixer, index, refresh);
    else if (control == &delayKnob)
        showDetail(DetailPage::delay, mixer, index, refresh);
}

void StripColumn::sliderValueChanged(juce::Slider* slider)
{
    if (updating || slider == nullptr)
        return;

    const float value = (float) slider->getValue();
    if (slider == &fader) mixer.setFaderDb(index, value);
    else if (slider == &colorKnob) mixer.setColor(index, value);
    else if (slider == &voiceKnob) mixer.setVoice(index, value);
    else if (slider == &echoKnob) mixer.setEcho(index, value);
    else if (slider == &compKnob) mixer.setComp(index, value);
    else if (slider == &gateKnob) mixer.setGate(index, value);
    else if (slider == &denoiseKnob) mixer.setDenoise(index, value);
    else if (slider == &bassKnob) mixer.setBass(index, value);
    else if (slider == &trebleKnob) mixer.setTreble(index, value);
    else if (slider == &reverbKnob) mixer.setReverbSend(index, value);
    else if (slider == &delayKnob) mixer.setDelaySend(index, value);
}

void StripColumn::buttonClicked(juce::Button* button)
{
    if (updating || button == nullptr)
        return;

    const bool on = button->getToggleState();
    if (button == &eqButton) mixer.setEqEnabled(index, on);
    else if (button == &postButton) mixer.setPostFx(index, on);
    else if (button == &soloButton) mixer.setStripSolo(index, on);
    else if (button == &monoButton) mixer.setStripMono(index, on);
    else if (button == &muteButton) mixer.setStripMuted(index, on);
    else
    {
        for (int bus = 0; bus < Mixer::busCount; ++bus)
            if (button == &assign[bus])
                mixer.setAssigned(index, bus, on);
    }
}

MasterPanel::MasterPanel(Mixer& source, AudioEngine& audioEngine)
    : mixer(source), engine(audioEngine)
{
    styleLabel(returnLabel, "Special FX");
    addAndMakeVisible(returnLabel);
    configureKnob(reverbReturn, "Reverb");
    configureKnob(delayReturn, "Delay");

    for (auto* button : { &playButton, &stopButton, &recordButton, &fileButton })
    {
        button->setComponentID(button == &stopButton || button == &fileButton ? "reset" : "toggle");
        button->setClickingTogglesState(button != &stopButton && button != &fileButton);
        button->addListener(this);
        addAndMakeVisible(button);
    }
    playButton.setButtonText("Play");
    stopButton.setButtonText("Stop");
    recordButton.setButtonText("Record");
    fileButton.setButtonText("File");

    styleLabel(playIntoLabel, "Play into");
    styleLabel(recordLabel, "Record bus");
    addAndMakeVisible(playIntoLabel);
    addAndMakeVisible(recordLabel);

    timeLabel.setText("0.0 s", juce::dontSendNotification);
    timeLabel.setFont(uiFont(13.0f));
    timeLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(timeLabel);
    fileLabel.setText("No file", juce::dontSendNotification);
    fileLabel.setFont(uiFont(12.0f));
    fileLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(fileLabel);

    for (int strip = 0; strip < Mixer::stripCount; ++strip)
        playbackStrip.addItem(stripNames[strip], strip + 1);
    playbackStrip.setSelectedId(6, juce::dontSendNotification);
    playbackStrip.setTooltip("Play into");
    addAndMakeVisible(playbackStrip);

    for (int bus = 0; bus < Mixer::busCount; ++bus)
        recordBus.addItem(busNames[bus], bus + 1);
    recordBus.setSelectedId(1, juce::dontSendNotification);
    recordBus.setTooltip("Record bus");
    addAndMakeVisible(recordBus);

    playbackStrip.onChange = [this]
    {
        engine.setPlaybackStrip(playbackStrip.getSelectedId() - 1);
    };
    recordBus.onChange = [this]
    {
        if (! engine.isRecording())
            engine.setRecordBus(recordBus.getSelectedId() - 1);
    };

    for (int bus = 0; bus < Mixer::busCount; ++bus)
    {
        busName[bus].setText(busNames[bus], juce::dontSendNotification);
        busName[bus].setFont(uiFont(13.0f, true));
        busName[bus].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(busName[bus]);

        busEq[bus].setButtonText("EQ");
        busMono[bus].setButtonText("Mono");
        busMute[bus].setButtonText("Mute");
        for (auto* button : { &busEq[bus], &busMono[bus], &busMute[bus] })
        {
            button->setComponentID("toggle");
            button->setClickingTogglesState(true);
            button->addListener(this);
            addAndMakeVisible(button);
        }

        configureKnob(busBass[bus], "Bass");
        busBass[bus].setRange(-1.0, 1.0, 0.01);
        configureKnob(busTreble[bus], "Treble");
        busTreble[bus].setRange(-1.0, 1.0, 0.01);

        busFader[bus].setSliderStyle(juce::Slider::LinearVertical);
        busFader[bus].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 52, 16);
        busFader[bus].setRange(Mixer::minimumDb, Mixer::maximumDb, 0.1);
        busFader[bus].setNumDecimalPlacesToDisplay(1);
        busFader[bus].setTextValueSuffix(" dB");
        busFader[bus].setDoubleClickReturnValue(true, 0.0);
        busFader[bus].setVelocityBasedMode(false);
        busFader[bus].setSliderSnapsToMousePosition(true);
        busFader[bus].setTitle(busNames[bus] + " fader");
        busFader[bus].addListener(this);
        addAndMakeVisible(busFader[bus]);
        busEq[bus].setTooltip("Right-click for parameters");
    }

    reverbReturn.setTooltip("Right-click for parameters");
    delayReturn.setTooltip("Right-click for parameters");
    addMouseListener(this, true);
}

void MasterPanel::configureKnob(juce::Slider& slider, const juce::String& title)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange(0.0, 1.0, 0.01);
    slider.setDoubleClickReturnValue(true, 0.0);
    slider.setVelocityBasedMode(false);
    slider.setTitle(title);
    slider.setTooltip(title);
    slider.addListener(this);
    addAndMakeVisible(slider);
}

void MasterPanel::refreshFromMixer()
{
    updating = true;
    reverbReturn.setValue(mixer.getReverbReturn(), juce::dontSendNotification);
    delayReturn.setValue(mixer.getDelayReturn(), juce::dontSendNotification);

    for (int bus = 0; bus < Mixer::busCount; ++bus)
    {
        busEq[bus].setToggleState(mixer.isBusEqEnabled(bus), juce::dontSendNotification);
        busMono[bus].setToggleState(mixer.isBusMono(bus), juce::dontSendNotification);
        busMute[bus].setToggleState(mixer.isBusMuted(bus), juce::dontSendNotification);
        busBass[bus].setValue(mixer.getBusBass(bus), juce::dontSendNotification);
        busTreble[bus].setValue(mixer.getBusTreble(bus), juce::dontSendNotification);
        busFader[bus].setValue(mixer.getBusFaderDb(bus), juce::dontSendNotification);
    }

    updating = false;
}

void MasterPanel::updateTransport()
{
    playButton.setToggleState(engine.isPlaybackPlaying(), juce::dontSendNotification);
    recordButton.setToggleState(engine.isRecording(), juce::dontSendNotification);
    const double seconds = engine.isRecording() ? engine.getRecordSeconds() : engine.getPlaybackSeconds();
    const juce::String prefix = engine.isRecording() ? "Record " : "Play ";
    timeLabel.setText(prefix + juce::String(seconds, 1) + " s", juce::dontSendNotification);
    const auto fileName = engine.getPlaybackName();
    fileLabel.setText(fileName.isEmpty() ? "No file" : fileName, juce::dontSendNotification);
    repaint();
}

void MasterPanel::paint(juce::Graphics& g)
{
    const auto* look = dynamic_cast<AppleMixerLookAndFeel*>(&getLookAndFeel());
    const auto text = look != nullptr ? look->getPalette().text : juce::Colour(0xffF5F5F7);
    const auto accent = look != nullptr ? look->getPalette().accent : juce::Colour(0xff0A84FF);
    const auto track = look != nullptr ? look->getPalette().track : juce::Colour(0xff3A3A3C);
    g.setColour(text);
    g.setFont(uiFont(10.0f));
    auto caption = [&g](juce::Slider& slider, const juce::String& text)
    {
        auto bounds = slider.getBounds();
        g.drawFittedText(text, bounds.getX(), bounds.getBottom() - 14, bounds.getWidth(), 14, juce::Justification::centred, 1);
    };
    caption(reverbReturn, "Reverb");
    caption(delayReturn, "Delay");

    for (int bus = 0; bus < Mixer::busCount; ++bus)
    {
        g.setColour(track);
        g.fillRoundedRectangle(meterArea[bus].toFloat(), 2.0f);
        const float level = juce::jlimit(0.0f, 1.0f, mixer.getBusMeter(bus));
        auto fill = meterArea[bus];
        fill = fill.removeFromBottom(juce::roundToInt((float) meterArea[bus].getHeight() * level));
        g.setColour(accent);
        g.fillRoundedRectangle(fill.toFloat(), 2.0f);
        g.setColour(text);
        caption(busBass[bus], "Bass");
        caption(busTreble[bus], "Treble");
    }
}

void MasterPanel::resized()
{
    auto area = getLocalBounds().reduced(8);
    auto header = area.removeFromTop(196);
    timeLabel.setBounds(header.removeFromTop(24));

    auto transport = header.removeFromTop(32);
    playButton.setBounds(transport.removeFromLeft(64).reduced(2, 0));
    stopButton.setBounds(transport.removeFromLeft(64).reduced(2, 0));
    recordButton.setBounds(transport.removeFromLeft(80).reduced(2, 0));
    fileButton.setBounds(transport.removeFromLeft(64).reduced(2, 0));

    fileLabel.setBounds(header.removeFromTop(18));
    header.removeFromTop(2);
    auto choiceLabels = header.removeFromTop(16);
    playIntoLabel.setBounds(choiceLabels.removeFromLeft(choiceLabels.getWidth() / 2));
    recordLabel.setBounds(choiceLabels);
    auto choices = header.removeFromTop(28);
    playbackStrip.setBounds(choices.removeFromLeft(choices.getWidth() / 2).reduced(2, 0));
    recordBus.setBounds(choices.reduced(2, 0));

    auto returns = header;
    returnLabel.setJustificationType(juce::Justification::centredLeft);
    returnLabel.setBounds(returns.removeFromTop(16).removeFromLeft(150));
    reverbReturn.setBounds(returns.removeFromLeft(72));
    delayReturn.setBounds(returns.removeFromLeft(72));

    const int columnWidth = juce::jmax(64, area.getWidth() / Mixer::busCount);
    for (int bus = 0; bus < Mixer::busCount; ++bus)
    {
        auto column = area.removeFromLeft(columnWidth).reduced(2, 0);
        busName[bus].setBounds(column.removeFromTop(20));
        busEq[bus].setBounds(column.removeFromTop(24).reduced(4, 0));
        auto knobs = column.removeFromTop(52);
        busBass[bus].setBounds(knobs.removeFromLeft(knobs.getWidth() / 2));
        busTreble[bus].setBounds(knobs);
        auto faderRow = column.removeFromTop(juce::jmax(80, column.getHeight() - 56));
        meterArea[bus] = faderRow.removeFromRight(8).reduced(0, 8);
        busFader[bus].setBounds(faderRow.reduced(6, 0));
        busMono[bus].setBounds(column.removeFromTop(24).reduced(2, 1));
        busMute[bus].setBounds(column.removeFromTop(24).reduced(2, 1));
    }
}

void MasterPanel::mouseDown(const juce::MouseEvent& event)
{
    if (! event.mods.isPopupMenu())
        return;

    const auto refresh = [this] { refreshFromMixer(); };
    auto* control = event.eventComponent;
    if (control == &reverbReturn)
        showDetail(DetailPage::reverb, mixer, -1, refresh);
    else if (control == &delayReturn)
        showDetail(DetailPage::delay, mixer, -1, refresh);
    else
        for (int bus = 0; bus < Mixer::busCount; ++bus)
            if (control == &busEq[bus] || control == &busBass[bus] || control == &busTreble[bus])
                showDetail(DetailPage::busEq, mixer, bus, refresh);
}

void MasterPanel::sliderValueChanged(juce::Slider* slider)
{
    if (updating || slider == nullptr)
        return;

    const float value = (float) slider->getValue();
    if (slider == &reverbReturn) mixer.setReverbReturn(value);
    else if (slider == &delayReturn) mixer.setDelayReturn(value);
    else
    {
        for (int bus = 0; bus < Mixer::busCount; ++bus)
        {
            if (slider == &busFader[bus]) mixer.setBusFaderDb(bus, value);
            else if (slider == &busBass[bus]) mixer.setBusBass(bus, value);
            else if (slider == &busTreble[bus]) mixer.setBusTreble(bus, value);
        }
    }
}

void MasterPanel::buttonClicked(juce::Button* button)
{
    if (button == &fileButton)
    {
        if (onChooseFile) onChooseFile();
        return;
    }
    if (button == &stopButton)
    {
        engine.stopPlayback();
        engine.stopRecording();
        updateTransport();
        return;
    }
    if (button == &playButton)
    {
        engine.setPlaybackPlaying(playButton.getToggleState());
        updateTransport();
        return;
    }
    if (button == &recordButton)
    {
        if (recordButton.getToggleState())
            recordButton.setToggleState(engine.startRecording(), juce::dontSendNotification);
        else
            engine.stopRecording();
        updateTransport();
        return;
    }

    if (updating || button == nullptr)
        return;

    for (int bus = 0; bus < Mixer::busCount; ++bus)
    {
        if (button == &busEq[bus]) mixer.setBusEqEnabled(bus, button->getToggleState());
        else if (button == &busMono[bus]) mixer.setBusMono(bus, button->getToggleState());
        else if (button == &busMute[bus]) mixer.setBusMuted(bus, button->getToggleState());
    }
}

} // namespace xenhusk
