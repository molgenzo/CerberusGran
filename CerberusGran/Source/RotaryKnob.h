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

        // Name label — bright white, larger
        label.setText (labelText, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::Font ("Avenir", 14.5f, juce::Font::bold));
        label.setColour (juce::Label::textColourId, juce::Colour (0xffdddddd));
        addAndMakeVisible (label);

        // Value readout — dimmer
        valueLabel.setJustificationType (juce::Justification::centred);
        valueLabel.setFont (juce::Font ("Avenir", 9.0f, juce::Font::bold));
        valueLabel.setColour (juce::Label::textColourId, juce::Colour (0xff999999));
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
        label.setBounds (area.removeFromBottom (16));
        // Trim bottom of slider area so knob circle sits close to the label
        area.removeFromBottom (-2);
        // Make slider area square so the knob circle doesn't stretch
        int side = juce::jmin (area.getWidth(), area.getHeight());
        auto sliderArea = area.withSizeKeepingCentre (side, side);
        slider.setBounds (sliderArea);

        if (slider.onValueChange)
            slider.onValueChange();
    }

private:
    juce::Slider slider;
    juce::Label label;
    juce::Label valueLabel;
    juce::String suffixStr;
};
