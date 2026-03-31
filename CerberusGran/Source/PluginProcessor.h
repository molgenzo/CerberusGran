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

    double getCurrentSampleRate() const { return currentSampleRate; }
    GrainEngine& getGrainEngine() { return grainEngine; }

    juce::AudioFormatManager& getFormatManager() { return formatManager; }
    juce::AudioThumbnailCache& getThumbnailCache() { return thumbnailCache; }

    juce::AudioProcessorValueTreeState apvts;

    // Grid info for waveform display (set during updateParametersFromAPVTS)
    std::atomic<float> syncGridMs { 0.0f };    // 0 = no grid
    std::atomic<bool> anySyncActive { false };

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
        std::atomic<float>* rateMode = nullptr;
        std::atomic<float>* rate = nullptr;
        std::atomic<float>* rateSyncDiv = nullptr;
        std::atomic<float>* rateSyncType = nullptr;
        std::atomic<float>* sizeLink = nullptr;
        std::atomic<float>* length = nullptr;
        std::atomic<float>* sizeRatio = nullptr;
        std::atomic<float>* pitch = nullptr;
        std::atomic<float>* shape = nullptr;
        std::atomic<float>* reverse = nullptr;
        std::atomic<float>* gain = nullptr;
        // FX chain
        std::atomic<float>* filterOn = nullptr;
        std::atomic<float>* filterType = nullptr;
        std::atomic<float>* filterCutoff = nullptr;
        std::atomic<float>* filterRes = nullptr;
        std::atomic<float>* crushOn = nullptr;
        std::atomic<float>* crushBits = nullptr;
        std::atomic<float>* crushRate = nullptr;
        std::atomic<float>* delayOn = nullptr;
        std::atomic<float>* delayTimeMode = nullptr;
        std::atomic<float>* delayTime = nullptr;
        std::atomic<float>* delaySyncDiv = nullptr;
        std::atomic<float>* delaySyncType = nullptr;
        std::atomic<float>* delayFeedback = nullptr;
        std::atomic<float>* delayMix = nullptr;
        std::atomic<float>* reverbOn = nullptr;
        std::atomic<float>* reverbSize = nullptr;
        std::atomic<float>* reverbDamp = nullptr;
        std::atomic<float>* reverbMix = nullptr;
    };

    std::array<HeadParamPtrs, kNumHeads> headParams;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CerberusGranAudioProcessor)
};
