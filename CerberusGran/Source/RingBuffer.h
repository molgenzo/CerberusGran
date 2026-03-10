#pragma once
#include <JuceHeader.h>
#include <atomic>

class RingBuffer
{
public:
    RingBuffer() = default;

    void prepare (int numChannels, int bufferSize)
    {
        buffer.setSize (numChannels, bufferSize);
        buffer.clear();
        this->bufferSize = bufferSize;
        writePos.store (0, std::memory_order_relaxed);
    }

    void write (const juce::AudioBuffer<float>& input, int numSamples)
    {
        const int nch = juce::jmin (buffer.getNumChannels(), input.getNumChannels());
        int wp = writePos.load (std::memory_order_relaxed);

        for (int ch = 0; ch < nch; ++ch)
        {
            const float* src = input.getReadPointer (ch);
            float* dst = buffer.getWritePointer (ch);

            int pos = wp;
            for (int i = 0; i < numSamples; ++i)
            {
                dst[pos] = src[i];
                if (++pos >= bufferSize) pos = 0;
            }
        }

        int newPos = wp + numSamples;
        if (newPos >= bufferSize) newPos -= bufferSize;
        writePos.store (newPos, std::memory_order_release);
    }

    float readSample (int channel, double readPosition) const
    {
        if (bufferSize == 0) return 0.0f;

        const float* data = buffer.getReadPointer (channel);
        int idx0 = static_cast<int> (readPosition);
        double frac = readPosition - idx0;

        idx0 = ((idx0 % bufferSize) + bufferSize) % bufferSize;
        int idx1 = idx0 + 1;
        if (idx1 >= bufferSize) idx1 = 0;

        return static_cast<float> (data[idx0] * (1.0 - frac) + data[idx1] * frac);
    }

    int getWritePosition() const { return writePos.load (std::memory_order_acquire); }
    int getBufferSize() const { return bufferSize; }
    int getNumChannels() const { return buffer.getNumChannels(); }

private:
    juce::AudioBuffer<float> buffer;
    int bufferSize = 0;
    std::atomic<int> writePos { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RingBuffer)
};
