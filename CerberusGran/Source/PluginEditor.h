#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class HeadPanel : public juce::Component
{
public:
    HeadPanel (CerberusGranAudioProcessor& p, int headIndex, juce::Colour headColour);
    ~HeadPanel() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setCollapsed (bool c) { collapsed = c; resized(); }
    bool isCollapsed() const { return collapsed; }

private:
    CerberusGranAudioProcessor& processor;
    int index;
    juce::Colour colour;
    bool collapsed = false;

    juce::TextButton collapseButton;
    juce::Label titleLabel;

    // Grain knobs
    juce::Slider positionSlider, posScatterSlider, densitySlider, durationSlider;
    juce::Slider pitchSlider, pitchScatterSlider, gainSlider, panSlider;
    juce::ComboBox windowBox;

    // DSP controls
    juce::ToggleButton filterBypassBtn, satBypassBtn, crushBypassBtn, delayBypassBtn;
    juce::ComboBox filterTypeBox;
    juce::Slider filterCutoffSlider, filterResSlider;
    juce::Slider satDriveSlider;
    juce::Slider crushBitsSlider, crushRateSlider;
    juce::Slider delayTimeSlider, delayFeedbackSlider, delayMixSlider;

    // LFO
    juce::Slider lfoRateSlider, lfoDepthSlider;
    juce::ComboBox lfoShapeBox, lfoTargetBox;

    // APVTS attachments
    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ButtonAttach> enableAttach;
    std::unique_ptr<SliderAttach> positionAttach, posScatterAttach, densityAttach, durationAttach;
    std::unique_ptr<SliderAttach> pitchAttach, pitchScatterAttach, gainAttach, panAttach;
    std::unique_ptr<ComboAttach>  windowAttach;

    std::unique_ptr<ButtonAttach> filterBypassAttach, satBypassAttach, crushBypassAttach, delayBypassAttach;
    std::unique_ptr<ComboAttach>  filterTypeAttach;
    std::unique_ptr<SliderAttach> filterCutoffAttach, filterResAttach;
    std::unique_ptr<SliderAttach> satDriveAttach;
    std::unique_ptr<SliderAttach> crushBitsAttach, crushRateAttach;
    std::unique_ptr<SliderAttach> delayTimeAttach, delayFeedbackAttach, delayMixAttach;

    std::unique_ptr<SliderAttach> lfoRateAttach, lfoDepthAttach;
    std::unique_ptr<ComboAttach>  lfoShapeAttach, lfoTargetAttach;

    void setupSlider (juce::Slider& s, const juce::String& suffix = "");
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

    // Head panels
    static constexpr int kNumHeads = 5;
    std::array<juce::Colour, kNumHeads> headColours;
    juce::OwnedArray<HeadPanel> headPanels;

    // Global controls
    juce::Slider masterGainSlider, dryWetSlider;
    juce::ComboBox sourceModeBox;

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SliderAttach> masterGainAttach, dryWetAttach;
    std::unique_ptr<ComboAttach>  sourceModeAttach;

    // Viewport for head panels
    juce::Viewport headViewport;
    juce::Component headContainer;

    void setupSlider (juce::Slider& s, const juce::String& suffix = "");

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CerberusGranAudioProcessorEditor)
};
