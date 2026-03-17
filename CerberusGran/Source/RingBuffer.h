#pragma once
#include <JuceHeader.h>
#include <atomic>

class RingBuffer
{
public:
    RingBuffer() = default;

    void prepare (int numChannels, int bufferSizeIn)
    {
        bufferSize = bufferSizeIn;
        buffer.setSize (numChannels, bufferSize);
        buffer.clear();
        writePos.store (0, std::memory_order_relaxed);
    }

    void write (const juce::AudioBuffer<float>& input, int numSamples)
    {
        int wp = writePos.load (std::memory_order_relaxed);
        int channels = juce::jmin (buffer.getNumChannels(), input.getNumChannels());

        for (int i = 0; i < numSamples; ++i)
        {
            for (int ch = 0; ch < channels; ++ch)
                buffer.setSample (ch, wp, input.getSample (ch, i));

            if (++wp >= bufferSize)
                wp = 0;
        }

        writePos.store (wp, std::memory_order_release);
    }

    float readSample (int channel, double position) const
    {
        if (bufferSize == 0) return 0.0f;

        int idx0 = static_cast<int> (position);
        int idx1 = idx0 + 1;
        float frac = static_cast<float> (position - idx0);

        idx0 = ((idx0 % bufferSize) + bufferSize) % bufferSize;
        idx1 = ((idx1 % bufferSize) + bufferSize) % bufferSize;

        channel = juce::jmin (channel, buffer.getNumChannels() - 1);
        return buffer.getSample (channel, idx0) * (1.0f - frac)
             + buffer.getSample (channel, idx1) * frac;
    }

    int getWritePosition() const { return writePos.load (std::memory_order_acquire); }
    int getBufferSize() const { return bufferSize; }
    int getNumChannels() const { return buffer.getNumChannels(); }

private:
    juce::AudioBuffer<float> buffer;
    int bufferSize = 0;
    std::atomic<int> writePos { 0 };
};
