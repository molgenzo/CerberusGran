#include "PluginProcessor.h"
#include "PluginEditor.h"

CerberusGranAudioProcessor::CerberusGranAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

void CerberusGranAudioProcessor::prepareToPlay(double, int) {}

void CerberusGranAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                              juce::MidiBuffer &)
{
    juce::ScopedNoDenormals noDenormals;

    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto *data = buffer.getWritePointer(ch);

        for (int i = 0; i < numSamples; ++i)
            data[i] *= 0.5f;
    }
}

juce::AudioProcessorEditor *CerberusGranAudioProcessor::createEditor()
{
    return new CerberusGranAudioProcessorEditor(*this);
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new CerberusGranAudioProcessor();
}

