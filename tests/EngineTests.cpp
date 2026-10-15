#include "engine/Mixer.h"
#include "engine/RoutingMatrix.h"
#include "engine/SettingsDocument.h"

#include <cmath>
#include <iostream>

namespace
{

int failures = 0;

void expect(bool condition, const char* message)
{
    if (! condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

bool near(float actual, float expected)
{
    return std::abs(actual - expected) < 1.0e-4f;
}

} // namespace

int main()
{
    expect(near(xenhusk::dbToLinear(0.0f), 1.0f), "0 dB is unity");
    expect(xenhusk::dbToLinear(-100.0f) == 0.0f, "-100 dB is silence");
    expect(xenhusk::dbToLinear(-120.0f) == 0.0f, "below -100 dB is silence");
    expect(near(xenhusk::linearToDb(1.0f), 0.0f), "unity is 0 dB");
    expect(xenhusk::linearToDb(0.0f) == -100.0f, "zero linear is -100 dB");

    xenhusk::RoutingMatrix matrix;
    juce::AudioBuffer<float> input(xenhusk::RoutingMatrix::stripCount, 4);
    juce::AudioBuffer<float> output(xenhusk::RoutingMatrix::busCount, 4);
    input.clear();
    input.setSample(0, 0, 1.0f);

    matrix.process(input, output);
    expect(near(output.getSample(0, 0), 1.0f), "diagonal route is unity");
    expect(output.getSample(1, 0) == 0.0f, "default off-diagonal is silence");

    matrix.setGainDb(0, 1, 0.0f);
    matrix.process(input, output);
    expect(near(output.getSample(1, 0), 1.0f), "off-diagonal crosspoint passes the strip");
    expect(near(output.getSample(0, 0), 1.0f), "diagonal stays unity");

    matrix.setStripMuted(0, true);
    matrix.process(input, output);
    expect(output.getSample(0, 0) == 0.0f, "strip mute silences the diagonal");
    expect(output.getSample(1, 0) == 0.0f, "strip mute silences the crosspoint");

    matrix.setStripMuted(0, false);
    matrix.setBusMuted(1, true);
    matrix.process(input, output);
    expect(near(output.getSample(0, 0), 1.0f), "bus mute leaves the other bus");
    expect(output.getSample(1, 0) == 0.0f, "bus mute silences that column");

    matrix.resetToDefault();
    matrix.process(input, output);
    expect(near(output.getSample(0, 0), 1.0f), "reset restores the diagonal");
    expect(output.getSample(1, 0) == 0.0f, "reset clears the crosspoint");
    expect(! matrix.isStripMuted(0) && ! matrix.isBusMuted(1), "reset clears mute");

    xenhusk::Mixer mixer;
    mixer.prepare(48000.0);
    juce::AudioBuffer<float> device(2, 4);
    juce::AudioBuffer<float> hardware(2, 4);
    device.clear();
    device.setSample(0, 0, 1.0f);
    device.setSample(1, 0, 1.0f);
    mixer.process(device, nullptr, 0, hardware, 0, nullptr);
    expect(near(hardware.getSample(0, 0), 1.0f), "strip fader at 0 dB reaches A1");
    expect(near(hardware.getSample(1, 0), 1.0f), "right channel reaches A1");

    mixer.setAssigned(0, 0, false);
    mixer.process(device, nullptr, 0, hardware, 0, nullptr);
    expect(hardware.getSample(0, 0) == 0.0f, "unassigned strip does not reach A1");

    mixer.setAssigned(0, 0, true);
    mixer.setStripMuted(0, true);
    mixer.process(device, nullptr, 0, hardware, 0, nullptr);
    expect(hardware.getSample(0, 0) == 0.0f, "strip mute silences A1");

    mixer.setStripMuted(0, false);
    mixer.setStripSolo(1, true);
    mixer.process(device, nullptr, 0, hardware, 0, nullptr);
    expect(hardware.getSample(0, 0) == 0.0f, "another solo silences this strip");

    mixer.setStripSolo(1, false);
    mixer.setBusMuted(0, true);
    mixer.process(device, nullptr, 0, hardware, 0, nullptr);
    expect(hardware.getSample(0, 0) == 0.0f, "bus mute silences A1");

    mixer.setBusMuted(0, false);
    mixer.setGate(0, 1.0f);
    juce::AudioBuffer<float> quiet(2, 400);
    juce::AudioBuffer<float> quietOut(2, 400);
    for (int channel = 0; channel < quiet.getNumChannels(); ++channel)
        juce::FloatVectorOperations::fill(quiet.getWritePointer(channel), 0.001f, quiet.getNumSamples());
    mixer.process(quiet, nullptr, 0, quietOut, 0, nullptr);
    expect(std::abs(quietOut.getSample(0, 399)) < 0.01f, "gate silences a quiet signal");

    juce::AudioBuffer<float> loud(2, 800);
    juce::AudioBuffer<float> loudOut(2, 800);
    for (int channel = 0; channel < loud.getNumChannels(); ++channel)
        juce::FloatVectorOperations::fill(loud.getWritePointer(channel), 1.0f, loud.getNumSamples());
    mixer.process(loud, nullptr, 0, loudOut, 0, nullptr);
    expect(near(loudOut.getSample(0, 799), 1.0f), "gate lets a full-scale signal through");

    mixer.setComp(0, 1.0f);
    xenhusk::Mixer::CompSettings compression;
    compression.detailed = true;
    compression.thresholdDb = -6.0f;
    compression.ratio = 20.0f;
    compression.attackMs = 1.0f;
    compression.releaseMs = 20.0f;
    compression.knee = 0.0f;
    mixer.setCompSettings(0, compression);
    juce::AudioBuffer<float> loudComp(2, 4000);
    juce::AudioBuffer<float> loudCompOut(2, 4000);
    for (int channel = 0; channel < loudComp.getNumChannels(); ++channel)
        juce::FloatVectorOperations::fill(loudComp.getWritePointer(channel), 1.0f, loudComp.getNumSamples());
    mixer.process(loudComp, nullptr, 0, loudCompOut, 0, nullptr);
    expect(std::abs(loudCompOut.getSample(0, 3999)) < 0.7f, "detailed compressor reduces a full-scale signal");

    mixer.setComp(0, 0.0f);
    xenhusk::Mixer::EqBand band;
    band.enabled = true;
    band.type = 4;
    band.frequency = 1000.0f;
    band.gainDb = 0.0f;
    band.q = 0.7f;
    mixer.setEqEnabled(0, true);
    mixer.setEqBand(0, 0, 0, band);
    mixer.setEqBand(0, 1, 0, band);
    for (int other = 1; other < xenhusk::Mixer::eqBands; ++other)
    {
        auto silent = mixer.getEqBand(0, 0, other);
        silent.enabled = false;
        mixer.setEqBand(0, 0, other, silent);
        mixer.setEqBand(0, 1, other, silent);
    }

    auto toneRms = [&mixer](float frequency)
    {
        juce::AudioBuffer<float> tone(2, 2000);
        juce::AudioBuffer<float> toneOut(2, 2000);
        for (int sample = 0; sample < tone.getNumSamples(); ++sample)
        {
            const float value = std::sin(2.0f * 3.14159265f * frequency * (float) sample / 48000.0f);
            tone.setSample(0, sample, value);
            tone.setSample(1, sample, value);
        }
        mixer.process(tone, nullptr, 0, toneOut, 0, nullptr);
        double energy = 0.0;
        for (int sample = 1000; sample < 2000; ++sample)
            energy += (double) toneOut.getSample(0, sample) * toneOut.getSample(0, sample);
        return energy / 1000.0;
    };
    expect(toneRms(200.0f) < toneRms(4000.0f), "low cut reduces a low tone more than a high tone");

    mixer.setFaderDb(0, 6.5f);
    mixer.setStripName(0, "PODCAST");
    const auto saved = xenhusk::settingsToXml(mixer);
    xenhusk::Mixer restored;
    restored.prepare(48000.0);
    expect(saved != nullptr && xenhusk::applySettingsXml(restored, *saved), "settings xml loads");
    expect(near(restored.getFaderDb(0), 6.5f), "settings keep the fader");
    expect(restored.getStripName(0) == "PODCAST", "settings keep the strip name");
    expect(near(restored.getEqBand(0, 0, 0).frequency, 1000.0f), "settings keep the EQ frequency");

    const auto voice = juce::XmlDocument::parse("<VBAudioVoicemeeterSettings><VoiceMeeterParameters><LabelStrip1>PODCAST</LabelStrip1><Strip index='1' mute='1' EQon='1' prg_c='1' busa='0' busb='1' dblevel='6.58'/><StripComp index='1' gainin='15.78' attack='10.0' release='50.0' knee='0.50' comprate='2.028' threshold='-18.27' automakeup='0' gainout='0.00'/></VoiceMeeterParameters><VoiceMeeterStripEQ><Strip index='1' channel='1' cell='2' EQon='1' EQtype='0' dblevel='14.50' freq='120.00' Q='2.00'/></VoiceMeeterStripEQ></VBAudioVoicemeeterSettings>");
    xenhusk::Mixer imported;
    imported.prepare(48000.0);
    expect(voice != nullptr && xenhusk::applySettingsXml(imported, *voice), "VoiceMeeter settings load");
    expect(imported.getStripName(0) == "PODCAST", "VoiceMeeter label becomes the strip name");
    expect(imported.isStripMuted(0), "VoiceMeeter mute loads");
    expect(near(imported.getFaderDb(0), 6.58f), "VoiceMeeter fader loads");
    expect(imported.isAssigned(0, 5) && ! imported.isAssigned(0, 0), "VoiceMeeter bus assign loads");
    expect(near(imported.getEqBand(0, 0, 1).frequency, 120.0f), "VoiceMeeter EQ frequency loads");
    expect(imported.getCompSettings(0).detailed && near(imported.getCompSettings(0).thresholdDb, -18.27f), "VoiceMeeter compressor loads");

    const auto doubled = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("xenhusk-doubled-parameters.xml");
    doubled.replaceWithText("<VBAudioVoicemeeterSettings><VoiceMeeterParameters>\n<VoiceMeeterParameters><LabelStrip1>PODCAST</LabelStrip1><LabelStrip2>MUSIC</LabelStrip2><Strip index='1' mute='0' EQon='1' busa='0' busb='1' dblevel='6.58'/><Strip index='2' mute='1' dblevel='5.00' busa='0' busb='1'/></VoiceMeeterParameters></VBAudioVoicemeeterSettings>");
    xenhusk::Mixer doubledMix;
    expect(xenhusk::loadSettingsFile(doubledMix, doubled), "duplicated VoiceMeeter parameters tag still loads");
    expect(doubledMix.getStripName(0) == "PODCAST" && doubledMix.getStripName(1) == "MUSIC", "duplicated file keeps both names");
    expect(near(doubledMix.getFaderDb(0), 6.58f) && ! doubledMix.isAssigned(0, 0) && doubledMix.isAssigned(0, 5), "duplicated file keeps strip 1");
    expect(doubledMix.isStripMuted(1) && near(doubledMix.getFaderDb(1), 5.0f), "duplicated file keeps strip 2");
    doubled.deleteFile();

    const auto podcast = juce::File("C:/Users/desuy/OneDrive/\xe6\x96\x87\xe6\xa1\xa3/GitHub/VoiceMeter REed/podcast voice.xml");
    if (podcast.existsAsFile())
    {
        xenhusk::Mixer podcastMix;
        expect(xenhusk::loadSettingsFile(podcastMix, podcast), "podcast voice.xml loads");
        expect(podcastMix.getStripName(0) == "PODCAST", "podcast strip 1 is PODCAST");
        expect(near(podcastMix.getFaderDb(0), 6.58f) && podcastMix.isEqEnabled(0), "podcast strip 1 fader and EQ");
        expect(! podcastMix.isAssigned(0, 0) && podcastMix.isAssigned(0, 5), "podcast strip 1 routes to B1");
        expect(podcastMix.getStripName(1) == "MUSIC" && podcastMix.isStripMuted(1) && near(podcastMix.getFaderDb(1), 5.0f), "podcast strip 2 is muted MUSIC");
    }

    if (failures == 0)
    {
        std::cout << "All engine tests passed\n";
        return 0;
    }

    std::cerr << failures << " test(s) failed\n";
    return 1;
}
