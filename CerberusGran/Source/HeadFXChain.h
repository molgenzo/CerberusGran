#pragma once
#include <JuceHeader.h>
#include "Bitcrusher.h"

class HeadFXChain
{
public:
    HeadFXChain() = default;

    void prepare (double sr, int maxBlockSize)
    {
        sampleRate = sr;

        // Filter
        juce::dsp::ProcessSpec spec { sr, static_cast<juce::uint32> (maxBlockSize), 2 };
        svfFilter.prepare (spec);
        svfFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);

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
        // Filter
        if (filterOn)
        {
            svfFilter.setCutoffFrequency (filterCutoff);
            svfFilter.setResonance (filterRes);

            switch (filterType)
            {
                case 0: svfFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass); break;
                case 1: svfFilter.setType (juce::dsp::StateVariableTPTFilterType::highpass); break;
                case 2: svfFilter.setType (juce::dsp::StateVariableTPTFilterType::bandpass); break;
            }

            for (int i = 0; i < numSamples; ++i)
            {
                float l = buffer.getSample (0, i);
                float r = buffer.getNumChannels() > 1 ? buffer.getSample (1, i) : l;
                buffer.setSample (0, i, svfFilter.processSample (0, l));
                if (buffer.getNumChannels() > 1)
                    buffer.setSample (1, i, svfFilter.processSample (1, r));
            }
        }

        // Bitcrusher
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

        // Delay
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

        // Reverb
        if (reverbOn)
        {
            juce::Reverb::Parameters rp;
            rp.roomSize = reverbSize;
            rp.damping = reverbDamp;
            rp.wetLevel = reverbMix;
            rp.dryLevel = 1.0f - reverbMix;
            rp.width = 1.0f;
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
        crusher.reset();
        delayBuffer.clear();
        delayWritePos = 0;
        reverb.reset();
    }

    // Filter setters
    void setFilterEnabled (bool on)   { filterOn = on; }
    void setFilterType (int t)        { filterType = t; }
    void setFilterCutoff (float hz)   { filterCutoff = hz; }
    void setFilterResonance (float r) { filterRes = r; }

    // Bitcrusher setters
    void setCrushEnabled (bool on)    { crushOn = on; }
    void setCrushBits (float b)       { crushBits = b; }
    void setCrushRate (float r)       { crushRate = r; }

    // Delay setters
    void setDelayEnabled (bool on)    { delayOn = on; }
    void setDelayTime (float ms)      { delayTime = ms; }
    void setDelayFeedback (float f)   { delayFeedback = f; }
    void setDelayMix (float m)        { delayMix = m; }

    // Reverb setters
    void setReverbEnabled (bool on)   { reverbOn = on; }
    void setReverbSize (float s)      { reverbSize = s; }
    void setReverbDamp (float d)      { reverbDamp = d; }
    void setReverbMix (float m)       { reverbMix = m; }

private:
    double sampleRate = 44100.0;

    // Filter
    juce::dsp::StateVariableTPTFilter<float> svfFilter;
    bool filterOn = false;
    int filterType = 0;
    float filterCutoff = 1000.0f;
    float filterRes = 0.707f;

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
