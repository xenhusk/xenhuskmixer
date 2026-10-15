#include "AudioEngine.h"

namespace xenhusk
{

AudioEngine::AudioEngine()
{
    formatManager.registerBasicFormats();
    writerThread.startThread();
    mixer.prepare(48000.0);
}

AudioEngine::~AudioEngine()
{
    stopRecording();
    stopPlayback();
    writerThread.stopThread(1000);
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    const int samples = device != nullptr ? juce::jmax(device->getCurrentBufferSizeSamples(), 1) : 512;
    const double rate = device != nullptr ? device->getCurrentSampleRate() : 48000.0;
    inputPlanes.setSize(Mixer::stripCount * 2, samples, false, true, true);
    playbackBuffer.setSize(2, samples, false, true, true);
    recordBuffer.setSize(2, samples, false, true, true);
    hardwareBuffer.setSize(2, samples, false, true, true);
    mixer.prepare(rate);

    {
        const juce::ScopedLock lock(statusLock);
        deviceName = device != nullptr ? device->getName() : juce::String();
    }

    sampleRate.store(rate, std::memory_order_relaxed);
    bufferSize.store(samples, std::memory_order_relaxed);
    mmcss.activate();
}

void AudioEngine::audioDeviceStopped()
{
    stopRecording();
    mmcss.revert();
    bufferSize.store(0, std::memory_order_relaxed);

    const juce::ScopedLock lock(statusLock);
    deviceName.clear();
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                    int numInputChannels,
                                                    float* const* outputChannelData,
                                                    int numOutputChannels,
                                                    int numSamples,
                                                    const juce::AudioIODeviceCallbackContext& context)
{
    juce::ignoreUnused(context);
    juce::ScopedNoDenormals noDenormals;

    if (! mmcss.isActive())
        mmcss.activate();

    if (numSamples > inputPlanes.getNumSamples())
    {
        inputPlanes.setSize(Mixer::stripCount * 2, numSamples, false, false, true);
        playbackBuffer.setSize(2, numSamples, false, false, true);
        recordBuffer.setSize(2, numSamples, false, false, true);
        hardwareBuffer.setSize(2, numSamples, false, false, true);
    }

    inputPlanes.clear();
    const int samples = juce::jmin(numSamples, inputPlanes.getNumSamples());
    const int channels = juce::jmin(numInputChannels, inputPlanes.getNumChannels());

    for (int channel = 0; channel < channels; ++channel)
        if (inputChannelData != nullptr && inputChannelData[channel] != nullptr)
            inputPlanes.copyFrom(channel, 0, inputChannelData[channel], samples);

    playbackBuffer.clear();
    if (playing.load(std::memory_order_relaxed) && readerLock.tryEnter())
    {
        if (reader != nullptr)
        {
            const int64_t position = playPosition.load(std::memory_order_relaxed);
            if (position < reader->lengthInSamples)
            {
                reader->read(&playbackBuffer, 0, samples, position, true, true);
                playPosition.store(position + samples, std::memory_order_relaxed);
            }
            else
            {
                playing.store(false, std::memory_order_relaxed);
            }
        }

        readerLock.exit();
    }

    hardwareBuffer.clear();
    recordBuffer.clear();
    const bool keepRecording = recording.load(std::memory_order_relaxed) && threadedWriter != nullptr;
    mixer.process(inputPlanes,
                  playing.load(std::memory_order_relaxed) ? &playbackBuffer : nullptr,
                  playbackStrip.load(std::memory_order_relaxed),
                  hardwareBuffer,
                  recordBus.load(std::memory_order_relaxed),
                  keepRecording ? &recordBuffer : nullptr);

    if (outputChannelData != nullptr)
    {
        const int outputs = juce::jmin(numOutputChannels, hardwareBuffer.getNumChannels());
        for (int channel = 0; channel < numOutputChannels; ++channel)
        {
            if (outputChannelData[channel] == nullptr)
                continue;

            if (channel < outputs)
                juce::FloatVectorOperations::copy(outputChannelData[channel], hardwareBuffer.getReadPointer(channel), samples);
            else
                juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);
        }
    }

    if (keepRecording)
    {
        const float* channelsOut[2] = { recordBuffer.getReadPointer(0), recordBuffer.getReadPointer(1) };
        threadedWriter->write(channelsOut, samples);
        recordedSamples.fetch_add(samples, std::memory_order_relaxed);
    }
}

juce::String AudioEngine::getDeviceName() const
{
    const juce::ScopedLock lock(statusLock);
    return deviceName;
}

double AudioEngine::getSampleRate() const noexcept { return sampleRate.load(std::memory_order_relaxed); }
int AudioEngine::getBufferSize() const noexcept { return bufferSize.load(std::memory_order_relaxed); }
bool AudioEngine::isProAudioActive() const noexcept { return mmcss.isActive(); }

bool AudioEngine::loadPlaybackFile(const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> next(formatManager.createReaderFor(file));
    if (next == nullptr)
        return false;

    const juce::ScopedLock lock(readerLock);
    playing.store(false, std::memory_order_relaxed);
    reader = std::move(next);
    playPosition.store(0, std::memory_order_relaxed);
    playbackName = file.getFileName();
    return true;
}

void AudioEngine::setPlaybackPlaying(bool shouldPlay)
{
    if (shouldPlay && reader == nullptr)
        return;

    if (shouldPlay && playPosition.load(std::memory_order_relaxed) >= (reader != nullptr ? reader->lengthInSamples : 0))
        playPosition.store(0, std::memory_order_relaxed);

    playing.store(shouldPlay, std::memory_order_relaxed);
}

void AudioEngine::stopPlayback()
{
    playing.store(false, std::memory_order_relaxed);
    playPosition.store(0, std::memory_order_relaxed);
}

bool AudioEngine::isPlaybackPlaying() const noexcept { return playing.load(std::memory_order_relaxed); }
juce::String AudioEngine::getPlaybackName() const { return playbackName; }
void AudioEngine::setPlaybackStrip(int strip) { playbackStrip.store(juce::jlimit(0, Mixer::stripCount - 1, strip), std::memory_order_relaxed); }
int AudioEngine::getPlaybackStrip() const noexcept { return playbackStrip.load(std::memory_order_relaxed); }

double AudioEngine::getPlaybackSeconds() const noexcept
{
    const double rate = sampleRate.load(std::memory_order_relaxed);
    return rate > 0.0 ? (double) playPosition.load(std::memory_order_relaxed) / rate : 0.0;
}

void AudioEngine::setRecordBus(int bus) { recordBus.store(juce::jlimit(0, Mixer::busCount - 1, bus), std::memory_order_relaxed); }
int AudioEngine::getRecordBus() const noexcept { return recordBus.load(std::memory_order_relaxed); }

bool AudioEngine::startRecording()
{
    stopRecording();
    const double rate = sampleRate.load(std::memory_order_relaxed);
    if (rate <= 0.0)
        return false;

    auto folder = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Xenhusk Mixer");
    folder.createDirectory();
    recordingFile = folder.getChildFile("recording-" + juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S") + ".wav");

    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> stream(recordingFile.createOutputStream());
    if (stream == nullptr)
        return false;

    const auto options = juce::AudioFormatWriterOptions{}.withSampleRate(rate)
                                                         .withNumChannels(2)
                                                         .withBitsPerSample(16);
    auto writer = wav.createWriterFor(stream, options);
    if (writer == nullptr)
        return false;

    threadedWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(writer.release(), writerThread, 32768);
    recordedSamples.store(0, std::memory_order_relaxed);
    recording.store(true, std::memory_order_relaxed);
    return true;
}

void AudioEngine::stopRecording()
{
    recording.store(false, std::memory_order_relaxed);
    threadedWriter.reset();
}

bool AudioEngine::isRecording() const noexcept { return recording.load(std::memory_order_relaxed); }

double AudioEngine::getRecordSeconds() const noexcept
{
    const double rate = sampleRate.load(std::memory_order_relaxed);
    return rate > 0.0 ? (double) recordedSamples.load(std::memory_order_relaxed) / rate : 0.0;
}

juce::File AudioEngine::getRecordingFile() const { return recordingFile; }

} // namespace xenhusk
