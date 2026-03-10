#pragma once
#include <JuceHeader.h>
#include "Grain.h"
#include "WindowFunctions.h"
#include "RingBuffer.h"
#include "Bitcrusher.h"

class GrainHead
{
public:
    GrainHead();

    void prepare (double sampleRate, int maxBlockSize);
    void process (juce::AudioBuffer<float>& outputBuffer, int numSamples,
                  const RingBuffer& ringBuffer,
                  const juce::AudioBuffer<float>* sampleBuffer,
                  bool liveMode);

    // Grain parameters
    void setEnabled (bool e)         { enabled = e; }
    bool isEnabled() const           { return enabled; }

    void setPosition (float p)       { position = p; }
    void setPositionScatter (float s){ posScatter = s; }
    void setDensity (float d)        { density = juce::jmax (0.1f, d); }
    void setGrainDuration (float ms) { grainDurationMs = juce::jmax (1.0f, ms); }
    void setPitch (float p)          { pitch = p; }
    void setPitchScatter (float s)   { pitchScatter = s; }
    void setGain (float g)           { gain = g; }
    void setPan (float p)            { pan = p; }
    void setWindowType (int w)       { windowType = w; }

    // DSP bypass
    void setFilterBypassed (bool b)       { filterBypassed = b; }
    void setSaturatorBypassed (bool b)    { saturatorBypassed = b; }
    void setBitcrusherBypassed (bool b)   { bitcrusherBypassed = b; }
    void setDelayBypassed (bool b)        { delayBypassed = b; }

    // Filter params
    void setFilterType (int type);
    void setFilterCutoff (float freq)     { filterCutoff.setTargetValue (freq); }
    void setFilterResonance (float q)     { filterResonance.setTargetValue (q); }

    // Saturator params
    void setSaturatorDrive (float d)      { saturatorDrive.setTargetValue (d); }

    // Bitcrusher params
    void setBitcrusherBits (float b)      { crusherBits = b; }
    void setBitcrusherRate (float r)      { crusherRate = r; }

    // Delay params
    void setDelayTime (float ms)          { delayTimeMs.setTargetValue (ms); }
    void setDelayFeedback (float fb)      { delayFeedback.setTargetValue (fb); }
    void setDelayMix (float m)            { delayMix.setTargetValue (m); }

private:
    void triggerGrain (const RingBuffer& ringBuffer,
                       const juce::AudioBuffer<float>* sampleBuffer,
                       bool liveMode);

    GrainPool pool;
    juce::AudioBuffer<float> headBuffer;

    double sampleRate = 44100.0;
    bool enabled = true;

    // Grain params
    float position = 0.5f;
    float posScatter = 0.0f;
    float density = 10.0f;
    float grainDurationMs = 80.0f;
    float pitch = 1.0f;
    float pitchScatter = 0.0f;
    float gain = 0.8f;
    float pan = 0.0f;
    int windowType = 0;

    int samplesUntilNextGrain = 0;
    juce::Random random;

    // DSP chain
    bool filterBypassed = true;
    bool saturatorBypassed = true;
    bool bitcrusherBypassed = true;
    bool delayBypassed = true;

    // Filter
    juce::dsp::StateVariableTPTFilter<float> filterL, filterR;
    juce::SmoothedValue<float> filterCutoff { 1000.0f };
    juce::SmoothedValue<float> filterResonance { 0.707f };

    // Saturator
    juce::SmoothedValue<float> saturatorDrive { 1.0f };

    // Bitcrusher
    Bitcrusher bitcrusher;
    float crusherBits = 16.0f;
    float crusherRate = 1.0f;

    // Delay
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLineL { 192000 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLineR { 192000 };
    juce::SmoothedValue<float> delayTimeMs { 250.0f };
    juce::SmoothedValue<float> delayFeedback { 0.3f };
    juce::SmoothedValue<float> delayMix { 0.5f };
};
