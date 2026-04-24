#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <cmath>

class LFO
{
public:
    enum Shape { Sine = 0, Triangle, SawUp, SawDown, Square, SampleHold, kNumShapes };

    void prepare (double sr)
    {
        sampleRate = sr > 0 ? sr : 44100.0;
        phase = 0.0f;
        shRandom = 0.0f;
    }

    // Setters — called from UI/message thread or from processBlock for sync
    void setRateHz   (float hz)  { rateHz = juce::jmax (0.001f, hz); }
    void setShape    (int s)     { shape = juce::jlimit (0, (int) kNumShapes - 1, s); }
    void setDepth    (float d)   { depth = juce::jlimit (0.0f, 1.0f, d); }
    void setBipolar  (bool b)    { bipolar = b; }
    void setPhaseOff (float p)   { phaseOffset = p - std::floor (p); }

    // Advance phase by numSamples. Computes single output sample at block end.
    void advance (int numSamples)
    {
        if (sampleRate <= 0.0) { output.store (0.0f); return; }
        float inc = rateHz / static_cast<float> (sampleRate);
        float prevPhase = phase;
        phase += inc * numSamples;
        phase -= std::floor (phase);

        // Phase wrap detection for S&H
        if (shape == SampleHold && phase < prevPhase)
            shRandom = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;

        float v = shapeValue (phase + phaseOffset);
        if (! bipolar) v = v * 0.5f + 0.5f;
        output.store (v * depth);
    }

    float getOutput() const { return output.load (std::memory_order_relaxed); }
    float getPhase()  const { return phase; }

    // For UI preview at arbitrary phase
    float shapeValueAtPhase (float p) const
    {
        p = p + phaseOffset;
        p -= std::floor (p);
        float v = shapeValue (p);
        if (! bipolar) v = v * 0.5f + 0.5f;
        return v * depth;
    }

private:
    float shapeValue (float p) const
    {
        p -= std::floor (p);
        switch (shape)
        {
            case Sine:
                return std::sin (p * 2.0f * juce::MathConstants<float>::pi);
            case Triangle:
                return 1.0f - 4.0f * std::abs (p - 0.5f);
            case SawUp:
                return 2.0f * p - 1.0f;
            case SawDown:
                return 1.0f - 2.0f * p;
            case Square:
                return p < 0.5f ? 1.0f : -1.0f;
            case SampleHold:
                return shRandom;
            default:
                return 0.0f;
        }
    }

    double sampleRate = 44100.0;
    float rateHz = 1.0f;
    int shape = Sine;
    float depth = 1.0f;
    bool bipolar = true;
    float phaseOffset = 0.0f;

    float phase = 0.0f;
    mutable float shRandom = 0.0f;
    std::atomic<float> output { 0.0f };
};
