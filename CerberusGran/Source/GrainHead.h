#pragma once
#include <JuceHeader.h>
#include "Grain.h"
#include "WindowFunctions.h"
#include "Parameters.h"
#include "HeadFilter.h"

class GrainHead
{
public:
    GrainHead();

    void prepare (double sampleRate, int blockSize);
    void process (juce::AudioBuffer<float>& output, int numSamples,
                  const class RingBuffer& ringBuffer,
                  const juce::AudioBuffer<float>* sampleBuffer,
                  bool liveMode);

    void setEnabled (bool e)          { enabled = e; }
    void setPosition (float p)        { position = p; }        // 0-100 %
    void setSpread (float s)          { spread = s; }          // 0-50 %
    void setRate (float ms)           { rateMs = ms; }         // ms between grains
    void setLength (float ms)         { lengthMs = ms; }       // grain duration ms
    void setPitchSemitones (float st) { pitchSt = st; }        // -24 to +24
    void setShape (int s)             { shape = s; }           // window type
    void setReverse (bool r)          { reverse = r; }
    void setGainDb (float db)         { gainLinear = juce::Decibels::decibelsToGain (db); }

    // Filter: type 0=Off, 1=LPF, 2=HPF, 3=BPF
    void setFilterParams (int type, float cutoffHz, float q)
    {
        headFilter.update (type, cutoffHz, q);
    }

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
    int shape = 0;
    bool reverse = false;
    float gainLinear = 1.0f;

    double sampleRate = 44100.0;
    int samplesUntilNextGrain = 0;

    GrainPool grainPool;
    juce::Random rng;

    // Per-head filter and temp buffer
    HeadFilter headFilter;
    juce::AudioBuffer<float> headBuffer;
};