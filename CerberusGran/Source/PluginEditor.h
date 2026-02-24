#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "WaveformDropComponent.h"

class CerberusGranAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
  explicit CerberusGranAudioProcessorEditor (CerberusGranAudioProcessor&);
  ~CerberusGranAudioProcessorEditor() override = default;

  void paint (juce::Graphics&) override;
  void resized() override;

private:
  CerberusGranAudioProcessor& audioProcessor;
  WaveformDropComponent waveformDropComponent;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CerberusGranAudioProcessorEditor)
};
