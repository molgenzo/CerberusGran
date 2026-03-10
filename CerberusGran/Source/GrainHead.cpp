#include "GrainHead.h"

GrainHead::GrainHead() = default;

void GrainHead::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr;
    headBuffer.setSize (2, maxBlockSize);
    headBuffer.clear();
    samplesUntilNextGrain = 0;
    WindowFunctions::initialise();

    // SmoothedValues
    filterCutoff.reset (sr, 0.02);
    filterResonance.reset (sr, 0.02);
    saturatorDrive.reset (sr, 0.02);
    delayTimeMs.reset (sr, 0.02);
    delayFeedback.reset (sr, 0.02);
    delayMix.reset (sr, 0.02);

    // Filter
    juce::dsp::ProcessSpec spec { sr, static_cast<juce::uint32> (maxBlockSize), 1 };
    filterL.prepare (spec);
    filterR.prepare (spec);
    filterL.setCutoffFrequency (1000.0f);
    filterR.setCutoffFrequency (1000.0f);

    // Bitcrusher
    bitcrusher.prepare (sr);

    // Delay
    delayLineL.prepare (spec);
    delayLineR.prepare (spec);
    delayLineL.reset();
    delayLineR.reset();
}

void GrainHead::setFilterType (int type)
{
    using Type = juce::dsp::StateVariableTPTFilterType;
    switch (type)
    {
        case 0: filterL.setType (Type::lowpass);  filterR.setType (Type::lowpass);  break;
        case 1: filterL.setType (Type::highpass); filterR.setType (Type::highpass); break;
        case 2: filterL.setType (Type::bandpass); filterR.setType (Type::bandpass); break;
        default: break;
    }
}

void GrainHead::process (juce::AudioBuffer<float>& outputBuffer, int numSamples,
                         const RingBuffer& ringBuffer,
                         const juce::AudioBuffer<float>* sampleBuffer,
                         bool liveMode)
{
    if (!enabled) return;

    headBuffer.clear (0, numSamples);

    // --- Grain synthesis ---
    for (int i = 0; i < numSamples; ++i)
    {
        if (--samplesUntilNextGrain <= 0)
        {
            triggerGrain (ringBuffer, sampleBuffer, liveMode);
            int interval = static_cast<int> (sampleRate / static_cast<double> (density));
            samplesUntilNextGrain = juce::jmax (1, interval);
        }

        for (auto& grain : pool)
        {
            if (!grain.active) continue;

            float phase = static_cast<float> (grain.currentSample)
                        / static_cast<float> (grain.durationSamples);
            float env = WindowFunctions::lookup (grain.windowType, phase);

            if (liveMode)
            {
                int nch = ringBuffer.getNumChannels();
                for (int ch = 0; ch < juce::jmin (2, nch); ++ch)
                {
                    float sample = ringBuffer.readSample (ch, grain.readPosition);
                    float panGain = (ch == 0) ? grain.panLeft : grain.panRight;
                    headBuffer.addSample (ch, i, sample * env * grain.amplitude * panGain);
                }
            }
            else if (sampleBuffer != nullptr && sampleBuffer->getNumSamples() > 0)
            {
                int nch = sampleBuffer->getNumChannels();
                int bufLen = sampleBuffer->getNumSamples();
                for (int ch = 0; ch < juce::jmin (2, nch); ++ch)
                {
                    const float* data = sampleBuffer->getReadPointer (ch);
                    int idx0 = static_cast<int> (grain.readPosition);
                    double frac = grain.readPosition - idx0;

                    idx0 = ((idx0 % bufLen) + bufLen) % bufLen;
                    int idx1 = (idx0 + 1) % bufLen;

                    float sample = static_cast<float> (data[idx0] * (1.0 - frac) + data[idx1] * frac);
                    float panGain = (ch == 0) ? grain.panLeft : grain.panRight;
                    headBuffer.addSample (ch, i, sample * env * grain.amplitude * panGain);
                }
            }

            grain.readPosition += grain.playbackRate;
            grain.currentSample++;

            if (grain.currentSample >= grain.durationSamples)
                pool.release (&grain);
        }
    }

    // --- DSP Chain: Filter -> Saturator -> Bitcrusher -> Delay ---

    // Filter
    if (!filterBypassed)
    {
        float cutoff = filterCutoff.getNextValue();
        float res = filterResonance.getNextValue();
        filterL.setCutoffFrequency (cutoff);
        filterR.setCutoffFrequency (cutoff);
        filterL.setResonance (res);
        filterR.setResonance (res);

        float* left = headBuffer.getWritePointer (0);
        float* right = headBuffer.getWritePointer (1);
        for (int i = 0; i < numSamples; ++i)
        {
            left[i]  = filterL.processSample (0, left[i]);
            right[i] = filterR.processSample (0, right[i]);
        }
    }

    // Saturator
    if (!saturatorBypassed)
    {
        float* left = headBuffer.getWritePointer (0);
        float* right = headBuffer.getWritePointer (1);
        for (int i = 0; i < numSamples; ++i)
        {
            float drive = saturatorDrive.getNextValue();
            left[i]  = std::tanh (left[i] * drive);
            right[i] = std::tanh (right[i] * drive);
        }
    }

    // Bitcrusher
    if (!bitcrusherBypassed)
    {
        bitcrusher.setBitDepth (crusherBits);
        bitcrusher.setRateReduction (crusherRate);

        float* left = headBuffer.getWritePointer (0);
        float* right = headBuffer.getWritePointer (1);
        for (int i = 0; i < numSamples; ++i)
            bitcrusher.processStereo (left[i], right[i]);
    }

    // Delay
    if (!delayBypassed)
    {
        float* left = headBuffer.getWritePointer (0);
        float* right = headBuffer.getWritePointer (1);
        for (int i = 0; i < numSamples; ++i)
        {
            float time = delayTimeMs.getNextValue();
            float fb   = delayFeedback.getNextValue();
            float mix  = delayMix.getNextValue();

            float delaySamples = static_cast<float> (time * 0.001 * sampleRate);
            delayLineL.setDelay (delaySamples);
            delayLineR.setDelay (delaySamples);

            float dryL = left[i];
            float dryR = right[i];
            float wetL = delayLineL.popSample (0);
            float wetR = delayLineR.popSample (0);

            delayLineL.pushSample (0, dryL + wetL * fb);
            delayLineR.pushSample (0, dryR + wetR * fb);

            left[i]  = dryL * (1.0f - mix) + wetL * mix;
            right[i] = dryR * (1.0f - mix) + wetR * mix;
        }
    }

    // Add to output
    for (int ch = 0; ch < juce::jmin (2, outputBuffer.getNumChannels()); ++ch)
        outputBuffer.addFrom (ch, 0, headBuffer, ch, 0, numSamples, gain);
}

void GrainHead::triggerGrain (const RingBuffer& ringBuffer,
                              const juce::AudioBuffer<float>* sampleBuffer,
                              bool liveMode)
{
    Grain* g = pool.acquire();
    if (g == nullptr) return;

    float scatter = posScatter > 0.0f ? (random.nextFloat() * 2.0f - 1.0f) * posScatter : 0.0f;
    float pos = juce::jlimit (0.0f, 1.0f, position + scatter);

    float pScatter = pitchScatter > 0.0f ? (random.nextFloat() * 2.0f - 1.0f) * pitchScatter : 0.0f;
    g->playbackRate = static_cast<double> (pitch + pScatter);
    if (g->playbackRate < 0.01) g->playbackRate = 0.01;

    g->durationSamples = juce::jmax (1, static_cast<int> (grainDurationMs * 0.001 * sampleRate));
    g->currentSample = 0;
    g->amplitude = 1.0f;
    g->windowType = windowType;

    float panAngle = pan * (juce::MathConstants<float>::pi * 0.25f);
    g->panLeft  = std::cos (panAngle + juce::MathConstants<float>::pi * 0.25f);
    g->panRight = std::sin (panAngle + juce::MathConstants<float>::pi * 0.25f);

    if (liveMode)
    {
        int bufSize = ringBuffer.getBufferSize();
        int wp = ringBuffer.getWritePosition();
        int lookback = static_cast<int> (pos * static_cast<float> (bufSize));
        int startPos = wp - lookback;
        if (startPos < 0) startPos += bufSize;
        g->readPosition = static_cast<double> (startPos);
    }
    else if (sampleBuffer != nullptr && sampleBuffer->getNumSamples() > 0)
    {
        g->readPosition = static_cast<double> (pos * static_cast<float> (sampleBuffer->getNumSamples()));
    }
    else
    {
        pool.release (g);
    }
}
