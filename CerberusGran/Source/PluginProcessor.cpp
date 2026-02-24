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

bool CerberusGranAudioProcessor::loadAudioFile(const juce::File& file, juce::String& errorMessage)
{
    return sampleLoader.loadFromFile(file, errorMessage);
}

const juce::AudioBuffer<float>* CerberusGranAudioProcessor::getCurrentSampleBuffer() const noexcept
{
    return sampleLoader.getActiveBuffer();
}

juce::String CerberusGranAudioProcessor::getLoadedFileName() const
{
    return sampleLoader.getLoadedFileName();
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new CerberusGranAudioProcessor();
}

