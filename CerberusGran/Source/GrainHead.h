#pragma once
#include <JuceHeader.h>
#include "Grain.h"
#include "WindowFunctions.h"
#include "Parameters.h"
#include "HeadFXChain.h"

class GrainHead
{
public:
    GrainHead();

    void prepare (double sampleRate, int blockSize);
    void process (juce::AudioBuffer<float>& output, int numSamples,
                  const class RingBuffer& ringBuffer,
                  const juce::AudioBuffer<float>* sampleBuffer,
                  bool liveMode);

    // Grain params
    void setEnabled (bool e)          { enabled = e; }
    void setPosition (float p)        { position = p; }
    void setSpread (float s)          { spread = s; }
    void setRate (float ms)           { rateMs = ms; }
    void setLength (float ms)         { lengthMs = ms; }
    void setPitchSemitones (float st) { pitchSt = st; }
    void setShape (float s)           { shape = s; }
    void setReverse (bool r)          { reverse = r; }
    void setGainDb (float db)         { gainLinear = juce::Decibels::decibelsToGain (db); }

    // FX chain setters (forwarded to fxChain)
    void setFilterEnabled (bool on)   { fxChain.setFilterEnabled (on); }
    void setFilterType (int t)        { fxChain.setFilterType (t); }
    void setFilterCutoff (float hz)   { fxChain.setFilterCutoff (hz); }
    void setFilterResonance (float r) { fxChain.setFilterResonance (r); }

    void setCrushEnabled (bool on)    { fxChain.setCrushEnabled (on); }
    void setCrushBits (float b)       { fxChain.setCrushBits (b); }
    void setCrushRate (float r)       { fxChain.setCrushRate (r); }

    void setDelayEnabled (bool on)    { fxChain.setDelayEnabled (on); }
    void setDelayTime (float ms)      { fxChain.setDelayTime (ms); }
    void setDelayFeedback (float f)   { fxChain.setDelayFeedback (f); }
    void setDelayMix (float m)        { fxChain.setDelayMix (m); }

    void setReverbEnabled (bool on)   { fxChain.setReverbEnabled (on); }
    void setReverbSize (float s)      { fxChain.setReverbSize (s); }
    void setReverbDamp (float d)      { fxChain.setReverbDamp (d); }
    void setReverbMix (float m)       { fxChain.setReverbMix (m); }

private:
    void spawnGrain (const class RingBuffer& ringBuffer,
                     const juce::AudioBuffer<float>* sampleBuffer,
                     bool liveMode);

    bool enabled = false;
    float position = 50.0f;
    float spread = 0.0f;
    float rateMs = 100.0f;
    float lengthMs = 200.0f;
    float pitchSt = 0.0f;
    float shape = 0.0f;
    bool reverse = false;
    float gainLinear = 1.0f;

    double sampleRate = 44100.0;
    int samplesUntilNextGrain = 0;

    GrainPool grainPool;
    juce::Random rng;

    // Per-head intermediate buffer and FX chain
    juce::AudioBuffer<float> headBuffer;
    HeadFXChain fxChain;
};
