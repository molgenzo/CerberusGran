#include "PluginEditor.h"

CerberusGranAudioProcessorEditor::CerberusGranAudioProcessorEditor (CerberusGranAudioProcessor& p)
: juce::AudioProcessorEditor (&p), audioProcessor (p)
{
  setSize (420, 220);
}

void CerberusGranAudioProcessorEditor::paint (juce::Graphics& g)
{
  g.fillAll (juce::Colours::black);
  g.setColour (juce::Colours::white);
  g.setFont (20.0f);
  g.drawFittedText ("CerberusGran", getLocalBounds(), juce::Justification::centred, 1);
}

void CerberusGranAudioProcessorEditor::resized() {}
