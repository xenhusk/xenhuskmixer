#include "Mixer.h"

#include <algorithm>
#include <cmath>

namespace
{
const float defaultEqFrequency[xenhusk::Mixer::eqBands] = { 80.0f, 200.0f, 800.0f, 2000.0f, 5000.0f, 12000.0f };
const char* defaultStripName[xenhusk::Mixer::stripCount] = { "Mic", "Music", "In 3", "In 4", "In 5", "Virtual 1", "Virtual 2", "Virtual 3" };
}

namespace xenhusk
{
namespace
{

float tap(std::vector<float>& line, int& pos, float input, int delaySamples, float feedback)
{
    if (line.empty())
        return input;

    const int size = (int) line.size();
    delaySamples = std::clamp(delaySamples, 1, size - 1);
    int read = pos - delaySamples;
    if (read < 0)
        read += size;

    const float delayed = line[(size_t) read];
    line[(size_t) pos] = input + delayed * feedback;
    if (++pos >= size)
        pos = 0;

    return delayed;
}

float readLine(const std::vector<float>& line, int pos, int delaySamples)
{
    if (line.empty())
        return 0.0f;

    const int size = (int) line.size();
    delaySamples = std::clamp(delaySamples, 0, size - 1);
    int read = pos - delaySamples;
    if (read < 0)
        read += size;

    return line[(size_t) read];
}

float msCoeff(float milliseconds, double rate)
{
    const float samples = std::max(1.0f, milliseconds * 0.001f * (float) rate);
    return 1.0f - std::exp(-1.0f / samples);
}

} // namespace

Mixer::Mixer()
{
    for (int strip = 0; strip < stripCount; ++strip)
        gateSmooth[strip] = 1.0f;

    resetToDefault();
}

float Mixer::unit(float value) noexcept
{
    return std::clamp(value, 0.0f, 1.0f);
}

float Mixer::bipolar(float value) noexcept
{
    return std::clamp(value, -1.0f, 1.0f);
}

void Mixer::prepare(double newSampleRate)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 48000.0;
    const int echoSamples = (int) (sampleRate * 0.25);
    const int delaySamples = (int) (sampleRate * 4.0);
    const int predelaySamples = (int) (sampleRate * 0.1);
    const int reverbSamples[4] = {
        (int) (sampleRate * 0.029),
        (int) (sampleRate * 0.037),
        (int) (sampleRate * 0.041),
        (int) (sampleRate * 0.053)
    };

    for (int strip = 0; strip < stripCount; ++strip)
        for (int channel = 0; channel < 2; ++channel)
            echoLine[strip][channel].assign((size_t) echoSamples, 0.0f);

    for (int channel = 0; channel < 2; ++channel)
    {
        delayLine[channel].assign((size_t) std::max(8, delaySamples), 0.0f);
        predelayLine[channel].assign((size_t) std::max(8, predelaySamples), 0.0f);
        for (int tapIndex = 0; tapIndex < 4; ++tapIndex)
            reverbLine[tapIndex][channel].assign((size_t) std::max(8, reverbSamples[tapIndex]), 0.0f);
    }
}

void Mixer::resetToDefault()
{
    for (int strip = 0; strip < stripCount; ++strip)
    {
        faderDb[strip].store(0.0f, std::memory_order_relaxed);
        faderGain[strip].store(1.0f, std::memory_order_relaxed);
        stripMute[strip].store(false, std::memory_order_relaxed);
        stripSolo[strip].store(false, std::memory_order_relaxed);
        stripMono[strip].store(false, std::memory_order_relaxed);
        eqEnabled[strip].store(false, std::memory_order_relaxed);
        postFx[strip].store(true, std::memory_order_relaxed);
        color[strip].store(0.0f, std::memory_order_relaxed);
        voice[strip].store(0.0f, std::memory_order_relaxed);
        echo[strip].store(0.0f, std::memory_order_relaxed);
        comp[strip].store(0.0f, std::memory_order_relaxed);
        gate[strip].store(0.0f, std::memory_order_relaxed);
        denoise[strip].store(0.0f, std::memory_order_relaxed);
        bass[strip].store(0.0f, std::memory_order_relaxed);
        treble[strip].store(0.0f, std::memory_order_relaxed);
        reverbSend[strip].store(0.0f, std::memory_order_relaxed);
        delaySend[strip].store(0.0f, std::memory_order_relaxed);
        stripMeter[strip].store(0.0f, std::memory_order_relaxed);
        stripName[strip] = defaultStripName[strip];
        pan[strip].store(0.0f, std::memory_order_relaxed);
        modulation[strip].store(0.0f, std::memory_order_relaxed);
        inputPair[strip].store(strip, std::memory_order_relaxed);
        compDetail[strip].store(false, std::memory_order_relaxed);
        compInDb[strip].store(0.0f, std::memory_order_relaxed);
        compThr[strip].store(-18.0f, std::memory_order_relaxed);
        compRatio[strip].store(1.0f, std::memory_order_relaxed);
        compAttack[strip].store(10.0f, std::memory_order_relaxed);
        compRelease[strip].store(50.0f, std::memory_order_relaxed);
        compKnee[strip].store(6.0f, std::memory_order_relaxed);
        compOutDb[strip].store(0.0f, std::memory_order_relaxed);
        compAuto[strip].store(false, std::memory_order_relaxed);
        gateDetail[strip].store(false, std::memory_order_relaxed);
        gateThr[strip].store(-40.0f, std::memory_order_relaxed);
        gateRange[strip].store(-60.0f, std::memory_order_relaxed);
        gateAttack[strip].store(10.0f, std::memory_order_relaxed);
        gateHold[strip].store(200.0f, std::memory_order_relaxed);
        gateRelease[strip].store(200.0f, std::memory_order_relaxed);

        for (int channel = 0; channel < 2; ++channel)
            for (int band = 0; band < eqBands; ++band)
            {
                eqBandOn[strip][channel][band].store(true, std::memory_order_relaxed);
                eqBandType[strip][channel][band].store(0, std::memory_order_relaxed);
                eqBandFreq[strip][channel][band].store(defaultEqFrequency[band], std::memory_order_relaxed);
                eqBandGain[strip][channel][band].store(0.0f, std::memory_order_relaxed);
                eqBandQ[strip][channel][band].store(1.0f, std::memory_order_relaxed);
            }

        for (int bus = 0; bus < busCount; ++bus)
            assign[strip][bus].store(bus == 0, std::memory_order_relaxed);
    }

    for (int bus = 0; bus < busCount; ++bus)
    {
        busFaderDb[bus].store(0.0f, std::memory_order_relaxed);
        busFaderGain[bus].store(1.0f, std::memory_order_relaxed);
        busMute[bus].store(false, std::memory_order_relaxed);
        busMono[bus].store(false, std::memory_order_relaxed);
        busEq[bus].store(false, std::memory_order_relaxed);
        busBass[bus].store(0.0f, std::memory_order_relaxed);
        busTreble[bus].store(0.0f, std::memory_order_relaxed);
        busMeter[bus].store(0.0f, std::memory_order_relaxed);
        busLimiter[bus].store(false, std::memory_order_relaxed);

        for (int band = 0; band < eqBands; ++band)
        {
            busEqBandOn[bus][band].store(true, std::memory_order_relaxed);
            busEqBandType[bus][band].store(0, std::memory_order_relaxed);
            busEqBandFreq[bus][band].store(defaultEqFrequency[band], std::memory_order_relaxed);
            busEqBandGain[bus][band].store(0.0f, std::memory_order_relaxed);
            busEqBandQ[bus][band].store(1.0f, std::memory_order_relaxed);
        }
    }

    reverbReturn.store(0.0f, std::memory_order_relaxed);
    delayReturn.store(0.0f, std::memory_order_relaxed);
    reverbDecay.store(1.0f, std::memory_order_relaxed);
    reverbPredelay.store(0.0f, std::memory_order_relaxed);
    reverbEarly.store(0.0f, std::memory_order_relaxed);
    delayTime.store(0.18f, std::memory_order_relaxed);
    delayFeedback.store(0.35f, std::memory_order_relaxed);

    for (int tapIndex = 0; tapIndex < delayTaps; ++tapIndex)
    {
        delayTapOn[tapIndex].store(tapIndex == 0, std::memory_order_relaxed);
        delayTapTime[tapIndex].store(0.18f * (float) (tapIndex + 1), std::memory_order_relaxed);
        delayTapGain[tapIndex].store(tapIndex == 0 ? 1.0f : 0.0f, std::memory_order_relaxed);
    }
}

void Mixer::setFaderDb(int strip, float db)
{
    if (strip < 0 || strip >= stripCount)
        return;

    db = std::clamp(db, minimumDb, maximumDb);
    faderDb[strip].store(db, std::memory_order_relaxed);
    faderGain[strip].store(dbToLinear(db <= minimumDb ? -100.0f : db), std::memory_order_relaxed);
}

float Mixer::getFaderDb(int strip) const
{
    return strip >= 0 && strip < stripCount ? load(faderDb[strip]) : minimumDb;
}

void Mixer::setStripMuted(int strip, bool muted)
{
    if (strip >= 0 && strip < stripCount)
        stripMute[strip].store(muted, std::memory_order_relaxed);
}

void Mixer::setStripSolo(int strip, bool solo)
{
    if (strip >= 0 && strip < stripCount)
        stripSolo[strip].store(solo, std::memory_order_relaxed);
}

void Mixer::setStripMono(int strip, bool mono)
{
    if (strip >= 0 && strip < stripCount)
        stripMono[strip].store(mono, std::memory_order_relaxed);
}

void Mixer::setEqEnabled(int strip, bool enabled)
{
    if (strip >= 0 && strip < stripCount)
        eqEnabled[strip].store(enabled, std::memory_order_relaxed);
}

void Mixer::setPostFx(int strip, bool post)
{
    if (strip >= 0 && strip < stripCount)
        postFx[strip].store(post, std::memory_order_relaxed);
}

void Mixer::setAssigned(int strip, int bus, bool assigned)
{
    if (strip >= 0 && strip < stripCount && bus >= 0 && bus < busCount)
        assign[strip][bus].store(assigned, std::memory_order_relaxed);
}

void Mixer::setColor(int strip, float value)
{
    if (strip >= 0 && strip < stripCount)
        color[strip].store(bipolar(value), std::memory_order_relaxed);
}

void Mixer::setVoice(int strip, float value)
{
    if (strip >= 0 && strip < stripCount)
        voice[strip].store(unit(value), std::memory_order_relaxed);
}

void Mixer::setEcho(int strip, float value)
{
    if (strip >= 0 && strip < stripCount)
        echo[strip].store(unit(value), std::memory_order_relaxed);
}

void Mixer::setComp(int strip, float value)
{
    if (strip >= 0 && strip < stripCount)
        comp[strip].store(unit(value), std::memory_order_relaxed);
}

void Mixer::setGate(int strip, float value)
{
    if (strip >= 0 && strip < stripCount)
        gate[strip].store(unit(value), std::memory_order_relaxed);
}

void Mixer::setDenoise(int strip, float value)
{
    if (strip >= 0 && strip < stripCount)
        denoise[strip].store(unit(value), std::memory_order_relaxed);
}

void Mixer::setBass(int strip, float value)
{
    if (strip >= 0 && strip < stripCount)
        bass[strip].store(bipolar(value), std::memory_order_relaxed);
}

void Mixer::setTreble(int strip, float value)
{
    if (strip >= 0 && strip < stripCount)
        treble[strip].store(bipolar(value), std::memory_order_relaxed);
}

void Mixer::setReverbSend(int strip, float value)
{
    if (strip >= 0 && strip < stripCount)
        reverbSend[strip].store(unit(value), std::memory_order_relaxed);
}

void Mixer::setDelaySend(int strip, float value)
{
    if (strip >= 0 && strip < stripCount)
        delaySend[strip].store(unit(value), std::memory_order_relaxed);
}

bool Mixer::isStripMuted(int strip) const { return strip >= 0 && strip < stripCount && load(stripMute[strip]); }
bool Mixer::isStripSolo(int strip) const { return strip >= 0 && strip < stripCount && load(stripSolo[strip]); }
bool Mixer::isStripMono(int strip) const { return strip >= 0 && strip < stripCount && load(stripMono[strip]); }
bool Mixer::isEqEnabled(int strip) const { return strip >= 0 && strip < stripCount && load(eqEnabled[strip]); }
bool Mixer::isPostFx(int strip) const { return strip >= 0 && strip < stripCount && load(postFx[strip]); }
bool Mixer::isAssigned(int strip, int bus) const
{
    return strip >= 0 && strip < stripCount && bus >= 0 && bus < busCount && load(assign[strip][bus]);
}
float Mixer::getColor(int strip) const { return strip >= 0 && strip < stripCount ? load(color[strip]) : 0.0f; }
float Mixer::getVoice(int strip) const { return strip >= 0 && strip < stripCount ? load(voice[strip]) : 0.0f; }
float Mixer::getEcho(int strip) const { return strip >= 0 && strip < stripCount ? load(echo[strip]) : 0.0f; }
float Mixer::getComp(int strip) const { return strip >= 0 && strip < stripCount ? load(comp[strip]) : 0.0f; }
float Mixer::getGate(int strip) const { return strip >= 0 && strip < stripCount ? load(gate[strip]) : 0.0f; }
float Mixer::getDenoise(int strip) const { return strip >= 0 && strip < stripCount ? load(denoise[strip]) : 0.0f; }
float Mixer::getBass(int strip) const { return strip >= 0 && strip < stripCount ? load(bass[strip]) : 0.0f; }
float Mixer::getTreble(int strip) const { return strip >= 0 && strip < stripCount ? load(treble[strip]) : 0.0f; }
float Mixer::getReverbSend(int strip) const { return strip >= 0 && strip < stripCount ? load(reverbSend[strip]) : 0.0f; }
float Mixer::getDelaySend(int strip) const { return strip >= 0 && strip < stripCount ? load(delaySend[strip]) : 0.0f; }
float Mixer::getStripMeter(int strip) const { return strip >= 0 && strip < stripCount ? load(stripMeter[strip]) : 0.0f; }

void Mixer::setBusFaderDb(int bus, float db)
{
    if (bus < 0 || bus >= busCount)
        return;

    db = std::clamp(db, minimumDb, maximumDb);
    busFaderDb[bus].store(db, std::memory_order_relaxed);
    busFaderGain[bus].store(dbToLinear(db <= minimumDb ? -100.0f : db), std::memory_order_relaxed);
}

float Mixer::getBusFaderDb(int bus) const { return bus >= 0 && bus < busCount ? load(busFaderDb[bus]) : minimumDb; }

void Mixer::setBusMuted(int bus, bool muted)
{
    if (bus >= 0 && bus < busCount)
        busMute[bus].store(muted, std::memory_order_relaxed);
}

void Mixer::setBusMono(int bus, bool mono)
{
    if (bus >= 0 && bus < busCount)
        busMono[bus].store(mono, std::memory_order_relaxed);
}

void Mixer::setBusEqEnabled(int bus, bool enabled)
{
    if (bus >= 0 && bus < busCount)
        busEq[bus].store(enabled, std::memory_order_relaxed);
}

void Mixer::setBusBass(int bus, float value)
{
    if (bus >= 0 && bus < busCount)
        busBass[bus].store(bipolar(value), std::memory_order_relaxed);
}

void Mixer::setBusTreble(int bus, float value)
{
    if (bus >= 0 && bus < busCount)
        busTreble[bus].store(bipolar(value), std::memory_order_relaxed);
}

bool Mixer::isBusMuted(int bus) const { return bus >= 0 && bus < busCount && load(busMute[bus]); }
bool Mixer::isBusMono(int bus) const { return bus >= 0 && bus < busCount && load(busMono[bus]); }
bool Mixer::isBusEqEnabled(int bus) const { return bus >= 0 && bus < busCount && load(busEq[bus]); }
float Mixer::getBusBass(int bus) const { return bus >= 0 && bus < busCount ? load(busBass[bus]) : 0.0f; }
float Mixer::getBusTreble(int bus) const { return bus >= 0 && bus < busCount ? load(busTreble[bus]) : 0.0f; }
float Mixer::getBusMeter(int bus) const { return bus >= 0 && bus < busCount ? load(busMeter[bus]) : 0.0f; }

void Mixer::setReverbReturn(float value) { reverbReturn.store(unit(value), std::memory_order_relaxed); }
void Mixer::setDelayReturn(float value) { delayReturn.store(unit(value), std::memory_order_relaxed); }
float Mixer::getReverbReturn() const { return load(reverbReturn); }
float Mixer::getDelayReturn() const { return load(delayReturn); }

void Mixer::setStripName(int strip, const juce::String& name)
{
    if (strip >= 0 && strip < stripCount)
        stripName[strip] = name;
}

juce::String Mixer::getStripName(int strip) const
{
    return strip >= 0 && strip < stripCount ? stripName[strip] : juce::String();
}

void Mixer::setInputPair(int strip, int pair)
{
    if (strip >= 0 && strip < stripCount)
        inputPair[strip].store(std::clamp(pair, 0, stripCount - 1), std::memory_order_relaxed);
}

int Mixer::getInputPair(int strip) const
{
    return strip >= 0 && strip < stripCount ? load(inputPair[strip]) : 0;
}

void Mixer::setPan(int strip, float value)
{
    if (strip >= 0 && strip < stripCount)
        pan[strip].store(bipolar(value), std::memory_order_relaxed);
}

float Mixer::getPan(int strip) const
{
    return strip >= 0 && strip < stripCount ? load(pan[strip]) : 0.0f;
}

void Mixer::setModulation(int strip, float depth)
{
    if (strip >= 0 && strip < stripCount)
        modulation[strip].store(unit(depth), std::memory_order_relaxed);
}

float Mixer::getModulation(int strip) const
{
    return strip >= 0 && strip < stripCount ? load(modulation[strip]) : 0.0f;
}

void Mixer::setEqBand(int strip, int channel, int band, EqBand value)
{
    if (strip < 0 || strip >= stripCount || channel < 0 || channel > 1 || band < 0 || band >= eqBands)
        return;

    eqBandOn[strip][channel][band].store(value.enabled, std::memory_order_relaxed);
    eqBandType[strip][channel][band].store(std::clamp(value.type, 0, 4), std::memory_order_relaxed);
    eqBandFreq[strip][channel][band].store(std::clamp(value.frequency, 20.0f, 20000.0f), std::memory_order_relaxed);
    eqBandGain[strip][channel][band].store(std::clamp(value.gainDb, -24.0f, 24.0f), std::memory_order_relaxed);
    eqBandQ[strip][channel][band].store(std::clamp(value.q, 0.1f, 20.0f), std::memory_order_relaxed);
}

Mixer::EqBand Mixer::getEqBand(int strip, int channel, int band) const
{
    EqBand value;
    if (strip < 0 || strip >= stripCount || channel < 0 || channel > 1 || band < 0 || band >= eqBands)
        return value;

    value.enabled = load(eqBandOn[strip][channel][band]);
    value.type = load(eqBandType[strip][channel][band]);
    value.frequency = load(eqBandFreq[strip][channel][band]);
    value.gainDb = load(eqBandGain[strip][channel][band]);
    value.q = load(eqBandQ[strip][channel][band]);
    return value;
}

void Mixer::setBusEqBand(int bus, int band, EqBand value)
{
    if (bus < 0 || bus >= busCount || band < 0 || band >= eqBands)
        return;

    busEqBandOn[bus][band].store(value.enabled, std::memory_order_relaxed);
    busEqBandType[bus][band].store(std::clamp(value.type, 0, 4), std::memory_order_relaxed);
    busEqBandFreq[bus][band].store(std::clamp(value.frequency, 20.0f, 20000.0f), std::memory_order_relaxed);
    busEqBandGain[bus][band].store(std::clamp(value.gainDb, -24.0f, 24.0f), std::memory_order_relaxed);
    busEqBandQ[bus][band].store(std::clamp(value.q, 0.1f, 20.0f), std::memory_order_relaxed);
}

Mixer::EqBand Mixer::getBusEqBand(int bus, int band) const
{
    EqBand value;
    if (bus < 0 || bus >= busCount || band < 0 || band >= eqBands)
        return value;

    value.enabled = load(busEqBandOn[bus][band]);
    value.type = load(busEqBandType[bus][band]);
    value.frequency = load(busEqBandFreq[bus][band]);
    value.gainDb = load(busEqBandGain[bus][band]);
    value.q = load(busEqBandQ[bus][band]);
    return value;
}

void Mixer::setBusLimiter(int bus, bool enabled)
{
    if (bus >= 0 && bus < busCount)
        busLimiter[bus].store(enabled, std::memory_order_relaxed);
}

bool Mixer::isBusLimiter(int bus) const
{
    return bus >= 0 && bus < busCount && load(busLimiter[bus]);
}

void Mixer::setCompSettings(int strip, CompSettings settings)
{
    if (strip < 0 || strip >= stripCount)
        return;

    compDetail[strip].store(settings.detailed, std::memory_order_relaxed);
    compInDb[strip].store(std::clamp(settings.inputDb, -24.0f, 24.0f), std::memory_order_relaxed);
    compThr[strip].store(std::clamp(settings.thresholdDb, -60.0f, 0.0f), std::memory_order_relaxed);
    compRatio[strip].store(std::clamp(settings.ratio, 1.0f, 20.0f), std::memory_order_relaxed);
    compAttack[strip].store(std::clamp(settings.attackMs, 0.1f, 200.0f), std::memory_order_relaxed);
    compRelease[strip].store(std::clamp(settings.releaseMs, 5.0f, 2000.0f), std::memory_order_relaxed);
    compKnee[strip].store(std::clamp(settings.knee, 0.0f, 24.0f), std::memory_order_relaxed);
    compOutDb[strip].store(std::clamp(settings.outputDb, -24.0f, 24.0f), std::memory_order_relaxed);
    compAuto[strip].store(settings.autoMakeup, std::memory_order_relaxed);
}

Mixer::CompSettings Mixer::getCompSettings(int strip) const
{
    CompSettings settings;
    if (strip < 0 || strip >= stripCount)
        return settings;

    settings.detailed = load(compDetail[strip]);
    settings.inputDb = load(compInDb[strip]);
    settings.thresholdDb = load(compThr[strip]);
    settings.ratio = load(compRatio[strip]);
    settings.attackMs = load(compAttack[strip]);
    settings.releaseMs = load(compRelease[strip]);
    settings.knee = load(compKnee[strip]);
    settings.outputDb = load(compOutDb[strip]);
    settings.autoMakeup = load(compAuto[strip]);
    return settings;
}

void Mixer::setGateSettings(int strip, GateSettings settings)
{
    if (strip < 0 || strip >= stripCount)
        return;

    gateDetail[strip].store(settings.detailed, std::memory_order_relaxed);
    gateThr[strip].store(std::clamp(settings.thresholdDb, -80.0f, 0.0f), std::memory_order_relaxed);
    gateRange[strip].store(std::clamp(settings.rangeDb, -80.0f, 0.0f), std::memory_order_relaxed);
    gateAttack[strip].store(std::clamp(settings.attackMs, 0.1f, 200.0f), std::memory_order_relaxed);
    gateHold[strip].store(std::clamp(settings.holdMs, 0.0f, 2000.0f), std::memory_order_relaxed);
    gateRelease[strip].store(std::clamp(settings.releaseMs, 5.0f, 2000.0f), std::memory_order_relaxed);
}

Mixer::GateSettings Mixer::getGateSettings(int strip) const
{
    GateSettings settings;
    if (strip < 0 || strip >= stripCount)
        return settings;

    settings.detailed = load(gateDetail[strip]);
    settings.thresholdDb = load(gateThr[strip]);
    settings.rangeDb = load(gateRange[strip]);
    settings.attackMs = load(gateAttack[strip]);
    settings.holdMs = load(gateHold[strip]);
    settings.releaseMs = load(gateRelease[strip]);
    return settings;
}

void Mixer::setReverbDecay(float seconds) { reverbDecay.store(std::clamp(seconds, 0.2f, 8.0f), std::memory_order_relaxed); }
void Mixer::setReverbPredelay(float seconds) { reverbPredelay.store(std::clamp(seconds, 0.0f, 0.1f), std::memory_order_relaxed); }
void Mixer::setReverbEarly(float amount) { reverbEarly.store(unit(amount), std::memory_order_relaxed); }
float Mixer::getReverbDecay() const { return load(reverbDecay); }
float Mixer::getReverbPredelay() const { return load(reverbPredelay); }
float Mixer::getReverbEarly() const { return load(reverbEarly); }
void Mixer::setDelayTime(float seconds) { delayTime.store(std::clamp(seconds, 0.01f, 2.0f), std::memory_order_relaxed); }
void Mixer::setDelayFeedback(float amount) { delayFeedback.store(std::clamp(amount, 0.0f, 0.95f), std::memory_order_relaxed); }
float Mixer::getDelayTime() const { return load(delayTime); }
float Mixer::getDelayFeedback() const { return load(delayFeedback); }

void Mixer::setDelayTap(int tapIndex, bool enabled, float seconds, float gain)
{
    if (tapIndex < 0 || tapIndex >= delayTaps)
        return;

    delayTapOn[tapIndex].store(enabled, std::memory_order_relaxed);
    delayTapTime[tapIndex].store(std::clamp(seconds, 0.01f, 2.0f), std::memory_order_relaxed);
    delayTapGain[tapIndex].store(std::clamp(gain, 0.0f, 1.0f), std::memory_order_relaxed);
}

bool Mixer::isDelayTapOn(int tapIndex) const
{
    return tapIndex >= 0 && tapIndex < delayTaps && load(delayTapOn[tapIndex]);
}

float Mixer::getDelayTapTime(int tapIndex) const
{
    return tapIndex >= 0 && tapIndex < delayTaps ? load(delayTapTime[tapIndex]) : 0.0f;
}

float Mixer::getDelayTapGain(int tapIndex) const
{
    return tapIndex >= 0 && tapIndex < delayTaps ? load(delayTapGain[tapIndex]) : 0.0f;
}

bool Mixer::anySolo() const noexcept
{
    for (int strip = 0; strip < stripCount; ++strip)
        if (load(stripSolo[strip]))
            return true;

    return false;
}

void Mixer::process(const juce::AudioBuffer<float>& deviceInput,
                    const juce::AudioBuffer<float>* playback,
                    int playbackStrip,
                    juce::AudioBuffer<float>& hardwareOutput,
                    int recordBus,
                    juce::AudioBuffer<float>* record) noexcept
{
    const int numSamples = hardwareOutput.getNumSamples();
    if (numSamples <= 0 || echoLine[0][0].empty())
        return;

    const bool soloed = anySolo();
    const float reverbMix = load(reverbReturn);
    const float delayMix = load(delayReturn);
    const int echoDelay = std::max(1, (int) (sampleRate * 0.08));
    const int mainDelaySamples = std::max(1, (int) (load(delayTime) * (float) sampleRate));
    const float mainFeedback = load(delayFeedback);
    const float reverbFeedback = std::clamp(0.2f + 0.42f * load(reverbDecay), 0.2f, 0.92f);
    const int predelaySamples = std::max(0, (int) (load(reverbPredelay) * (float) sampleRate));
    const float earlyAmount = load(reverbEarly);
    const float lowCoeff = 1.0f - std::exp(-2.0f * 3.14159265f * 180.0f / (float) sampleRate);

    for (int strip = 0; strip < stripCount; ++strip)
        for (int channel = 0; channel < 2; ++channel)
            for (int band = 0; band < eqBands; ++band)
                if (load(eqBandOn[strip][channel][band]))
                    stripFilter[strip][channel][band].update(load(eqBandType[strip][channel][band]), sampleRate,
                                                            load(eqBandFreq[strip][channel][band]),
                                                            load(eqBandGain[strip][channel][band]),
                                                            load(eqBandQ[strip][channel][band]));

    for (int bus = 0; bus < busCount; ++bus)
        for (int channel = 0; channel < 2; ++channel)
            for (int band = 0; band < eqBands; ++band)
                if (load(busEqBandOn[bus][band]))
                    busFilter[bus][channel][band].update(load(busEqBandType[bus][band]), sampleRate,
                                                        load(busEqBandFreq[bus][band]),
                                                        load(busEqBandGain[bus][band]),
                                                        load(busEqBandQ[bus][band]));
    float stripPeak[stripCount] {};
    float busPeak[busCount] {};

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float busL[busCount] {};
        float busR[busCount] {};
        float reverbInL = 0.0f;
        float reverbInR = 0.0f;
        float delayInL = 0.0f;
        float delayInR = 0.0f;

        for (int strip = 0; strip < stripCount; ++strip)
        {
            float left = 0.0f;
            float right = 0.0f;
            const int inL = std::clamp(load(inputPair[strip]), 0, stripCount - 1) * 2;
            const int inR = inL + 1;

            if (inL < deviceInput.getNumChannels() && sample < deviceInput.getNumSamples())
                left = deviceInput.getSample(inL, sample);
            if (inR < deviceInput.getNumChannels() && sample < deviceInput.getNumSamples())
                right = deviceInput.getSample(inR, sample);
            else
                right = left;

            if (playback != nullptr && strip == playbackStrip && sample < playback->getNumSamples())
            {
                if (playback->getNumChannels() > 0)
                    left += playback->getSample(0, sample);
                if (playback->getNumChannels() > 1)
                    right += playback->getSample(1, sample);
                else if (playback->getNumChannels() > 0)
                    right += playback->getSample(0, sample);
            }

            if (load(stripMono[strip]))
            {
                const float mid = 0.5f * (left + right);
                left = right = mid;
            }

            const float level = std::max(std::abs(left), std::abs(right));
            envelope[strip] = std::max(level, envelope[strip] * 0.9995f);

            float gain = 1.0f;
            const float compAmount = load(comp[strip]);
            if (compAmount > 0.001f && load(compDetail[strip]))
            {
                const float input = dbToLinear(load(compInDb[strip]));
                const float target = level * input;
                const float coeff = target > compEnv[strip] ? msCoeff(load(compAttack[strip]), sampleRate)
                                                            : msCoeff(load(compRelease[strip]), sampleRate);
                compEnv[strip] += coeff * (target - compEnv[strip]);
                const float delta = linearToDb(compEnv[strip]) - load(compThr[strip]);
                const float ratio = std::max(1.0f, load(compRatio[strip]));
                const float knee = load(compKnee[strip]);
                float reduction = 0.0f;
                if (knee > 0.1f && delta > -knee * 0.5f && delta < knee * 0.5f)
                    reduction = (delta + knee * 0.5f) * (delta + knee * 0.5f) / (2.0f * knee) * (1.0f - 1.0f / ratio);
                else if (delta > 0.0f)
                    reduction = delta * (1.0f - 1.0f / ratio);

                float out = dbToLinear(-reduction + load(compOutDb[strip]));
                if (load(compAuto[strip]))
                    out *= dbToLinear(std::max(0.0f, -load(compThr[strip]) * (1.0f - 1.0f / ratio)) * 0.5f);

                gain *= 1.0f + (out - 1.0f) * compAmount;
            }
            else if (compAmount > 0.001f)
            {
                const float threshold = dbToLinear(-12.0f - compAmount * 24.0f);
                const float over = envelope[strip] / std::max(threshold, 1.0e-6f);
                if (over > 1.0f)
                    gain /= std::pow(over, 0.75f);
            }

            const float gateAmount = load(gate[strip]);
            float targetGate = 1.0f;
            if (gateAmount > 0.001f && load(gateDetail[strip]))
            {
                const float threshold = dbToLinear(load(gateThr[strip]));
                const int holdSamples = (int) (load(gateHold[strip]) * 0.001f * (float) sampleRate);
                if (envelope[strip] >= threshold)
                    gateHoldLeft[strip] = holdSamples;
                else if (gateHoldLeft[strip] > 0)
                    --gateHoldLeft[strip];
                else
                    targetGate = dbToLinear(load(gateRange[strip]));

                const float smooth = envelope[strip] >= threshold ? msCoeff(load(gateAttack[strip]), sampleRate)
                                                                  : msCoeff(load(gateRelease[strip]), sampleRate);
                gateSmooth[strip] += smooth * (targetGate - gateSmooth[strip]);
            }
            else
            {
                if (gateAmount > 0.001f)
                {
                    const float threshold = dbToLinear(-48.0f + gateAmount * 28.0f);
                    if (envelope[strip] < threshold)
                        targetGate = 0.0f;
                }

                gateSmooth[strip] += 0.02f * (targetGate - gateSmooth[strip]);
            }

            const float denoiseAmount = load(denoise[strip]);
            if (denoiseAmount > 0.001f)
            {
                const float threshold = dbToLinear(-56.0f + denoiseAmount * 36.0f);
                if (envelope[strip] < threshold)
                    gain *= 0.08f;
            }

            gain *= gateSmooth[strip];
            left *= gain;
            right *= gain;

            const float tone = load(color[strip]);
            const float presence = load(voice[strip]);
            for (int channel = 0; channel < 2; ++channel)
            {
                float& sampleRef = channel == 0 ? left : right;
                float& state = lowState[strip][channel];
                state += lowCoeff * (sampleRef - state);
                const float high = sampleRef - state;
                sampleRef = sampleRef + tone * (tone >= 0.0f ? high : -state);
                sampleRef += presence * high * 1.8f;
            }

            if (load(eqEnabled[strip]))
            {
                for (int band = 0; band < eqBands; ++band)
                {
                    if (load(eqBandOn[strip][0][band]))
                        left = stripFilter[strip][0][band].process(left);
                    if (load(eqBandOn[strip][1][band]))
                        right = stripFilter[strip][1][band].process(right);
                }

                const float bassAmount = load(bass[strip]);
                const float trebleAmount = load(treble[strip]);
                left += bassAmount * lowState[strip][0] + trebleAmount * (left - lowState[strip][0]);
                right += bassAmount * lowState[strip][1] + trebleAmount * (right - lowState[strip][1]);
            }

            const float panAmount = load(pan[strip]);
            if (panAmount < 0.0f)
                right *= 1.0f + panAmount;
            else if (panAmount > 0.0f)
                left *= 1.0f - panAmount;

            const float depth = load(modulation[strip]);
            if (depth > 0.001f)
            {
                modPhase[strip] += 6.2831853f * 3.0f / (float) sampleRate;
                const float mod = 1.0f - depth * 0.5f * (1.0f - std::sin(modPhase[strip]));
                left *= mod;
                right *= mod;
            }

            const float preL = left;
            const float preR = right;
            const float fader = load(faderGain[strip]);
            left *= fader;
            right *= fader;

            const float echoAmount = load(echo[strip]);
            if (echoAmount > 0.001f)
            {
                left += echoAmount * tap(echoLine[strip][0], echoPos[strip][0], left, echoDelay, 0.25f);
                right += echoAmount * tap(echoLine[strip][1], echoPos[strip][1], right, echoDelay, 0.25f);
            }

            const bool audible = ! load(stripMute[strip]) && (! soloed || load(stripSolo[strip]));
            if (! audible)
            {
                left = 0.0f;
                right = 0.0f;
            }

            stripPeak[strip] = std::max(stripPeak[strip], std::max(std::abs(left), std::abs(right)));

            const float sendL = audible ? (load(postFx[strip]) ? left : preL) : 0.0f;
            const float sendR = audible ? (load(postFx[strip]) ? right : preR) : 0.0f;
            const float rev = load(reverbSend[strip]);
            const float del = load(delaySend[strip]);
            reverbInL += sendL * rev;
            reverbInR += sendR * rev;
            delayInL += sendL * del;
            delayInR += sendR * del;

            if (audible)
            {
                for (int bus = 0; bus < busCount; ++bus)
                {
                    if (! load(assign[strip][bus]))
                        continue;

                    busL[bus] += left;
                    busR[bus] += right;
                }
            }
        }

        float feedL = reverbInL;
        float feedR = reverbInR;
        float earlyL = 0.0f;
        float earlyR = 0.0f;
        if (predelaySamples > 0 && ! predelayLine[0].empty())
        {
            feedL = readLine(predelayLine[0], predelayPos[0], predelaySamples);
            feedR = readLine(predelayLine[1], predelayPos[1], predelaySamples);
            earlyL = readLine(predelayLine[0], predelayPos[0], std::max(1, predelaySamples / 2)) * earlyAmount;
            earlyR = readLine(predelayLine[1], predelayPos[1], std::max(1, predelaySamples / 2)) * earlyAmount;
            predelayLine[0][(size_t) predelayPos[0]] = reverbInL;
            predelayLine[1][(size_t) predelayPos[1]] = reverbInR;
            if (++predelayPos[0] >= (int) predelayLine[0].size()) predelayPos[0] = 0;
            if (++predelayPos[1] >= (int) predelayLine[1].size()) predelayPos[1] = 0;
        }

        float reverbL = earlyL;
        float reverbR = earlyR;
        for (int tapIndex = 0; tapIndex < 4; ++tapIndex)
        {
            reverbL += tap(reverbLine[tapIndex][0], reverbPos[tapIndex][0], feedL, (int) reverbLine[tapIndex][0].size() / 2, reverbFeedback);
            reverbR += tap(reverbLine[tapIndex][1], reverbPos[tapIndex][1], feedR, (int) reverbLine[tapIndex][1].size() / 2, reverbFeedback);
        }
        reverbL *= 0.25f;
        reverbR *= 0.25f;

        float delayL = tap(delayLine[0], delayPos[0], delayInL, mainDelaySamples, mainFeedback);
        float delayR = tap(delayLine[1], delayPos[1], delayInR, mainDelaySamples, mainFeedback);
        if (! load(delayTapOn[0]))
        {
            delayL = 0.0f;
            delayR = 0.0f;
        }

        for (int tapIndex = 1; tapIndex < delayTaps; ++tapIndex)
        {
            if (! load(delayTapOn[tapIndex]))
                continue;

            const int tapSamples = std::max(1, (int) (load(delayTapTime[tapIndex]) * (float) sampleRate));
            const float tapGain = load(delayTapGain[tapIndex]);
            delayL += readLine(delayLine[0], delayPos[0], tapSamples) * tapGain;
            delayR += readLine(delayLine[1], delayPos[1], tapSamples) * tapGain;
        }

        for (int bus = 0; bus < busCount; ++bus)
        {
            float left = busL[bus] + reverbL * reverbMix + delayL * delayMix;
            float right = busR[bus] + reverbR * reverbMix + delayR * delayMix;

            busLow[bus][0] += lowCoeff * (left - busLow[bus][0]);
            busLow[bus][1] += lowCoeff * (right - busLow[bus][1]);

            if (load(busEq[bus]))
            {
                for (int band = 0; band < eqBands; ++band)
                {
                    if (load(busEqBandOn[bus][band]))
                    {
                        left = busFilter[bus][0][band].process(left);
                        right = busFilter[bus][1][band].process(right);
                    }
                }

                left += load(busBass[bus]) * busLow[bus][0] + load(busTreble[bus]) * (left - busLow[bus][0]);
                right += load(busBass[bus]) * busLow[bus][1] + load(busTreble[bus]) * (right - busLow[bus][1]);
            }

            if (load(busLimiter[bus]))
            {
                const float peak = std::max(std::abs(left), std::abs(right));
                if (peak > 1.0f)
                {
                    left /= peak;
                    right /= peak;
                }
            }

            if (load(busMono[bus]))
            {
                const float mid = 0.5f * (left + right);
                left = right = mid;
            }

            const float fader = load(busMute[bus]) ? 0.0f : load(busFaderGain[bus]);
            left *= fader;
            right *= fader;
            busPeak[bus] = std::max(busPeak[bus], std::max(std::abs(left), std::abs(right)));

            if (bus == 0 && hardwareOutput.getNumChannels() > 0)
            {
                hardwareOutput.setSample(0, sample, left);
                if (hardwareOutput.getNumChannels() > 1)
                    hardwareOutput.setSample(1, sample, right);
            }

            if (record != nullptr && bus == recordBus && sample < record->getNumSamples())
            {
                if (record->getNumChannels() > 0)
                    record->setSample(0, sample, left);
                if (record->getNumChannels() > 1)
                    record->setSample(1, sample, right);
            }
        }
    }

    for (int strip = 0; strip < stripCount; ++strip)
    {
        const float previous = load(stripMeter[strip]);
        stripMeter[strip].store(std::max(stripPeak[strip], previous * 0.85f), std::memory_order_relaxed);
    }

    for (int bus = 0; bus < busCount; ++bus)
    {
        const float previous = load(busMeter[bus]);
        busMeter[bus].store(std::max(busPeak[bus], previous * 0.85f), std::memory_order_relaxed);
    }
}

} // namespace xenhusk
