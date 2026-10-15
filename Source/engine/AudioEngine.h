#pragma once

#include "Mixer.h"
#include "MmcssTask.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>

namespace xenhusk
{

class AudioEngine : public juce::AudioIODeviceCallback
{
public:
    AudioEngine();
    ~AudioEngine() override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;

    Mixer& getMixer() noexcept { return mixer; }
    const Mixer& getMixer() const noexcept { return mixer; }

    juce::String getDeviceName() const;
    double getSampleRate() const noexcept;
    int getBufferSize() const noexcept;
    bool isProAudioActive() const noexcept;

    bool loadPlaybackFile(const juce::File& file);
    void setPlaybackPlaying(bool shouldPlay);
    void stopPlayback();
    bool isPlaybackPlaying() const noexcept;
    juce::String getPlaybackName() const;
    void setPlaybackStrip(int strip);
    int getPlaybackStrip() const noexcept;
    double getPlaybackSeconds() const noexcept;

    void setRecordBus(int bus);
    int getRecordBus() const noexcept;
    bool startRecording();
    void stopRecording();
    bool isRecording() const noexcept;
    double getRecordSeconds() const noexcept;
    juce::File getRecordingFile() const;

private:
    Mixer mixer;
    MmcssTask mmcss;
    juce::AudioBuffer<float> inputPlanes;
    juce::AudioBuffer<float> playbackBuffer;
    juce::AudioBuffer<float> recordBuffer;
    juce::AudioBuffer<float> hardwareBuffer;

    juce::AudioFormatManager formatManager;
    juce::TimeSliceThread writerThread { "Xenhusk Recorder" };
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter;
    juce::CriticalSection readerLock;
    std::unique_ptr<juce::AudioFormatReader> reader;
    std::atomic<bool> playing { false };
    std::atomic<int64_t> playPosition { 0 };
    std::atomic<int> playbackStrip { 5 };
    std::atomic<int> recordBus { 0 };
    std::atomic<bool> recording { false };
    std::atomic<int64_t> recordedSamples { 0 };
    juce::File recordingFile;
    juce::String playbackName;

    mutable juce::CriticalSection statusLock;
    juce::String deviceName;
    std::atomic<double> sampleRate { 48000.0 };
    std::atomic<int> bufferSize { 0 };
};

} // namespace xenhusk
