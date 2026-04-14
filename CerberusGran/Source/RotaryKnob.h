#pragma once
#include <JuceHeader.h>

class RotaryKnob : public juce::Component
{
public:
    RotaryKnob (const juce::String& labelText, const juce::String& suffix = "")
        : suffixStr (suffix)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        slider.setScrollWheelEnabled (false);
        if (suffix.isNotEmpty())
            slider.setTextValueSuffix (suffix);
        addAndMakeVisible (slider);

        // Name label — below knob, bigger
        label.setText (labelText, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        label.setColour (juce::Label::textColourId, juce::Colour (0xffaaaaaa));
        addAndMakeVisible (label);

        // Value readout — below name, smaller
        valueLabel.setJustificationType (juce::Justification::centred);
        valueLabel.setFont (juce::FontOptions (9.0f));
        valueLabel.setColour (juce::Label::textColourId, juce::Colour (0xff666666));
        addAndMakeVisible (valueLabel);

        slider.onValueChange = [this]
        {
            juce::String text;
            if (slider.textFromValueFunction)
                text = slider.textFromValueFunction (slider.getValue());
            else
            {
                text = juce::String (slider.getValue(), 1);
                if (suffixStr.isNotEmpty()) text += suffixStr;
            }
            valueLabel.setText (text, juce::dontSendNotification);
        };
    }

    void setAccentColour (juce::Colour c)
    {
        slider.getProperties().set ("accentColour", (juce::int64) c.getARGB());
    }

    juce::Slider& getSlider() { return slider; }

    void resized() override
    {
        auto area = getLocalBounds();
        valueLabel.setBounds (area.removeFromBottom (12));
        label.setBounds (area.removeFromBottom (14));
        slider.setBounds (area);

        // Update value text
        if (slider.onValueChange)
            slider.onValueChange();
    }

private:
    juce::Slider slider;
    juce::Label label;
    juce::Label valueLabel;
    juce::String suffixStr;
};
