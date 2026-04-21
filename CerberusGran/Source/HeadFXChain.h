#pragma once
#include <JuceHeader.h>
#include "Bitcrusher.h"
#include "CombFilter.h"

class HeadFXChain
{
public:
    HeadFXChain() = default;

    void prepare (double sr, int maxBlockSize)
    {
        sampleRate = sr;

        // Standard SVF filter
        juce::dsp::ProcessSpec spec { sr, static_cast<juce::uint32> (maxBlockSize), 2 };
        svfFilter.prepare (spec);
        svfFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);

        // Comb filter bank
        combFilter.prepare (sr, maxBlockSize);

        // Scratch buffer for the filter wet signal
        filterWetBuffer.setSize (2, maxBlockSize);
        filterWetBuffer.clear();

        // Bitcrusher
        crusher.reset();

        // Delay (max 2 seconds)
        maxDelaySamples = static_cast<int> (sr * 2.0);
        delayBuffer.setSize (2, maxDelaySamples);
        delayBuffer.clear();
        delayWritePos = 0;

        // Reverb
        reverb.setSampleRate (sr);
        reverb.reset();
    }

    void process (juce::AudioBuffer<float>& buffer, int numSamples)
    {
        // ========== Filter stage ==========
        if (filterOn)
        {
            const int ch = juce::jmin (2, buffer.getNumChannels());
            if (filterWetBuffer.getNumSamples() < numSamples)
                filterWetBuffer.setSize (2, numSamples, false, false, true);

            // Copy dry → wet scratch
            for (int c = 0; c < ch; ++c)
                filterWetBuffer.copyFrom (c, 0, buffer, c, 0, numSamples);

            if (CombFilter::isCombType (filterType))
            {
                combFilter.setType (filterType);
                combFilter.setCombFreq (filterCombFreq);
                combFilter.setCutoff (filterCutoff);
                combFilter.setResonance (filterRes);
                combFilter.setDrive (filterDrive);
                combFilter.process (filterWetBuffer, numSamples);
            }
            else
            {
                svfFilter.setCutoffFrequency (filterCutoff);
                svfFilter.setResonance (filterRes);
                switch (filterType)
                {
                    case 0: svfFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);  break;
                    case 1: svfFilter.setType (juce::dsp::StateVariableTPTFilterType::highpass); break;
                    case 2: svfFilter.setType (juce::dsp::StateVariableTPTFilterType::bandpass); break;
                    default: break;
                }

                for (int i = 0; i < numSamples; ++i)
                {
                    float l = filterWetBuffer.getSample (0, i);
                    float r = ch > 1 ? filterWetBuffer.getSample (1, i) : l;
                    filterWetBuffer.setSample (0, i, svfFilter.processSample (0, l));
                    if (ch > 1)
                        filterWetBuffer.setSample (1, i, svfFilter.processSample (1, r));
                }

                // Optional gentle drive on standard filter types too
                if (filterDrive > 0.01f)
                {
                    float gainUp = 1.0f + filterDrive * 4.0f;
                    float gainDn = 1.0f / std::sqrt (gainUp);
                    for (int c = 0; c < ch; ++c)
                    {
                        float* d = filterWetBuffer.getWritePointer (c);
                        for (int i = 0; i < numSamples; ++i)
                            d[i] = std::tanh (d[i] * gainUp) * gainDn;
                    }
                }
            }

            // Apply pan on wet signal (equal-power)
            if (ch > 1)
            {
                float panNorm = 0.5f * (filterPan + 1.0f); // -1..1 -> 0..1
                float lGain = std::cos (panNorm * juce::MathConstants<float>::halfPi);
                float rGain = std::sin (panNorm * juce::MathConstants<float>::halfPi);
                float* wetL = filterWetBuffer.getWritePointer (0);
                float* wetR = filterWetBuffer.getWritePointer (1);
                for (int i = 0; i < numSamples; ++i)
                {
                    float m = 0.5f * (wetL[i] + wetR[i]);
                    wetL[i] = m * lGain * 1.41421356f; // restore energy vs mono mix
                    wetR[i] = m * rGain * 1.41421356f;
                }
            }

            // Dry/wet mix into buffer
            float wet = juce::jlimit (0.0f, 1.0f, filterMix);
            float dry = 1.0f - wet;
            for (int c = 0; c < ch; ++c)
            {
                float* dst = buffer.getWritePointer (c);
                const float* src = filterWetBuffer.getReadPointer (c);
                for (int i = 0; i < numSamples; ++i)
                    dst[i] = dst[i] * dry + src[i] * wet;
            }
        }

        // ========== Bitcrusher ==========
        if (crushOn)
        {
            crusher.setBits (crushBits);
            crusher.setRateReduction (crushRate);

            for (int i = 0; i < numSamples; ++i)
            {
                float l = buffer.getSample (0, i);
                float r = buffer.getNumChannels() > 1 ? buffer.getSample (1, i) : l;
                crusher.processStereo (l, r);
                buffer.setSample (0, i, l);
                if (buffer.getNumChannels() > 1)
                    buffer.setSample (1, i, r);
            }
        }

        // ========== Delay ==========
        if (delayOn && maxDelaySamples > 0)
        {
            int delaySamps = juce::jlimit (1, maxDelaySamples - 1,
                static_cast<int> (delayTime * 0.001f * static_cast<float> (sampleRate)));

            for (int i = 0; i < numSamples; ++i)
            {
                int readPos = delayWritePos - delaySamps;
                if (readPos < 0) readPos += maxDelaySamples;

                for (int ch = 0; ch < juce::jmin (2, buffer.getNumChannels()); ++ch)
                {
                    float dry = buffer.getSample (ch, i);
                    float delayed = delayBuffer.getSample (ch, readPos);

                    delayBuffer.setSample (ch, delayWritePos, dry + delayed * delayFeedback);

                    buffer.setSample (ch, i, dry * (1.0f - delayMix) + delayed * delayMix);
                }

                if (++delayWritePos >= maxDelaySamples)
                    delayWritePos = 0;
            }
        }

        // ========== Reverb ==========
        if (reverbOn)
        {
            juce::Reverb::Parameters rp;
            rp.roomSize   = reverbSize;
            rp.damping    = reverbDamp;
            rp.wetLevel   = reverbMix;
            rp.dryLevel   = 1.0f - reverbMix;
            rp.width      = 1.0f;
            rp.freezeMode = 0.0f;
            reverb.setParameters (rp);

            if (buffer.getNumChannels() >= 2)
                reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), numSamples);
            else
                reverb.processMono (buffer.getWritePointer (0), numSamples);
        }
    }

    void reset()
    {
        svfFilter.reset();
        combFilter.reset();
        crusher.reset();
        delayBuffer.clear();
        delayWritePos = 0;
        reverb.reset();
    }

    // ---- Filter setters ----
    void setFilterEnabled (bool on)   { filterOn = on; }
    void setFilterType (int t)        { filterType = t; }
    void setFilterCutoff (float hz)   { filterCutoff = hz; }
    void setFilterResonance (float r) { filterRes = r; }
    void setFilterPan (float p)       { filterPan = juce::jlimit (-1.0f, 1.0f, p); }
    void setFilterDrive (float d)     { filterDrive = juce::jlimit (0.0f, 1.0f, d); }
    void setFilterCombFreq (float f)  { filterCombFreq = juce::jlimit (20.0f, 5000.0f, f); }
    void setFilterMix (float m)       { filterMix = juce::jlimit (0.0f, 1.0f, m); }

    // ---- Bitcrusher setters ----
    void setCrushEnabled (bool on)    { crushOn = on; }
    void setCrushBits (float b)       { crushBits = b; }
    void setCrushRate (float r)       { crushRate = r; }

    // ---- Delay setters ----
    void setDelayEnabled (bool on)    { delayOn = on; }
    void setDelayTime (float ms)      { delayTime = ms; }
    void setDelayFeedback (float f)   { delayFeedback = f; }
    void setDelayMix (float m)        { delayMix = m; }

    // ---- Reverb setters ----
    void setReverbEnabled (bool on)   { reverbOn = on; }
    void setReverbSize (float s)      { reverbSize = s; }
    void setReverbDamp (float d)      { reverbDamp = d; }
    void setReverbMix (float m)       { reverbMix = m; }

private:
    double sampleRate = 44100.0;

    // Filter
    juce::dsp::StateVariableTPTFilter<float> svfFilter;
    CombFilter combFilter;
    juce::AudioBuffer<float> filterWetBuffer;
    bool  filterOn = false;
    int   filterType = 0;
    float filterCutoff = 1000.0f;
    float filterRes = 0.707f;
    float filterPan = 0.0f;
    float filterDrive = 0.0f;
    float filterCombFreq = 220.0f;
    float filterMix = 1.0f;

    // Bitcrusher
    Bitcrusher crusher;
    bool crushOn = false;
    float crushBits = 16.0f;
    float crushRate = 1.0f;

    // Delay
    juce::AudioBuffer<float> delayBuffer;
    int delayWritePos = 0;
    int maxDelaySamples = 0;
    bool delayOn = false;
    float delayTime = 250.0f;
    float delayFeedback = 0.3f;
    float delayMix = 0.5f;

    // Reverb
    juce::Reverb reverb;
    bool reverbOn = false;
    float reverbSize = 0.5f;
    float reverbDamp = 0.5f;
    float reverbMix = 0.3f;
};