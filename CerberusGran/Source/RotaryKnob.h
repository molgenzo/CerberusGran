#pragma once
#include <JuceHeader.h>

class RotaryKnob : public juce::Component
{
public:
    RotaryKnob (const juce::String& labelText, const juce::String& suffix = "")
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 14);
        slider.setScrollWheelEnabled (false);
        if (suffix.isNotEmpty())
            slider.setTextValueSuffix (suffix);
        addAndMakeVisible (slider);

        label.setText (labelText, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        label.setColour (juce::Label::textColourId, juce::Colour (0xffaaaaaa));
        addAndMakeVisible (label);
    }

    void setAccentColour (juce::Colour c)
    {
        slider.getProperties().set ("accentColour", (juce::int64) c.getARGB());
    }

    juce::Slider& getSlider() { return slider; }

    void resized() override
    {
        auto area = getLocalBounds();
        label.setBounds (area.removeFromBottom (16));
        slider.setBounds (area);
    }

private:
    juce::Slider slider;
    juce::Label label;
};
