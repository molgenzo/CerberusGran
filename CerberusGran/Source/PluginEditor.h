#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class HeadStrip : public juce::Component
{
public:
    HeadStrip (CerberusGranAudioProcessor& p, int headIndex, juce::Colour headColour);
    ~HeadStrip() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    CerberusGranAudioProcessor& processor;
    int index;
    juce::Colour colour;

    juce::ToggleButton enableBtn;
    juce::Slider posSlider, spreadSlider, rateSlider, lengthSlider, pitchSlider, gainSlider;
    juce::ComboBox shapeBox;
    juce::ToggleButton reverseBtn;

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ButtonAttach> enableAttach;
    std::unique_ptr<SliderAttach> posAttach, spreadAttach, rateAttach, lengthAttach, pitchAttach, gainAttach;
    std::unique_ptr<ComboAttach>  shapeAttach;
    std::unique_ptr<ButtonAttach> reverseAttach;
};

class CerberusGranAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         public juce::FileDragAndDropTarget,
                                         public juce::Timer
{
public:
    explicit CerberusGranAudioProcessorEditor (CerberusGranAudioProcessor&);
    ~CerberusGranAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

private:
    CerberusGranAudioProcessor& audioProcessor;

    juce::AudioThumbnail thumbnail;
    bool fileLoaded = false;
    bool dragOver = false;

    // Live waveform display buffer (downsampled for drawing)
    static constexpr int kWaveformPoints = 800;
    std::array<float, kWaveformPoints> liveWaveform {};
    void updateLiveWaveform();

    static constexpr int kNumHeadStrips = 5;
    std::array<juce::Colour, kNumHeadStrips> headColours;
    juce::OwnedArray<HeadStrip> headStrips;

    // Global controls
    juce::Slider masterGainSlider, mixSlider;
    juce::ToggleButton freezeBtn;
    juce::ComboBox sourceModeBox;

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttach> masterGainAttach, mixAttach;
    std::unique_ptr<ButtonAttach> freezeAttach;
    std::unique_ptr<ComboAttach>  sourceModeAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CerberusGranAudioProcessorEditor)
};
