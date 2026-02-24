#pragma once
#include <JuceHeader.h>
#include "AudioSampleLoader.h"

class CerberusGranAudioProcessor : public juce::AudioProcessor
{
public:
  CerberusGranAudioProcessor();
  ~CerberusGranAudioProcessor() override = default;

  void prepareToPlay (double sampleRate, int samplesPerBlock) override;
  void releaseResources() override {}
  void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override { return true; }

  const juce::String getName() const override { return "CerberusGran"; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram (int) override {}
  const juce::String getProgramName (int) override { return {}; }
  void changeProgramName (int, const juce::String&) override {}

  void getStateInformation (juce::MemoryBlock&) override {}
  void setStateInformation (const void*, int) override {}

  bool loadAudioFile(const juce::File& file, juce::String& errorMessage);
  const juce::AudioBuffer<float>* getCurrentSampleBuffer() const noexcept;
  juce::String getLoadedFileName() const;

private:
  AudioSampleLoader sampleLoader;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CerberusGranAudioProcessor)
};
