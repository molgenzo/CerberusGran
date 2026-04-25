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

    // Override label/value colours (used by light-themed panels)
    void setTextColours (juce::Colour labelCol, juce::Colour valueCol)
    {
        label.setColour (juce::Label::textColourId, labelCol);
        valueLabel.setColour (juce::Label::textColourId, valueCol);
    }

    // Override the knob's filled circle colour (default is dark for the main UI)
    void setKnobBgColour (juce::Colour c)
    {
        slider.getProperties().set ("knobBgColour", (juce::int64) c.getARGB());
    }

    void setKnobOutline (bool show)
    {
        if (show)  slider.getProperties().remove ("noKnobOutline");
        else       slider.getProperties().set ("noKnobOutline", true);
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
