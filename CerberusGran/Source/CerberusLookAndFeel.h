#pragma once
#include <JuceHeader.h>

class CerberusLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CerberusLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xff1A1A1E));
        setColour (juce::Label::textColourId, juce::Colour (0xffcccccc));
        setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffcccccc));
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2A2A30));
        setColour (juce::ComboBox::textColourId, juce::Colour (0xffcccccc));
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff3A3A40));
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff2A2A30));
        setColour (juce::PopupMenu::textColourId, juce::Colour (0xffcccccc));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff3A3A40));
    }

    juce::Label* createSliderTextBox (juce::Slider& slider) override
    {
        auto* label = juce::LookAndFeel_V4::createSliderTextBox (slider);
        label->setFont (juce::FontOptions (10.0f));
        label->setColour (juce::Label::textColourId, juce::Colour (0xffaaaaaa));
        return label;
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
        float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        float centreX = bounds.getCentreX();
        float centreY = bounds.getCentreY();

        // Get accent colour from component property, fallback to white
        auto accentVar = slider.getProperties() ["accentColour"];
        juce::Colour accent = accentVar.isVoid()
            ? juce::Colour (0xff888888)
            : juce::Colour (static_cast<juce::uint32> ((juce::int64) accentVar));

        // Outer circle outline
        g.setColour (juce::Colour (0xff3A3A40));
        g.drawEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f, 0.75f);

        // Value arc
        float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        juce::Path arc;
        arc.addCentredArc (centreX, centreY, radius - 1.0f, radius - 1.0f,
                           0.0f, rotaryStartAngle, angle, true);
        g.setColour (accent);
        g.strokePath (arc, juce::PathStrokeType (2.0f));

        // Indicator dot at end of arc
        float dotRadius = 3.0f;
        float dotX = centreX + (radius - 1.0f) * std::cos (angle - juce::MathConstants<float>::halfPi);
        float dotY = centreY + (radius - 1.0f) * std::sin (angle - juce::MathConstants<float>::halfPi);
        g.fillEllipse (dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        juce::ignoreUnused (shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

        auto bounds = button.getLocalBounds().toFloat();

        // Pill toggle
        float pillW = juce::jmin (40.0f, bounds.getWidth());
        float pillH = juce::jmin (20.0f, bounds.getHeight());
        float pillX = bounds.getCentreX() - pillW * 0.5f;
        float pillY = bounds.getCentreY() - pillH * 0.5f;
        float pillRadius = pillH * 0.5f;

        auto accentVar = button.getProperties() ["accentColour"];
        juce::Colour accent = accentVar.isVoid()
            ? juce::Colour (0xff4CAF50)
            : juce::Colour (static_cast<juce::uint32> ((juce::int64) accentVar));

        bool isOn = button.getToggleState();

        g.setColour (isOn ? accent : juce::Colour (0xff3A3A40));
        g.fillRoundedRectangle (pillX, pillY, pillW, pillH, pillRadius);

        // Thumb circle
        float thumbR = pillH * 0.4f;
        float thumbX = isOn ? (pillX + pillW - pillH * 0.5f) : (pillX + pillH * 0.5f);
        g.setColour (juce::Colour (0xffeeeeee));
        g.fillEllipse (thumbX - thumbR, pillY + pillH * 0.5f - thumbR, thumbR * 2.0f, thumbR * 2.0f);

        // Button text (if any) to the left
        auto text = button.getButtonText();
        if (text.isNotEmpty())
        {
            g.setColour (juce::Colour (0xffcccccc));
            g.setFont (juce::FontOptions (12.0f));
            g.drawText (text, bounds.withRight (pillX - 4.0f), juce::Justification::centredRight);
        }
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        juce::ignoreUnused (minSliderPos, maxSliderPos, style);

        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();

        auto accentVar = slider.getProperties() ["accentColour"];
        juce::Colour accent = accentVar.isVoid()
            ? juce::Colour (0xff888888)
            : juce::Colour (static_cast<juce::uint32> ((juce::int64) accentVar));

        // Track background — full height of the slider box
        g.setColour (juce::Colour (0xff3A3A40));
        g.fillRoundedRectangle (bounds, 3.0f);

        // Filled portion — full height
        float fillW = sliderPos - bounds.getX();
        g.setColour (accent.withAlpha (0.6f));
        g.fillRoundedRectangle (bounds.getX(), bounds.getY(), fillW, bounds.getHeight(), 3.0f);
    }
};
