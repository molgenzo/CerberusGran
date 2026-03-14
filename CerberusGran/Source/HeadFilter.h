#pragma once
#include <JuceHeader.h>

class HeadFilter
{
public:
    void prepare (double sampleRate, int blockSize)
    {
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32> (blockSize);
        spec.numChannels = 2;
        filter.prepare (spec);
        filter.reset();
    }

    // type: 0=Off, 1=LPF, 2=HPF, 3=BPF
    void update (int type, float cutoffHz, float q)
    {
        if (type == 0) { filterType = 0; return; }

        filterType = type;
        cutoffHz = juce::jlimit (20.0f, static_cast<float> (spec.sampleRate * 0.499), cutoffHz);
        q = juce::jmax (0.1f, q);

        using CoeffType = juce::dsp::IIR::Coefficients<float>;

        switch (type)
        {
            case 1: *filter.state = *CoeffType::makeLowPass  (spec.sampleRate, cutoffHz, q); break;
            case 2: *filter.state = *CoeffType::makeHighPass  (spec.sampleRate, cutoffHz, q); break;
            case 3: *filter.state = *CoeffType::makeBandPass  (spec.sampleRate, cutoffHz, q); break;
            default: filterType = 0; break;
        }
    }

    void process (juce::AudioBuffer<float>& buffer, int numSamples)
    {
        if (filterType == 0) return;

        juce::dsp::AudioBlock<float> block (buffer);
        auto sub = block.getSubBlock (0, static_cast<size_t> (numSamples));
        juce::dsp::ProcessContextReplacing<float> ctx (sub);
        filter.process (ctx);
    }

    void reset() { filter.reset(); }

private:
    juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>> filter;

    juce::dsp::ProcessSpec spec {};
    int filterType = 0;
};