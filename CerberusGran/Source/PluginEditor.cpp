#include "PluginEditor.h"

CerberusGranAudioProcessorEditor::CerberusGranAudioProcessorEditor (CerberusGranAudioProcessor& p)
: juce::AudioProcessorEditor (&p), audioProcessor (p)
{
  waveformDropComponent.setFileDropHandler([this](const juce::File& file, juce::String& errorMessage)
  {
    return audioProcessor.loadAudioFile(file, errorMessage);
  });

  waveformDropComponent.setLoadedNameProvider([this]()
  {
    return audioProcessor.getLoadedFileName();
  });

  addAndMakeVisible(waveformDropComponent);
  setSize (800, 450);
}

void CerberusGranAudioProcessorEditor::paint (juce::Graphics& g)
{
  g.fillAll (juce::Colour::fromRGB (15, 16, 19));
  g.setColour (juce::Colours::white);
  g.setFont (20.0f);
  g.drawFittedText ("CerberusGran - Sample Loader", getLocalBounds().removeFromTop (40).reduced (16, 0), juce::Justification::centredLeft, 1);
}

void CerberusGranAudioProcessorEditor::resized()
{
  waveformDropComponent.setBounds(getLocalBounds().reduced(16).withTrimmedTop(48));
}
