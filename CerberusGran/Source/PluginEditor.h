#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CerberusLookAndFeel.h"
#include "WaveformDisplay.h"
#include "EngineColumn.h"
#include "GlobalBar.h"

class CerberusGranAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         public juce::Timer
{
public:
    explicit CerberusGranAudioProcessorEditor (CerberusGranAudioProcessor&);
    ~CerberusGranAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    CerberusGranAudioProcessor& audioProcessor;
    CerberusLookAndFeel cerberusLnf;

    static constexpr int kNumCols = 5;
    std::array<juce::Colour, kNumCols> headColours;

    WaveformDisplay waveformDisplay;
    juce::OwnedArray<EngineColumn> columns;
    GlobalBar globalBar;

    int currentHeadIndex = 0;
    std::unique_ptr<juce::FileChooser> presetFileChooser;

    void switchToHead (int headIndex);
    void savePresetAs();
    void loadPresetFromDisk();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CerberusGranAudioProcessorEditor)
};
