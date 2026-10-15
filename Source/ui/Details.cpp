#include "Details.h"

#include "AppleMixerLookAndFeel.h"
#include "engine/Biquad.h"

#include <juce_audio_utils/juce_audio_utils.h>

namespace xenhusk
{
namespace
{

juce::Font uiFont(float size, bool bold = false)
{
    return juce::Font(juce::FontOptions("Segoe UI", size, bold ? juce::Font::bold : juce::Font::plain));
}

int typeToMenu(int type)
{
    const int menu[5] = { 1, 2, 3, 5, 4 };
    return menu[juce::jlimit(0, 4, type)];
}

int menuToType(int menuId)
{
    const int type[5] = { 0, 1, 2, 4, 3 };
    return type[juce::jlimit(0, 4, menuId - 1)];
}

Mixer::EqBand copiedBand[2][Mixer::eqBands];
bool hasCopiedBands = false;

class DetailEditor : public juce::Component,
                     private juce::Slider::Listener,
                     private juce::Button::Listener
{
public:
    DetailEditor(DetailPage sourcePage, Mixer& source, int sourceIndex, std::function<void()> changed)
        : page(sourcePage), mixer(source), index(sourceIndex), onChanged(std::move(changed))
    {
        if (page == DetailPage::eq || page == DetailPage::busEq)
            buildEq();
        else if (page == DetailPage::comp)
            buildComp();
        else if (page == DetailPage::gate)
            buildGate();
        else if (page == DetailPage::denoise)
            buildDenoise();
        else if (page == DetailPage::pan)
            buildPan();
        else if (page == DetailPage::reverb)
            buildReverb();
        else
            buildDelay();
    }

    void paint(juce::Graphics& g) override
    {
        const auto* look = dynamic_cast<AppleMixerLookAndFeel*>(&getLookAndFeel());
        g.fillAll(look != nullptr ? look->getPalette().background : juce::Colour(0xff1D1D1F));
        if (graph.isEmpty())
            return;

        g.setColour(juce::Colour(0xff2C2C2E));
        g.fillRoundedRectangle(graph.toFloat(), 8.0f);
        g.setColour(juce::Colour(0xff3A3A3C));
        for (int line = 1; line < 4; ++line)
        {
            const int y = graph.getY() + graph.getHeight() * line / 4;
            g.drawHorizontalLine(y, (float) graph.getX(), (float) graph.getRight());
        }

        juce::Path curve;
        const int steps = juce::jmax(2, graph.getWidth());
        for (int step = 0; step < steps; ++step)
        {
            const float ratio = (float) step / (float) (steps - 1);
            const float toneFrequency = 20.0f * std::pow(1000.0f, ratio);
            float magnitude = 0.0f;
            const int channel = channelChoice <= 1 ? 0 : 1;
            for (int band = 0; band < Mixer::eqBands; ++band)
            {
                const auto value = page == DetailPage::busEq ? mixer.getBusEqBand(index, band)
                                                            : mixer.getEqBand(index, channel, band);
                if (! value.enabled)
                    continue;

                Biquad filter;
                filter.update(value.type, 48000.0, value.frequency, value.gainDb, value.q);
                magnitude += filter.magnitudeDb(toneFrequency, 48000.0);
            }

            const float y = juce::jmap(juce::jlimit(-24.0f, 24.0f, magnitude), 24.0f, -24.0f,
                                       (float) graph.getY(), (float) graph.getBottom());
            const float x = (float) graph.getX() + (float) step;
            if (step == 0)
                curve.startNewSubPath(x, y);
            else
                curve.lineTo(x, y);
        }

        g.setColour(juce::Colour(0xff0A84FF));
        g.strokePath(curve, juce::PathStrokeType(2.0f));
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(16);
        if (! graph.isEmpty() || page == DetailPage::eq || page == DetailPage::busEq)
            graph = area.removeFromBottom(180).reduced(0, 8);

        if (page == DetailPage::eq || page == DetailPage::busEq)
        {
            auto tools = area.removeFromTop(32);
            const int width = 72;
            for (auto* button : { &flatButton, &slotA, &slotB, &allButton, &leftButton, &rightButton, &copyButton, &pasteButton })
                button->setBounds(tools.removeFromLeft(width).reduced(2, 0));
            if (page == DetailPage::busEq)
                limiterButton.setBounds(tools.removeFromLeft(width).reduced(2, 0));

            area.removeFromTop(8);
            auto captions = area.removeFromLeft(36);
            captions.removeFromTop(56);
            hzCaption.setBounds(captions.removeFromTop(36));
            dbCaption.setBounds(captions.removeFromTop(36));
            qCaption.setBounds(captions.removeFromTop(36));
            const int columnWidth = juce::jmax(80, area.getWidth() / Mixer::eqBands);
            for (int band = 0; band < Mixer::eqBands; ++band)
            {
                auto column = area.removeFromLeft(columnWidth).reduced(4, 0);
                bandOn[band].setBounds(column.removeFromTop(28));
                bandType[band].setBounds(column.removeFromTop(28));
                frequency[band].setBounds(column.removeFromTop(36));
                gain[band].setBounds(column.removeFromTop(36));
                q[band].setBounds(column.removeFromTop(36));
            }
            return;
        }

        auto row = area;
        for (int i = 0; i < rowCount; ++i)
        {
            auto line = row.removeFromTop(36);
            rowLabel[i].setBounds(line.removeFromLeft(140));
            rowSlider[i].setBounds(line);
        }

        auto buttons = row.removeFromTop(36);
        for (auto& button : presetButtons)
            if (button != nullptr)
                button->setBounds(buttons.removeFromLeft(110).reduced(4, 2));
    }

private:
    void buildEq()
    {
        setSize(980, 460);
        configureButton(flatButton, "Flat");
        configureButton(slotA, "A");
        configureButton(slotB, "B");
        configureButton(allButton, "All");
        configureButton(leftButton, "1");
        configureButton(rightButton, "2");
        configureButton(copyButton, "Copy");
        configureButton(pasteButton, "Paste");
        allButton.setToggleState(true, juce::dontSendNotification);
        const char* types[] = { "Peak", "Low shelf", "High shelf", "Low cut", "High cut" };

        for (int band = 0; band < Mixer::eqBands; ++band)
        {
            configureButton(bandOn[band], "On");
            bandOn[band].setClickingTogglesState(true);
            for (int type = 0; type < 5; ++type)
                bandType[band].addItem(types[type], type + 1);
            addAndMakeVisible(bandType[band]);
            bandType[band].onChange = [this, band] { pushBand(band); };
            configureSlider(frequency[band], 20.0, 20000.0, 0.1, 1000.0);
            frequency[band].setSkewFactorFromMidPoint(1000.0);
            frequency[band].setTextValueSuffix(" Hz");
            configureSlider(gain[band], -24.0, 24.0, 0.1, 0.0);
            gain[band].setTextValueSuffix(" dB");
            configureSlider(q[band], 0.1, 20.0, 0.01, 1.0);
            q[band].setTextValueSuffix(" Q");
        }

        configureCaption(hzCaption, "Hz");
        configureCaption(dbCaption, "dB");
        configureCaption(qCaption, "Q");
        if (page == DetailPage::busEq)
        {
            configureButton(limiterButton, "Limiter");
            limiterButton.setToggleState(mixer.isBusLimiter(index), juce::dontSendNotification);
        }

        for (int slot = 0; slot < 2; ++slot)
            for (int channel = 0; channel < 2; ++channel)
                for (int band = 0; band < Mixer::eqBands; ++band)
                    memory[slot][channel][band] = currentBand(channel, band);

        pullEq();
    }

    void buildComp()
    {
        setSize(520, 360);
        const auto settings = mixer.getCompSettings(index);
        addValueRow("Input", -24.0, 24.0, settings.inputDb, 0.0, [this](float value) { editComp([value](Mixer::CompSettings& settings) { settings.inputDb = value; }); });
        addValueRow("Threshold", -60.0, 0.0, settings.thresholdDb, -18.0, [this](float value) { editComp([value](Mixer::CompSettings& settings) { settings.thresholdDb = value; }); });
        addValueRow("Ratio", 1.0, 20.0, settings.ratio, 1.0, [this](float value) { editComp([value](Mixer::CompSettings& settings) { settings.ratio = value; }); });
        addValueRow("Attack", 0.1, 200.0, settings.attackMs, 10.0, [this](float value) { editComp([value](Mixer::CompSettings& settings) { settings.attackMs = value; }); });
        addValueRow("Release", 5.0, 2000.0, settings.releaseMs, 50.0, [this](float value) { editComp([value](Mixer::CompSettings& settings) { settings.releaseMs = value; }); });
        addValueRow("Knee", 0.0, 24.0, settings.knee, 6.0, [this](float value) { editComp([value](Mixer::CompSettings& settings) { settings.knee = value; }); });
        addValueRow("Output", -24.0, 24.0, settings.outputDb, 0.0, [this](float value) { editComp([value](Mixer::CompSettings& settings) { settings.outputDb = value; }); });
        addValueRow("Auto makeup", 0.0, 1.0, settings.autoMakeup ? 1.0f : 0.0f, 0.0, [this](float value) { editComp([value](Mixer::CompSettings& settings) { settings.autoMakeup = value > 0.5f; }); });
    }

    void buildGate()
    {
        setSize(520, 280);
        const auto settings = mixer.getGateSettings(index);
        addValueRow("Threshold", -80.0, 0.0, settings.thresholdDb, -40.0, [this](float value) { editGate([value](Mixer::GateSettings& settings) { settings.thresholdDb = value; }); });
        addValueRow("Range", -80.0, 0.0, settings.rangeDb, -60.0, [this](float value) { editGate([value](Mixer::GateSettings& settings) { settings.rangeDb = value; }); });
        addValueRow("Attack", 0.1, 200.0, settings.attackMs, 10.0, [this](float value) { editGate([value](Mixer::GateSettings& settings) { settings.attackMs = value; }); });
        addValueRow("Hold", 0.0, 2000.0, settings.holdMs, 200.0, [this](float value) { editGate([value](Mixer::GateSettings& settings) { settings.holdMs = value; }); });
        addValueRow("Release", 5.0, 2000.0, settings.releaseMs, 200.0, [this](float value) { editGate([value](Mixer::GateSettings& settings) { settings.releaseMs = value; }); });
    }

    void buildDenoise()
    {
        setSize(520, 120);
        addValueRow("Amount", 0.0, 1.0, mixer.getDenoise(index), 0.0, [this](float value)
        {
            mixer.setDenoise(index, value);
            changed();
        });
    }

    void buildPan()
    {
        setSize(520, 280);
        addValueRow("Position", -1.0, 1.0, mixer.getPan(index), 0.0, [this](float value) { mixer.setPan(index, value); changed(); });
        addValueRow("Modulation", 0.0, 1.0, mixer.getModulation(index), 0.0, [this](float value) { mixer.setModulation(index, value); changed(); });
        addValueRow("Color", -1.0, 1.0, mixer.getColor(index), 0.0, [this](float value) { mixer.setColor(index, value); changed(); });
        addValueRow("Voice", 0.0, 1.0, mixer.getVoice(index), 0.0, [this](float value) { mixer.setVoice(index, value); changed(); });
        addValueRow("Echo", 0.0, 1.0, mixer.getEcho(index), 0.0, [this](float value) { mixer.setEcho(index, value); changed(); });
    }

    void buildReverb()
    {
        setSize(560, 280);
        addValueRow("Decay", 0.2, 8.0, mixer.getReverbDecay(), 1.0, [this](float value) { mixer.setReverbDecay(value); changed(); });
        addValueRow("Pre-delay", 0.0, 0.1, mixer.getReverbPredelay(), 0.0, [this](float value) { mixer.setReverbPredelay(value); changed(); });
        addValueRow("Early", 0.0, 1.0, mixer.getReverbEarly(), 0.0, [this](float value) { mixer.setReverbEarly(value); changed(); });
        addValueRow("Wet", 0.0, 1.0, mixer.getReverbReturn(), 0.0, [this](float value) { mixer.setReverbReturn(value); changed(); });
        if (index >= 0)
            addValueRow("Send", 0.0, 1.0, mixer.getReverbSend(index), 0.0, [this](float value) { mixer.setReverbSend(index, value); changed(); });

        const char* names[] = { "Room", "Hall", "Plate", "Chamber" };
        const float decay[] = { 0.6f, 2.2f, 1.2f, 1.5f };
        const float predelay[] = { 0.01f, 0.03f, 0.0f, 0.02f };
        const float early[] = { 0.4f, 0.3f, 0.1f, 0.5f };
        for (int preset = 0; preset < 4; ++preset)
        {
            presetButtons[(size_t) preset] = std::make_unique<juce::TextButton>(names[preset]);
            presetButtons[(size_t) preset]->setComponentID("reset");
            const float chosenDecay = decay[preset];
            const float chosenPredelay = predelay[preset];
            const float chosenEarly = early[preset];
            presetButtons[(size_t) preset]->onClick = [this, chosenDecay, chosenPredelay, chosenEarly]
            {
                mixer.setReverbDecay(chosenDecay);
                mixer.setReverbPredelay(chosenPredelay);
                mixer.setReverbEarly(chosenEarly);
                if (mixer.getReverbReturn() <= 0.001f)
                    mixer.setReverbReturn(0.6f);
                changed();
                rowSlider[0].setValue(chosenDecay, juce::dontSendNotification);
                rowSlider[1].setValue(chosenPredelay, juce::dontSendNotification);
                rowSlider[2].setValue(chosenEarly, juce::dontSendNotification);
                rowSlider[3].setValue(mixer.getReverbReturn(), juce::dontSendNotification);
            };
            addAndMakeVisible(*presetButtons[(size_t) preset]);
        }
    }

    void buildDelay()
    {
        setSize(560, 720);
        addValueRow("Time", 0.01, 2.0, mixer.getDelayTime(), 0.18, [this](float value) { mixer.setDelayTime(value); mixer.setDelayTap(0, true, value, mixer.getDelayTapGain(0)); changed(); });
        addValueRow("Feedback", 0.0, 0.95, mixer.getDelayFeedback(), 0.35, [this](float value) { mixer.setDelayFeedback(value); changed(); });
        addValueRow("Wet", 0.0, 1.0, mixer.getDelayReturn(), 0.0, [this](float value) { mixer.setDelayReturn(value); changed(); });
        if (index >= 0)
            addValueRow("Send", 0.0, 1.0, mixer.getDelaySend(index), 0.0, [this](float value) { mixer.setDelaySend(index, value); changed(); });

        for (int tap = 0; tap < Mixer::delayTaps; ++tap)
        {
            addValueRow("Tap " + juce::String(tap + 1) + " on", 0.0, 1.0, mixer.isDelayTapOn(tap) ? 1.0f : 0.0f, tap == 0 ? 1.0 : 0.0,
                        [this, tap](float value) { mixer.setDelayTap(tap, value > 0.5f, mixer.getDelayTapTime(tap), mixer.getDelayTapGain(tap)); changed(); });
            addValueRow("Tap " + juce::String(tap + 1) + " time", 0.01, 2.0, mixer.getDelayTapTime(tap), 0.18f * (float) (tap + 1),
                        [this, tap](float value) { mixer.setDelayTap(tap, mixer.isDelayTapOn(tap), value, mixer.getDelayTapGain(tap)); changed(); });
            addValueRow("Tap " + juce::String(tap + 1) + " gain", 0.0, 1.0, mixer.getDelayTapGain(tap), tap == 0 ? 1.0 : 0.0,
                        [this, tap](float value) { mixer.setDelayTap(tap, mixer.isDelayTapOn(tap), mixer.getDelayTapTime(tap), value); changed(); });
        }
    }

    void addValueRow(const juce::String& name, double minimum, double maximum, double value, double reset, std::function<void(float)> apply)
    {
        auto& label = rowLabel[rowCount];
        auto& slider = rowSlider[rowCount];
        label.setText(name, juce::dontSendNotification);
        label.setFont(uiFont(13.0f));
        addAndMakeVisible(label);
        slider.setRange(minimum, maximum, maximum > 20.0 ? 0.1 : 0.01);
        slider.setTextValueSuffix(unitFor(name));
        slider.setValue(value, juce::dontSendNotification);
        slider.setDoubleClickReturnValue(true, reset);
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 22);
        slider.setVelocityBasedMode(false);
        slider.onValueChange = [apply, &slider] { apply((float) slider.getValue()); };
        addAndMakeVisible(slider);
        ++rowCount;
    }

    static juce::String unitFor(const juce::String& name)
    {
        if (name == "Ratio")
            return ":1";
        if (name == "Attack" || name == "Release" || name == "Hold")
            return " ms";
        if (name == "Time" || name == "Decay" || name == "Pre-delay" || name.endsWith("time"))
            return " s";
        if (name == "Input" || name == "Threshold" || name == "Knee" || name == "Output" || name == "Range")
            return " dB";
        return {};
    }

    void configureCaption(juce::Label& label, const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setFont(uiFont(11.0f));
        label.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(label);
    }

    void configureButton(juce::TextButton& button, const juce::String& text)
    {
        button.setButtonText(text);
        button.setComponentID(text == "Flat" || text == "Copy" || text == "Paste" ? "reset" : "toggle");
        button.setClickingTogglesState(button.getComponentID() == "toggle");
        button.addListener(this);
        addAndMakeVisible(button);
    }

    void configureSlider(juce::Slider& slider, double minimum, double maximum, double interval, double reset)
    {
        slider.setRange(minimum, maximum, interval);
        slider.setDoubleClickReturnValue(true, reset);
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 20);
        slider.setVelocityBasedMode(false);
        slider.addListener(this);
        addAndMakeVisible(slider);
    }

    Mixer::EqBand currentBand(int channel, int band) const
    {
        return page == DetailPage::busEq ? mixer.getBusEqBand(index, band) : mixer.getEqBand(index, channel, band);
    }

    void pullEq()
    {
        updating = true;
        const int channel = channelChoice <= 1 ? 0 : 1;
        for (int band = 0; band < Mixer::eqBands; ++band)
        {
            const auto value = currentBand(channel, band);
            bandOn[band].setToggleState(value.enabled, juce::dontSendNotification);
            bandType[band].setSelectedId(typeToMenu(value.type), juce::dontSendNotification);
            frequency[band].setValue(value.frequency, juce::dontSendNotification);
            gain[band].setValue(value.gainDb, juce::dontSendNotification);
            q[band].setValue(value.q, juce::dontSendNotification);
        }
        updating = false;
        repaint();
    }

    void pushBand(int band)
    {
        if (updating)
            return;

        Mixer::EqBand value;
        value.enabled = bandOn[band].getToggleState();
        value.type = menuToType(bandType[band].getSelectedId());
        value.frequency = (float) frequency[band].getValue();
        value.gainDb = (float) gain[band].getValue();
        value.q = (float) q[band].getValue();
        writeBand(band, value);
        if (page == DetailPage::eq)
            mixer.setEqEnabled(index, true);
        else
            mixer.setBusEqEnabled(index, true);
        changed();
        repaint();
    }

    void writeBand(int band, Mixer::EqBand value)
    {
        if (page == DetailPage::busEq)
        {
            mixer.setBusEqBand(index, band, value);
            return;
        }

        if (channelChoice == 0 || channelChoice == 1)
            mixer.setEqBand(index, 0, band, value);
        if (channelChoice == 0 || channelChoice == 2)
            mixer.setEqBand(index, 1, band, value);
    }

    void editComp(const std::function<void(Mixer::CompSettings&)>& edit)
    {
        auto settings = mixer.getCompSettings(index);
        settings.detailed = true;
        edit(settings);
        mixer.setCompSettings(index, settings);
        mixer.setComp(index, 1.0f);
        changed();
    }

    void editGate(const std::function<void(Mixer::GateSettings&)>& edit)
    {
        auto settings = mixer.getGateSettings(index);
        settings.detailed = true;
        edit(settings);
        mixer.setGateSettings(index, settings);
        mixer.setGate(index, 1.0f);
        changed();
    }

    void changed()
    {
        if (onChanged)
            onChanged();
    }

    void sliderValueChanged(juce::Slider* slider) override
    {
        for (int band = 0; band < Mixer::eqBands; ++band)
            if (slider == &frequency[band] || slider == &gain[band] || slider == &q[band])
                pushBand(band);
    }

    void buttonClicked(juce::Button* button) override
    {
        if (button == &limiterButton)
        {
            mixer.setBusLimiter(index, limiterButton.getToggleState());
            changed();
            return;
        }

        for (int band = 0; band < Mixer::eqBands; ++band)
            if (button == &bandOn[band])
                pushBand(band);

        if (button == &flatButton)
        {
            for (int band = 0; band < Mixer::eqBands; ++band)
            {
                auto value = currentBand(0, band);
                value.gainDb = 0.0f;
                writeBand(band, value);
            }
            pullEq();
            changed();
        }
        else if (button == &allButton || button == &leftButton || button == &rightButton)
        {
            channelChoice = button == &leftButton ? 1 : button == &rightButton ? 2 : 0;
            allButton.setToggleState(channelChoice == 0, juce::dontSendNotification);
            leftButton.setToggleState(channelChoice == 1, juce::dontSendNotification);
            rightButton.setToggleState(channelChoice == 2, juce::dontSendNotification);
            pullEq();
        }
        else if (button == &slotA || button == &slotB)
        {
            const int next = button == &slotA ? 0 : 1;
            storeMemory(activeSlot);
            activeSlot = next;
            recallMemory(activeSlot);
            slotA.setToggleState(activeSlot == 0, juce::dontSendNotification);
            slotB.setToggleState(activeSlot == 1, juce::dontSendNotification);
        }
        else if (button == &copyButton)
        {
            for (int channel = 0; channel < 2; ++channel)
                for (int band = 0; band < Mixer::eqBands; ++band)
                    copiedBand[channel][band] = currentBand(channel, band);
            hasCopiedBands = true;
        }
        else if (button == &pasteButton && hasCopiedBands)
        {
            for (int channel = 0; channel < 2; ++channel)
                for (int band = 0; band < Mixer::eqBands; ++band)
                    if (page == DetailPage::eq)
                        mixer.setEqBand(index, channel, band, copiedBand[channel][band]);
                    else
                        mixer.setBusEqBand(index, band, copiedBand[0][band]);
            pullEq();
            changed();
        }
    }

    void storeMemory(int slot)
    {
        for (int channel = 0; channel < 2; ++channel)
            for (int band = 0; band < Mixer::eqBands; ++band)
                memory[slot][channel][band] = currentBand(channel, band);
    }

    void recallMemory(int slot)
    {
        for (int channel = 0; channel < 2; ++channel)
            for (int band = 0; band < Mixer::eqBands; ++band)
                if (page == DetailPage::eq)
                    mixer.setEqBand(index, channel, band, memory[slot][channel][band]);
                else if (channel == 0)
                    mixer.setBusEqBand(index, band, memory[slot][0][band]);
        pullEq();
        changed();
    }

    DetailPage page;
    Mixer& mixer;
    int index = 0;
    std::function<void()> onChanged;
    bool updating = false;
    int channelChoice = 0;
    int activeSlot = 0;
    int rowCount = 0;
    Mixer::EqBand memory[2][2][Mixer::eqBands];
    juce::Rectangle<int> graph;
    juce::TextButton flatButton, slotA, slotB, allButton, leftButton, rightButton, copyButton, pasteButton, limiterButton;
    juce::Label hzCaption, dbCaption, qCaption;
    juce::TextButton bandOn[Mixer::eqBands];
    juce::ComboBox bandType[Mixer::eqBands];
    juce::Slider frequency[Mixer::eqBands], gain[Mixer::eqBands], q[Mixer::eqBands];
    juce::Label rowLabel[16];
    juce::Slider rowSlider[16];
    std::unique_ptr<juce::TextButton> presetButtons[4];
};

class SystemPage : public juce::Component
{
public:
    SystemPage(juce::AudioDeviceManager& devices, Mixer& source, std::function<void()> save, std::function<void()> load, std::function<void()> changed)
        : mixer(source), onSave(std::move(save)), onLoad(std::move(load)), onChanged(std::move(changed)),
          selector(devices, 0, 16, 0, 2, false, false, true, false)
    {
        setOpaque(true);
        addAndMakeVisible(selector);
        saveButton.setButtonText("Save");
        loadButton.setButtonText("Load");
        saveButton.setComponentID("reset");
        loadButton.setComponentID("reset");
        saveButton.onClick = [this] { if (onSave) onSave(); };
        loadButton.onClick = [this] { if (onLoad) onLoad(); pull(); };
        addAndMakeVisible(saveButton);
        addAndMakeVisible(loadButton);

        limiter.setButtonText("A1 limiter");
        limiter.setComponentID("toggle");
        limiter.setClickingTogglesState(true);
        limiter.setToggleState(mixer.isBusLimiter(0), juce::dontSendNotification);
        limiter.onClick = [this]
        {
            mixer.setBusLimiter(0, limiter.getToggleState());
            if (onChanged) onChanged();
        };
        addAndMakeVisible(limiter);

        for (int strip = 0; strip < Mixer::stripCount; ++strip)
        {
            pairLabel[strip].setText(mixer.getStripName(strip) + " input", juce::dontSendNotification);
            pairLabel[strip].setFont(uiFont(12.0f));
            addAndMakeVisible(pairLabel[strip]);
            for (int pair = 0; pair < Mixer::stripCount; ++pair)
                inputPair[strip].addItem("Pair " + juce::String(pair + 1), pair + 1);
            inputPair[strip].setSelectedId(mixer.getInputPair(strip) + 1, juce::dontSendNotification);
            inputPair[strip].onChange = [this, strip]
            {
                mixer.setInputPair(strip, inputPair[strip].getSelectedId() - 1);
                if (onChanged) onChanged();
            };
            addAndMakeVisible(inputPair[strip]);
        }

        setSize(640, 760);
    }

    void paint(juce::Graphics& g) override
    {
        const auto* look = dynamic_cast<AppleMixerLookAndFeel*>(&getLookAndFeel());
        g.fillAll(look != nullptr ? look->getPalette().background : juce::Colour(0xff1D1D1F));
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(12);
        selector.setBounds(area.removeFromTop(430));
        auto tools = area.removeFromTop(36);
        saveButton.setBounds(tools.removeFromLeft(100).reduced(4, 2));
        loadButton.setBounds(tools.removeFromLeft(100).reduced(4, 2));
        limiter.setBounds(tools.removeFromLeft(140).reduced(4, 2));
        for (int strip = 0; strip < Mixer::stripCount; ++strip)
        {
            auto line = area.removeFromTop(32);
            pairLabel[strip].setBounds(line.removeFromLeft(180));
            inputPair[strip].setBounds(line.reduced(4, 2));
        }
    }

private:
    void pull()
    {
        limiter.setToggleState(mixer.isBusLimiter(0), juce::dontSendNotification);
        for (int strip = 0; strip < Mixer::stripCount; ++strip)
        {
            pairLabel[strip].setText(mixer.getStripName(strip) + " input", juce::dontSendNotification);
            inputPair[strip].setSelectedId(mixer.getInputPair(strip) + 1, juce::dontSendNotification);
        }
        if (onChanged) onChanged();
    }

    Mixer& mixer;
    std::function<void()> onSave, onLoad, onChanged;
    juce::AudioDeviceSelectorComponent selector;
    juce::TextButton saveButton, loadButton, limiter;
    juce::Label pairLabel[Mixer::stripCount];
    juce::ComboBox inputPair[Mixer::stripCount];
};

void launch(const juce::String& title, std::unique_ptr<juce::Component> content)
{
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(content.release());
    options.dialogTitle = title;
    const auto* look = dynamic_cast<AppleMixerLookAndFeel*>(&juce::LookAndFeel::getDefaultLookAndFeel());
    options.dialogBackgroundColour = look != nullptr ? look->getPalette().background : juce::Colour(0xff1D1D1F);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;
    options.launchAsync();
}

} // namespace

void showDetail(DetailPage page, Mixer& mixer, int index, std::function<void()> onChanged)
{
    juce::String title = "Parameters";
    if (page == DetailPage::eq || page == DetailPage::busEq) title = "Equalizer";
    else if (page == DetailPage::comp) title = "Compressor";
    else if (page == DetailPage::gate) title = "Gate";
    else if (page == DetailPage::denoise) title = "Denoise";
    else if (page == DetailPage::pan) title = "IntelliPan";
    else if (page == DetailPage::reverb) title = "Reverb";
    else if (page == DetailPage::delay) title = "Delay";

    launch(title, std::make_unique<DetailEditor>(page, mixer, index, std::move(onChanged)));
}

void showSystemPage(juce::AudioDeviceManager& devices, Mixer& mixer, std::function<void()> onSave, std::function<void()> onLoad, std::function<void()> onChanged)
{
    launch("System settings", std::make_unique<SystemPage>(devices, mixer, std::move(onSave), std::move(onLoad), std::move(onChanged)));
}

} // namespace xenhusk
