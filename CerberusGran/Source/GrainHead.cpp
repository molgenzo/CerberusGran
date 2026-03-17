#include "GrainHead.h"
#include "RingBuffer.h"

GrainHead::GrainHead() = default;

void GrainHead::prepare (double sr, int blockSize)
{
    sampleRate = sr;
    headFilter.prepare (sr, blockSize);
    headBuffer.setSize (2, blockSize);
}

void GrainHead::spawnGrain (const RingBuffer& ringBuffer,
                            const juce::AudioBuffer<float>* sampleBuffer,
                            bool liveMode)
{
    Grain* g = grainPool.acquire();
    if (g == nullptr) return;

    // Position: 0-100% with spread randomisation
    float scatterAmt = spread * (rng.nextFloat() * 2.0f - 1.0f);
    float pos = juce::jlimit (0.0f, 100.0f, position + scatterAmt) / 100.0f;

    if (liveMode)
    {
        int ringSize = ringBuffer.getBufferSize();
        int wp = ringBuffer.getWritePosition();
        g->readPosition = static_cast<double> (wp) - pos * ringSize;
        if (g->readPosition < 0.0)
            g->readPosition += ringSize;
    }
    else if (sampleBuffer != nullptr && sampleBuffer->getNumSamples() > 0)
    {
        g->readPosition = pos * (sampleBuffer->getNumSamples() - 1);
    }
    else
    {
        grainPool.release (g);
        return;
    }

    // Pitch: semitones to playback rate
    double rate = std::pow (2.0, static_cast<double> (pitchSt) / 12.0);
    g->playbackRate = reverse ? -rate : rate;

    g->durationSamples = static_cast<int> (lengthMs * 0.001 * sampleRate);
    g->currentSample = 0;
    g->amplitude = gainLinear;
    g->windowType = shape;
    g->panLeft = 0.707f;
    g->panRight = 0.707f;
}

void GrainHead::process (juce::AudioBuffer<float>& output, int numSamples,
                         const RingBuffer& ringBuffer,
                         const juce::AudioBuffer<float>* sampleBuffer,
                         bool liveMode)
{
    if (!enabled) return;

    // Clear the per-head temp buffer
    headBuffer.clear (0, numSamples);

    // Schedule new grains
    for (int i = 0; i < numSamples; ++i)
    {
        if (--samplesUntilNextGrain <= 0)
        {
            spawnGrain (ringBuffer, sampleBuffer, liveMode);
            int interval = static_cast<int> (rateMs * 0.001 * sampleRate);
            samplesUntilNextGrain = juce::jmax (1, interval);
        }
    }

    // Synthesise active grains into headBuffer (not output)
    for (Grain* g = grainPool.begin(); g != grainPool.end(); ++g)
    {
        if (!g->active) continue;

        for (int i = 0; i < numSamples; ++i)
        {
            if (g->currentSample >= g->durationSamples)
            {
                grainPool.release (g);
                break;
            }

            float phase = static_cast<float> (g->currentSample)
                        / static_cast<float> (g->durationSamples);
            float window = WindowFunctions::getValue (g->windowType, phase);

            float sampleL = 0.0f, sampleR = 0.0f;

            if (liveMode)
            {
                double rp = g->readPosition;
                int ringSize = ringBuffer.getBufferSize();
                while (rp < 0.0) rp += ringSize;
                while (rp >= ringSize) rp -= ringSize;

                sampleL = ringBuffer.readSample (0, rp);
                sampleR = ringBuffer.readSample (juce::jmin (1, ringBuffer.getNumChannels() - 1), rp);
            }
            else if (sampleBuffer != nullptr)
            {
                int bufLen = sampleBuffer->getNumSamples();
                double rp = g->readPosition;

                if (rp < 0.0) rp = 0.0;
                if (rp >= bufLen - 1) rp = bufLen - 1.001;

                int idx0 = static_cast<int> (rp);
                int idx1 = idx0 + 1;
                float frac = static_cast<float> (rp - idx0);

                idx0 = juce::jlimit (0, bufLen - 1, idx0);
                idx1 = juce::jlimit (0, bufLen - 1, idx1);

                sampleL = sampleBuffer->getSample (0, idx0) * (1.0f - frac)
                        + sampleBuffer->getSample (0, idx1) * frac;
                int rCh = juce::jmin (1, sampleBuffer->getNumChannels() - 1);
                sampleR = sampleBuffer->getSample (rCh, idx0) * (1.0f - frac)
                        + sampleBuffer->getSample (rCh, idx1) * frac;
            }

            float mono = (sampleL + sampleR) * 0.5f * window * g->amplitude;

            // Write to per-head buffer instead of output
            headBuffer.addSample (0, i, mono);
            if (headBuffer.getNumChannels() > 1)
                headBuffer.addSample (1, i, mono);

            g->readPosition += g->playbackRate;
            g->currentSample++;
        }
    }

    // Apply per-head filter
    headFilter.process (headBuffer, numSamples);

    // Mix filtered result into main output
    for (int ch = 0; ch < output.getNumChannels(); ++ch)
        output.addFrom (ch, 0, headBuffer, juce::jmin (ch, headBuffer.getNumChannels() - 1), 0, numSamples);
}