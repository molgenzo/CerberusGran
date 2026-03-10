#pragma once
#include <JuceHeader.h>
#include "GrainHead.h"
#include "RingBuffer.h"

static constexpr int kNumHeads = 5;

class GrainEngine
{
public:
    GrainEngine();

    void prepare (double sampleRate, int maxBlockSize);
    void process (juce::AudioBuffer<float>& buffer, int numSamples,
                  const RingBuffer& ringBuffer,
                  const juce::AudioBuffer<float>* sampleBuffer,
                  bool liveMode);

    GrainHead& getHead (int index) { return heads[index]; }
    const GrainHead& getHead (int index) const { return heads[index]; }

    void setMasterGain (float g) { masterGain = g; }

private:
    std::array<GrainHead, kNumHeads> heads;
    float masterGain = 1.0f;
};
