#pragma once

#include "Biquad.h"
#include "Gain.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <atomic>
#include <vector>

namespace xenhusk
{

class Mixer
{
public:
    static constexpr int stripCount = 8;
    static constexpr int busCount = 8;
    static constexpr int eqBands = 6;
    static constexpr int delayTaps = 4;
    static constexpr float minimumDb = -60.0f;
    static constexpr float maximumDb = 12.0f;

    struct EqBand
    {
        bool enabled = true;
        int type = 0;
        float frequency = 1000.0f;
        float gainDb = 0.0f;
        float q = 1.0f;
    };

    struct CompSettings
    {
        bool detailed = false;
        float inputDb = 0.0f;
        float thresholdDb = -18.0f;
        float ratio = 1.0f;
        float attackMs = 10.0f;
        float releaseMs = 50.0f;
        float knee = 6.0f;
        float outputDb = 0.0f;
        bool autoMakeup = false;
    };

    struct GateSettings
    {
        bool detailed = false;
        float thresholdDb = -40.0f;
        float rangeDb = -60.0f;
        float attackMs = 10.0f;
        float holdMs = 200.0f;
        float releaseMs = 200.0f;
    };

    Mixer();

    void prepare(double sampleRate);
    void resetToDefault();

    void setFaderDb(int strip, float db);
    float getFaderDb(int strip) const;
    void setStripMuted(int strip, bool muted);
    void setStripSolo(int strip, bool solo);
    void setStripMono(int strip, bool mono);
    void setEqEnabled(int strip, bool enabled);
    void setPostFx(int strip, bool post);
    void setAssigned(int strip, int bus, bool assigned);
    void setColor(int strip, float value);
    void setVoice(int strip, float value);
    void setEcho(int strip, float value);
    void setComp(int strip, float value);
    void setGate(int strip, float value);
    void setDenoise(int strip, float value);
    void setBass(int strip, float value);
    void setTreble(int strip, float value);
    void setReverbSend(int strip, float value);
    void setDelaySend(int strip, float value);

    bool isStripMuted(int strip) const;
    bool isStripSolo(int strip) const;
    bool isStripMono(int strip) const;
    bool isEqEnabled(int strip) const;
    bool isPostFx(int strip) const;
    bool isAssigned(int strip, int bus) const;
    float getColor(int strip) const;
    float getVoice(int strip) const;
    float getEcho(int strip) const;
    float getComp(int strip) const;
    float getGate(int strip) const;
    float getDenoise(int strip) const;
    float getBass(int strip) const;
    float getTreble(int strip) const;
    float getReverbSend(int strip) const;
    float getDelaySend(int strip) const;
    float getStripMeter(int strip) const;

    void setBusFaderDb(int bus, float db);
    float getBusFaderDb(int bus) const;
    void setBusMuted(int bus, bool muted);
    void setBusMono(int bus, bool mono);
    void setBusEqEnabled(int bus, bool enabled);
    void setBusBass(int bus, float value);
    void setBusTreble(int bus, float value);
    bool isBusMuted(int bus) const;
    bool isBusMono(int bus) const;
    bool isBusEqEnabled(int bus) const;
    float getBusBass(int bus) const;
    float getBusTreble(int bus) const;
    float getBusMeter(int bus) const;

    void setReverbReturn(float value);
    void setDelayReturn(float value);
    float getReverbReturn() const;
    float getDelayReturn() const;

    void setStripName(int strip, const juce::String& name);
    juce::String getStripName(int strip) const;
    void setInputPair(int strip, int pair);
    int getInputPair(int strip) const;
    void setPan(int strip, float pan);
    float getPan(int strip) const;
    void setModulation(int strip, float depth);
    float getModulation(int strip) const;

    void setEqBand(int strip, int channel, int band, EqBand value);
    EqBand getEqBand(int strip, int channel, int band) const;
    void setBusEqBand(int bus, int band, EqBand value);
    EqBand getBusEqBand(int bus, int band) const;
    void setBusLimiter(int bus, bool enabled);
    bool isBusLimiter(int bus) const;

    void setCompSettings(int strip, CompSettings settings);
    CompSettings getCompSettings(int strip) const;
    void setGateSettings(int strip, GateSettings settings);
    GateSettings getGateSettings(int strip) const;

    void setReverbDecay(float seconds);
    void setReverbPredelay(float seconds);
    void setReverbEarly(float amount);
    float getReverbDecay() const;
    float getReverbPredelay() const;
    float getReverbEarly() const;
    void setDelayTime(float seconds);
    void setDelayFeedback(float amount);
    float getDelayTime() const;
    float getDelayFeedback() const;
    void setDelayTap(int tap, bool enabled, float seconds, float gain);
    bool isDelayTapOn(int tap) const;
    float getDelayTapTime(int tap) const;
    float getDelayTapGain(int tap) const;

    void process(const juce::AudioBuffer<float>& deviceInput,
                 const juce::AudioBuffer<float>* playback,
                 int playbackStrip,
                 juce::AudioBuffer<float>& hardwareOutput,
                 int recordBus,
                 juce::AudioBuffer<float>* record) noexcept;

private:
    template <typename T>
    static T load(const std::atomic<T>& value) noexcept
    {
        return value.load(std::memory_order_relaxed);
    }

    static float unit(float value) noexcept;
    static float bipolar(float value) noexcept;
    bool anySolo() const noexcept;

    std::atomic<float> faderDb[stripCount];
    std::atomic<float> faderGain[stripCount];
    std::atomic<bool> stripMute[stripCount];
    std::atomic<bool> stripSolo[stripCount];
    std::atomic<bool> stripMono[stripCount];
    std::atomic<bool> eqEnabled[stripCount];
    std::atomic<bool> postFx[stripCount];
    std::atomic<bool> assign[stripCount][busCount];
    std::atomic<float> color[stripCount];
    std::atomic<float> voice[stripCount];
    std::atomic<float> echo[stripCount];
    std::atomic<float> comp[stripCount];
    std::atomic<float> gate[stripCount];
    std::atomic<float> denoise[stripCount];
    std::atomic<float> bass[stripCount];
    std::atomic<float> treble[stripCount];
    std::atomic<float> reverbSend[stripCount];
    std::atomic<float> delaySend[stripCount];
    std::atomic<float> stripMeter[stripCount];

    std::atomic<float> busFaderDb[busCount];
    std::atomic<float> busFaderGain[busCount];
    std::atomic<bool> busMute[busCount];
    std::atomic<bool> busMono[busCount];
    std::atomic<bool> busEq[busCount];
    std::atomic<float> busBass[busCount];
    std::atomic<float> busTreble[busCount];
    std::atomic<float> busMeter[busCount];
    std::atomic<float> reverbReturn { 0.0f };
    std::atomic<float> delayReturn { 0.0f };
    std::atomic<float> reverbDecay { 1.0f };
    std::atomic<float> reverbPredelay { 0.0f };
    std::atomic<float> reverbEarly { 0.0f };
    std::atomic<float> delayTime { 0.18f };
    std::atomic<float> delayFeedback { 0.35f };
    std::atomic<bool> delayTapOn[delayTaps];
    std::atomic<float> delayTapTime[delayTaps];
    std::atomic<float> delayTapGain[delayTaps];

    std::atomic<bool> eqBandOn[stripCount][2][eqBands];
    std::atomic<int> eqBandType[stripCount][2][eqBands];
    std::atomic<float> eqBandFreq[stripCount][2][eqBands];
    std::atomic<float> eqBandGain[stripCount][2][eqBands];
    std::atomic<float> eqBandQ[stripCount][2][eqBands];
    std::atomic<bool> busEqBandOn[busCount][eqBands];
    std::atomic<int> busEqBandType[busCount][eqBands];
    std::atomic<float> busEqBandFreq[busCount][eqBands];
    std::atomic<float> busEqBandGain[busCount][eqBands];
    std::atomic<float> busEqBandQ[busCount][eqBands];
    std::atomic<bool> busLimiter[busCount];

    std::atomic<bool> compDetail[stripCount];
    std::atomic<float> compInDb[stripCount];
    std::atomic<float> compThr[stripCount];
    std::atomic<float> compRatio[stripCount];
    std::atomic<float> compAttack[stripCount];
    std::atomic<float> compRelease[stripCount];
    std::atomic<float> compKnee[stripCount];
    std::atomic<float> compOutDb[stripCount];
    std::atomic<bool> compAuto[stripCount];
    std::atomic<bool> gateDetail[stripCount];
    std::atomic<float> gateThr[stripCount];
    std::atomic<float> gateRange[stripCount];
    std::atomic<float> gateAttack[stripCount];
    std::atomic<float> gateHold[stripCount];
    std::atomic<float> gateRelease[stripCount];
    std::atomic<float> pan[stripCount];
    std::atomic<float> modulation[stripCount];
    std::atomic<int> inputPair[stripCount];

    juce::String stripName[stripCount];

    double sampleRate = 48000.0;
    Biquad stripFilter[stripCount][2][eqBands];
    Biquad busFilter[busCount][2][eqBands];
    float compEnv[stripCount] {};
    int gateHoldLeft[stripCount] {};
    float modPhase[stripCount] {};
    std::vector<float> predelayLine[2];
    int predelayPos[2] {};
    std::vector<float> echoLine[stripCount][2];
    std::vector<float> delayLine[2];
    std::vector<float> reverbLine[4][2];
    int echoPos[stripCount][2] {};
    int delayPos[2] {};
    int reverbPos[4][2] {};
    float envelope[stripCount] {};
    float gateSmooth[stripCount] {};
    float lowState[stripCount][2] {};
    float busLow[busCount][2] {};
};

} // namespace xenhusk
