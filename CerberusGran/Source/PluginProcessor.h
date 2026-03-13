#pragma once
#include <JuceHeader.h>
#include "RingBuffer.h"
#include "GrainEngine.h"

enum class AudioSourceMode { Live = 0, File = 1 };

class CerberusGranAudioProcessor : public juce::AudioProcessor
{
public:
    CerberusGranAudioProcessor();
    ~CerberusGranAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    const juce::String getName() const override { return "CerberusGran"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void loadSampleFile (const juce::File& file);

    RingBuffer& getRingBuffer() { return ringBuffer; }
    const juce::AudioBuffer<float>* getSampleBuffer() const { return sampleBuffer.load (std::memory_order_acquire); }
    AudioSourceMode getSourceMode() const { return static_cast<AudioSourceMode> (sourceMode.load (std::memory_order_relaxed)); }

    double getCurrentSampleRate() const { return currentSampleRate; }
    GrainEngine& getGrainEngine() { return grainEngine; }

    juce::AudioFormatManager& getFormatManager() { return formatManager; }
    juce::AudioThumbnailCache& getThumbnailCache() { return thumbnailCache; }

    juce::AudioProcessorValueTreeState apvts;

private:
    void updateParametersFromAPVTS();

    RingBuffer ringBuffer;
    double currentSampleRate = 44100.0;

    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache { 5 };

    std::atomic<int> sourceMode { 0 };
    std::atomic<bool> freeze { false };

    juce::AudioBuffer<float> sampleBufferA, sampleBufferB;
    std::atomic<juce::AudioBuffer<float>*> sampleBuffer { nullptr };
    bool useBufferA = true;

    GrainEngine grainEngine;
    juce::AudioBuffer<float> dryBuffer;

    // Cached param pointers
    std::atomic<float>* masterGainParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* freezeParam = nullptr;
    std::atomic<float>* sourceModeParam = nullptr;

    struct HeadParamPtrs
    {
        std::atomic<float>* enable = nullptr;
        std::atomic<float>* position = nullptr;
        std::atomic<float>* spread = nullptr;
        std::atomic<float>* rate = nullptr;
        std::atomic<float>* length = nullptr;
        std::atomic<float>* pitch = nullptr;
        std::atomic<float>* shape = nullptr;
        std::atomic<float>* reverse = nullptr;
        std::atomic<float>* gain = nullptr;
    };

    std::array<HeadParamPtrs, kNumHeads> headParams;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CerberusGranAudioProcessor)
};
