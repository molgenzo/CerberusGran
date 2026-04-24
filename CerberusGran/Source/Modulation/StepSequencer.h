#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <array>

class StepSequencer
{
public:
    static constexpr int kMaxSteps = 16;
    enum PlayMode { Forward = 0, Reverse, PingPong, Random, kNumModes };

    StepSequencer()
    {
        for (auto& s : stepValues) s.store (0.0f);
    }

    void prepare (double sr)
    {
        sampleRate = sr > 0 ? sr : 44100.0;
        phase = 0.0f;
        currentStep = 0;
        pingPongDir = 1;
    }

    void setRateHz   (float hz)  { rateHz = juce::jmax (0.01f, hz); }
    void setLength   (int l)     { length = juce::jlimit (1, kMaxSteps, l); }
    void setPlayMode (int m)     { playMode = juce::jlimit (0, (int) kNumModes - 1, m); }
    void setBipolar  (bool b)    { bipolar = b; }
    void setSmooth   (float s)   { smooth = juce::jlimit (0.0f, 1.0f, s); }

    void setStepValue (int idx, float v)
    {
        if (idx >= 0 && idx < kMaxSteps)
            stepValues[idx].store (juce::jlimit (-1.0f, 1.0f, v));
    }

    float getStepValue (int idx) const
    {
        return stepValues[juce::jlimit (0, kMaxSteps - 1, idx)].load();
    }

    int getLength()      const { return length; }
    int getCurrentStep() const { return currentStep; }
    float getOutput()    const { return output.load (std::memory_order_relaxed); }

    void clearSteps()
    {
        for (auto& s : stepValues) s.store (0.0f);
    }

    // Advance sequencer by numSamples, update output
    void advance (int numSamples)
    {
        if (sampleRate <= 0 || rateHz <= 0 || length <= 0) return;

        float inc = rateHz / static_cast<float> (sampleRate);
        phase += inc * numSamples;
        while (phase >= 1.0f)
        {
            phase -= 1.0f;
            advanceStep();
        }

        float val = stepValues[currentStep].load();

        // Inter-step smoothing: blend into next step's value over `smooth` fraction of step time
        if (smooth > 0.001f)
        {
            int nextIdx = peekNextStep();
            float nextVal = stepValues[nextIdx].load();
            // blend in the last `smooth` fraction of the step
            float blendStart = 1.0f - smooth;
            if (phase > blendStart)
            {
                float t = (phase - blendStart) / smooth;
                t = juce::jlimit (0.0f, 1.0f, t);
                val = val * (1.0f - t) + nextVal * t;
            }
        }

        if (! bipolar) val = val * 0.5f + 0.5f;
        output.store (val);
    }

private:
    void advanceStep()
    {
        switch (playMode)
        {
            case Forward:
                currentStep = (currentStep + 1) % length;
                break;
            case Reverse:
                currentStep = (currentStep - 1 + length) % length;
                break;
            case PingPong:
                currentStep += pingPongDir;
                if (currentStep >= length - 1) { currentStep = length - 1; pingPongDir = -1; }
                else if (currentStep <= 0)     { currentStep = 0;          pingPongDir = 1; }
                break;
            case Random:
                currentStep = juce::Random::getSystemRandom().nextInt (length);
                break;
        }
    }

    int peekNextStep() const
    {
        switch (playMode)
        {
            case Forward: return (currentStep + 1) % length;
            case Reverse: return (currentStep - 1 + length) % length;
            case PingPong:
            {
                int n = currentStep + pingPongDir;
                if (n < 0 || n >= length) return currentStep;
                return n;
            }
            case Random: return currentStep;
        }
        return currentStep;
    }

    double sampleRate = 44100.0;
    float rateHz = 2.0f;
    int length = 16;
    int playMode = Forward;
    bool bipolar = false;
    float smooth = 0.0f;

    std::array<std::atomic<float>, kMaxSteps> stepValues;
    int currentStep = 0;
    int pingPongDir = 1;
    float phase = 0.0f;
    std::atomic<float> output { 0.0f };
};
