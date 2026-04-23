#pragma once
#include <JuceHeader.h>

class CerberusLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CerberusLookAndFeel()
    {
        setDefaultSansSerifTypefaceName ("Avenir");
        setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xff1A1A1E));
        setColour (juce::Label::textColourId, juce::Colour (0xffcccccc));
        setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffcccccc));
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2A2A30));
        setColour (juce::ComboBox::textColourId, juce::Colour (0xffcccccc));
        setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
        setColour (juce::ComboBox::arrowColourId, juce::Colour (0xff666666));
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff2A2A30));
        setColour (juce::PopupMenu::textColourId, juce::Colour (0xffcccccc));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff3A3A40));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        float cornerSize = bounds.getHeight() * 0.45f;

        auto baseColour = backgroundColour;
        if (shouldDrawButtonAsDown)
            baseColour = baseColour.brighter (0.15f);
        else if (shouldDrawButtonAsHighlighted)
            baseColour = baseColour.brighter (0.08f);

        g.setColour (baseColour);
        g.fillRoundedRectangle (bounds, cornerSize);
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                       int, int, int, int, juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<int> (0, 0, width, height).toFloat();
        float cornerSize = bounds.getHeight() * 0.45f;

        g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle (bounds, cornerSize);

        float arrowX = static_cast<float> (width) - 10.0f;
        float arrowY = static_cast<float> (height) * 0.5f;
        juce::Path arrow;
        arrow.addTriangle (arrowX - 3.0f, arrowY - 2.0f,
                           arrowX + 3.0f, arrowY - 2.0f,
                           arrowX, arrowY + 2.5f);
        g.setColour (juce::Colour (0xff888888));
        g.fillPath (arrow);
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (6, 0, box.getWidth() - 16, box.getHeight());
        if (box.getComponentID() == "sourceModeBigCombo")
            label.setFont (juce::Font ("Avenir", 16.0f, juce::Font::bold));
        else
            label.setFont (juce::Font ("Avenir", 12.0f, juce::Font::bold));
    }

    juce::Font getPopupMenuFont() override
    {
        return juce::Font ("Avenir", 13.0f, juce::Font::bold);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font ("Avenir", 12.0f, juce::Font::bold);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override
    {
        return juce::Font ("Avenir", 13.0f, juce::Font::bold);
    }

    juce::Font getLabelFont (juce::Label& label) override
    {
        // Respect whatever font has been set on the label
        return label.getFont();
    }

    juce::Component* getComboBoxButton (juce::ComboBox&) { return nullptr; }

    juce::Label* createSliderTextBox (juce::Slider& slider) override
    {
        auto* label = juce::LookAndFeel_V4::createSliderTextBox (slider);
        label->setFont (juce::Font ("Avenir", 12.0f, juce::Font::bold));
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

        auto accentVar = slider.getProperties() ["accentColour"];
        juce::Colour accent = accentVar.isVoid()
            ? juce::Colour (0xff888888)
            : juce::Colour (static_cast<juce::uint32> ((juce::int64) accentVar));

        // Dark filled circle background
        g.setColour (juce::Colour (0xff1E1E24));
        g.fillEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

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

        float thumbR = pillH * 0.4f;
        float thumbX = isOn ? (pillX + pillW - pillH * 0.5f) : (pillX + pillH * 0.5f);
        g.setColour (juce::Colour (0xffeeeeee));
        g.fillEllipse (thumbX - thumbR, pillY + pillH * 0.5f - thumbR, thumbR * 2.0f, thumbR * 2.0f);

        auto text = button.getButtonText();
        if (text.isNotEmpty())
        {
            g.setColour (juce::Colour (0xffcccccc));
            g.setFont (juce::Font ("Avenir", 12.0f, juce::Font::plain));
            g.drawText (text, bounds.withRight (pillX - 4.0f), juce::Justification::centredRight);
        }
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        juce::ignoreUnused (minSliderPos, maxSliderPos);

        if (style == juce::Slider::LinearVertical)
        {
            // Segmented vertical bar (mini level meter)
            auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();

            g.setColour (juce::Colour (0xff2A2A30));
            g.fillRoundedRectangle (bounds, 2.0f);

            int numSegs = 7;
            float gap = 1.5f;
            float segH = (bounds.getHeight() - gap * (numSegs - 1)) / numSegs;

            for (int i = 0; i < numSegs; ++i)
            {
                // i=0 at bottom, i=numSegs-1 at top
                float segBottom = bounds.getBottom() - i * (segH + gap);
                float segTop = segBottom - segH;
                float segCenterY = (segTop + segBottom) * 0.5f;

                bool filled = segCenterY > sliderPos;

                if (filled)
                {
                    // Gradient: brighter at top of fill
                    float t = static_cast<float> (i) / static_cast<float> (numSegs - 1);
                    g.setColour (juce::Colour (0xff888888).interpolatedWith (
                        juce::Colour (0xffeeeeee), t));
                }
                else
                {
                    g.setColour (juce::Colour (0xff3A3A40));
                }

                g.fillRect (bounds.getX() + 1.5f, segTop, bounds.getWidth() - 3.0f, segH);
            }
        }
        else
        {
            // Horizontal linear bar
            auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();

            auto accentVar = slider.getProperties() ["accentColour"];
            juce::Colour accent = accentVar.isVoid()
                ? juce::Colour (0xff888888)
                : juce::Colour (static_cast<juce::uint32> ((juce::int64) accentVar));

            g.setColour (juce::Colour (0xff3A3A40));
            g.fillRoundedRectangle (bounds, 3.0f);

            float fillW = sliderPos - bounds.getX();
            g.setColour (accent.withAlpha (0.6f));
            g.fillRoundedRectangle (bounds.getX(), bounds.getY(), fillW, bounds.getHeight(), 3.0f);
        }
    }
};
