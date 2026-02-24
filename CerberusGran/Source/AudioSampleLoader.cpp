#include "AudioSampleLoader.h"

AudioSampleLoader::AudioSampleLoader()
{
    formatManager.registerBasicFormats();
    sampleBuffers[0] = std::make_unique<juce::AudioBuffer<float>>();
    sampleBuffers[1] = std::make_unique<juce::AudioBuffer<float>>();
}

bool AudioSampleLoader::loadFromFile(const juce::File& file, juce::String& errorMessage)
{
    if (!file.existsAsFile())
    {
        errorMessage = "File does not exist.";
        return false;
    }

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr)
    {
        errorMessage = "Unsupported or unreadable audio file.";
        return false;
    }

    const auto lengthInSamples = static_cast<int>(reader->lengthInSamples);
    const auto channelCount = juce::jlimit(1, 2, static_cast<int>(reader->numChannels));

    if (lengthInSamples <= 0)
    {
        errorMessage = "Audio file is empty.";
        return false;
    }

    const auto nextIndex = 1 - activeBufferIndex.load(std::memory_order_acquire);
    auto& nextBuffer = *sampleBuffers[static_cast<size_t>(nextIndex)];
    nextBuffer.setSize(channelCount, lengthInSamples, false, true, true);

    if (!reader->read(&nextBuffer, 0, lengthInSamples, 0, true, true))
    {
        errorMessage = "Failed to read audio data.";
        return false;
    }

    activeBuffer.store(&nextBuffer, std::memory_order_release);
    activeBufferIndex.store(nextIndex, std::memory_order_release);

    {
        const juce::ScopedLock lock(loadedFileNameLock);
        loadedFileName = file.getFileName();
    }

    errorMessage.clear();
    return true;
}

const juce::AudioBuffer<float>* AudioSampleLoader::getActiveBuffer() const noexcept
{
    return activeBuffer.load(std::memory_order_acquire);
}

juce::String AudioSampleLoader::getLoadedFileName() const
{
    const juce::ScopedLock lock(loadedFileNameLock);
    return loadedFileName;
}
