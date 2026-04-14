#pragma once
#include <JuceHeader.h>
#include "WindowFunctions.h"

class GrainShapeKnob : public juce::Component
{
public:
    GrainShapeKnob()
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        slider.setScrollWheelEnabled (false);
        addAndMakeVisible (slider);

        label.setText ("Shape", juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        label.setColour (juce::Label::textColourId, juce::Colour (0xffaaaaaa));
        addAndMakeVisible (label);

        slider.onValueChange = [this] { repaint(); };
    }

    void setAccentColour (juce::Colour c)
    {
        accentColour = c;
        slider.getProperties().set ("accentColour", (juce::int64) c.getARGB());
    }

    juce::Slider& getSlider() { return slider; }

    void paint (juce::Graphics& g) override
    {
        // Draw shape preview inside the knob area (between top and label)
        auto knobArea = getLocalBounds().withTrimmedBottom (26); // leave room for label
        auto centre = knobArea.getCentre().toFloat();
        float radius = juce::jmin (knobArea.getWidth(), knobArea.getHeight()) * 0.5f - 6.0f;

        float previewW = radius * 0.9f;
        float previewH = radius * 0.5f;
        float previewX = centre.x - previewW * 0.5f;
        float previewY = centre.y - previewH * 0.5f;

        float shapeVal = static_cast<float> (slider.getValue());

        juce::Path shapePath;
        int numPts = static_cast<int> (previewW);

        for (int i = 0; i <= numPts; ++i)
        {
            float phase = static_cast<float> (i) / static_cast<float> (numPts);
            float val = WindowFunctions::getValue (shapeVal, phase);
            float x = previewX + phase * previewW;
            float y = previewY + previewH * (1.0f - val);

            if (i == 0) shapePath.startNewSubPath (x, y);
            else shapePath.lineTo (x, y);
        }

        g.setColour (juce::Colour (0xffcccccc));
        g.strokePath (shapePath, juce::PathStrokeType (1.5f));
    }

    void resized() override
    {
        auto area = getLocalBounds();
        // Same layout as RotaryKnob: knob on top, label below (no value readout)
        area.removeFromBottom (12); // match the valueLabel space in RotaryKnob
        label.setBounds (area.removeFromBottom (14));
        slider.setBounds (area);
    }

private:
    juce::Slider slider;
    juce::Label label;
    juce::Colour accentColour { 0xff888888 };
};
