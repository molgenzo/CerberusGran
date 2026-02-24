#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

class AudioSampleLoader
{
public:
    AudioSampleLoader();

    bool loadFromFile(const juce::File& file, juce::String& errorMessage);
    const juce::AudioBuffer<float>* getActiveBuffer() const noexcept;
    juce::String getLoadedFileName() const;

private:
    juce::AudioFormatManager formatManager;
    std::array<std::unique_ptr<juce::AudioBuffer<float>>, 2> sampleBuffers;
    std::atomic<juce::AudioBuffer<float>*> activeBuffer { nullptr };
    std::atomic<int> activeBufferIndex { 0 };

    mutable juce::CriticalSection loadedFileNameLock;
    juce::String loadedFileName;
};
