#pragma once
#include <JuceHeader.h>

class GlobalBar : public juce::Component
{
public:
    GlobalBar (juce::AudioProcessorValueTreeState& apvts)
    {
        nameLabel.setText ("CerberusGran", juce::dontSendNotification);
        nameLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        nameLabel.setColour (juce::Label::textColourId, juce::Colour (0xffeeeeee));
        addAndMakeVisible (nameLabel);

        freezeBtn.setButtonText ("Freeze");
        addAndMakeVisible (freezeBtn);
        freezeAttach = std::make_unique<ButtonAttach> (apvts, "freeze", freezeBtn);

        sourceModeBox.addItemList ({ "Live", "File" }, 1);
        addAndMakeVisible (sourceModeBox);
        sourceModeAttach = std::make_unique<ComboAttach> (apvts, "sourceMode", sourceModeBox);

        gainSlider.setSliderStyle (juce::Slider::LinearBar);
        gainSlider.setTextBoxIsEditable (true);
        gainSlider.setTextValueSuffix ("x");
        addAndMakeVisible (gainSlider);
        gainAttach = std::make_unique<SliderAttach> (apvts, "masterGain", gainSlider);

        mixSlider.setSliderStyle (juce::Slider::LinearBar);
        mixSlider.setTextBoxIsEditable (true);
        mixSlider.setTextValueSuffix ("%");
        addAndMakeVisible (mixSlider);
        mixAttach = std::make_unique<SliderAttach> (apvts, "mix", mixSlider);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colour (0xff151518));
        g.fillRect (getLocalBounds());
        g.setColour (juce::Colour (0xff3A3A40));
        g.drawLine (0.0f, 0.0f, static_cast<float> (getWidth()), 0.0f, 0.5f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (10, 6);

        nameLabel.setBounds (area.removeFromLeft (120));
        area.removeFromLeft (20);

        freezeBtn.setBounds (area.removeFromLeft (60).withHeight (22).withY (area.getY() + 2));
        area.removeFromLeft (8);
        sourceModeBox.setBounds (area.removeFromLeft (65).withHeight (22).withY (area.getY() + 2));

        auto right = area.removeFromRight (200);
        mixSlider.setBounds (right.removeFromRight (80).withHeight (22).withY (right.getY() + 2));
        right.removeFromRight (8);
        gainSlider.setBounds (right.removeFromRight (80).withHeight (22).withY (right.getY() + 2));
    }

private:
    juce::Label nameLabel;
    juce::ToggleButton freezeBtn;
    juce::ComboBox sourceModeBox;
    juce::Slider gainSlider, mixSlider;

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttach> gainAttach, mixAttach;
    std::unique_ptr<ButtonAttach> freezeAttach;
    std::unique_ptr<ComboAttach> sourceModeAttach;
};
