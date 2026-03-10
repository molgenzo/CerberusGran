#include "GrainEngine.h"

GrainEngine::GrainEngine()
{
    // Default positions spread across the buffer
    static constexpr float defaultPositions[kNumHeads] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };

    for (int i = 0; i < kNumHeads; ++i)
    {
        heads[i].setPosition (defaultPositions[i]);
        heads[i].setEnabled (i == 0);  // Only head 0 enabled by default
    }
}

void GrainEngine::prepare (double sampleRate, int maxBlockSize)
{
    for (auto& head : heads)
        head.prepare (sampleRate, maxBlockSize);
}

void GrainEngine::process (juce::AudioBuffer<float>& buffer, int numSamples,
                           const RingBuffer& ringBuffer,
                           const juce::AudioBuffer<float>* sampleBuffer,
                           bool liveMode)
{
    for (auto& head : heads)
        head.process (buffer, numSamples, ringBuffer, sampleBuffer, liveMode);

    // Soft clipper on output
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float* data = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
            data[i] = std::tanh (data[i] * masterGain);
    }
}
