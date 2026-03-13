#pragma once
#include <JuceHeader.h>
#include "GrainHead.h"

class GrainEngine
{
public:
    GrainEngine() = default;

    void prepare (double sampleRate, int blockSize)
    {
        for (auto& head : heads)
            head.prepare (sampleRate, blockSize);
    }

    void process (juce::AudioBuffer<float>& output, int numSamples,
                  const class RingBuffer& ringBuffer,
                  const juce::AudioBuffer<float>* sampleBuffer,
                  bool liveMode)
    {
        for (auto& head : heads)
            head.process (output, numSamples, ringBuffer, sampleBuffer, liveMode);

        // Soft clip output
        for (int ch = 0; ch < output.getNumChannels(); ++ch)
        {
            float* data = output.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                data[i] = std::tanh (data[i] * masterGain);
        }
    }

    void setMasterGain (float g) { masterGain = g; }
    GrainHead& getHead (int index) { return heads[index]; }

private:
    std::array<GrainHead, kNumHeads> heads;
    float masterGain = 1.0f;
};
