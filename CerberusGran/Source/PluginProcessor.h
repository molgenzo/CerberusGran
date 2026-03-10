#pragma once
#include <JuceHeader.h>
#include "RingBuffer.h"
#include "GrainEngine.h"
#include "Parameters.h"

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
    double getTailLengthSeconds() const override { return 2.0; }

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
    void setSourceMode (AudioSourceMode mode) { sourceMode.store (static_cast<int> (mode), std::memory_order_relaxed); }

    double getCurrentSampleRate() const { return currentSampleRate; }
    GrainEngine& getGrainEngine() { return grainEngine; }

    juce::AudioFormatManager& getFormatManager() { return formatManager; }
    juce::AudioThumbnailCache& getThumbnailCache() { return thumbnailCache; }

    juce::AudioProcessorValueTreeState apvts;

    // Per-head RMS for UI metering
    std::atomic<float> rmsLevels[kNumHeads] {};

private:
    void updateParametersFromAPVTS();

    RingBuffer ringBuffer;
    double currentSampleRate = 44100.0;

    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache { 5 };

    std::atomic<int> sourceMode { static_cast<int> (AudioSourceMode::Live) };

    juce::AudioBuffer<float> sampleBufferA, sampleBufferB;
    std::atomic<juce::AudioBuffer<float>*> sampleBuffer { nullptr };
    bool useBufferA = true;

    GrainEngine grainEngine;
    juce::AudioBuffer<float> dryBuffer;

    // Cached param pointers (set once in constructor)
    std::atomic<float>* masterGainParam = nullptr;
    std::atomic<float>* masterPitchParam = nullptr;
    std::atomic<float>* dryWetParam = nullptr;
    std::atomic<float>* sourceModeParam = nullptr;

    struct HeadParamPtrs
    {
        std::atomic<float>* enable = nullptr;
        std::atomic<float>* position = nullptr;
        std::atomic<float>* posScatter = nullptr;
        std::atomic<float>* density = nullptr;
        std::atomic<float>* duration = nullptr;
        std::atomic<float>* pitch = nullptr;
        std::atomic<float>* pitchScatter = nullptr;
        std::atomic<float>* window = nullptr;
        std::atomic<float>* gain = nullptr;
        std::atomic<float>* pan = nullptr;
        std::atomic<float>* filterBypass = nullptr;
        std::atomic<float>* filterType = nullptr;
        std::atomic<float>* filterCutoff = nullptr;
        std::atomic<float>* filterRes = nullptr;
        std::atomic<float>* satBypass = nullptr;
        std::atomic<float>* satDrive = nullptr;
        std::atomic<float>* crushBypass = nullptr;
        std::atomic<float>* crushBits = nullptr;
        std::atomic<float>* crushRate = nullptr;
        std::atomic<float>* delayBypass = nullptr;
        std::atomic<float>* delayTime = nullptr;
        std::atomic<float>* delayFeedback = nullptr;
        std::atomic<float>* delayMix = nullptr;
        std::atomic<float>* lfoRate = nullptr;
        std::atomic<float>* lfoDepth = nullptr;
        std::atomic<float>* lfoShape = nullptr;
        std::atomic<float>* lfoTarget = nullptr;
    };

    std::array<HeadParamPtrs, kNumHeads> headParams;

    // LFO state per head
    struct LFOState
    {
        double phase = 0.0;
        float shValue = 0.0f;
    };
    std::array<LFOState, kNumHeads> lfoStates;

    float computeLFO (int headIndex, float rate, int shape, int numSamples);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CerberusGranAudioProcessor)
};
