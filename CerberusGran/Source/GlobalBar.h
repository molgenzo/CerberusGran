#pragma once
#include <JuceHeader.h>

class GlobalBar : public juce::Component
{
public:
    GlobalBar (juce::AudioProcessorValueTreeState& apvts,
               const std::array<juce::Colour, 5>& colours)
        : apvtsRef (apvts), headColours (colours)
    {
        // Head navigation - invisible buttons, triangles painted in paint()
        prevBtn.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        prevBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
        prevBtn.onClick = [this] { switchHead (currentHead - 1); };
        addAndMakeVisible (prevBtn);

        nextBtn.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        nextBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
        nextBtn.onClick = [this] { switchHead (currentHead + 1); };
        addAndMakeVisible (nextBtn);

        // Plugin name
        nameLabel.setText ("Pocket Granules", juce::dontSendNotification);
        nameLabel.setFont (juce::Font ("Avenir", 16.0f, juce::Font::bold));
        nameLabel.setColour (juce::Label::textColourId, juce::Colour (0xffeeeeee));
        addAndMakeVisible (nameLabel);

        // Source mode selector
        sourceModeBox.addItemList ({ "Live", "File" }, 1);
        sourceModeBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2E2E34));
        sourceModeBox.setColour (juce::ComboBox::textColourId, juce::Colour (0xffeeeeee));
        sourceModeBox.setColour (juce::ComboBox::arrowColourId, juce::Colour (0xffdddddd));
        sourceModeBox.onChange = [this] { onModeChanged(); };
        addAndMakeVisible (sourceModeBox);
        sourceModeAttach = std::make_unique<ComboAttach> (apvts, "sourceMode", sourceModeBox);

        // Preset actions dropdown in the same visual style as source mode selector
        presetActionBox.addItem ("Save As...", 1);
        presetActionBox.addItem ("Load", 2);
        presetActionBox.setTextWhenNothingSelected ("Presets");
        presetActionBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2E2E34));
        presetActionBox.setColour (juce::ComboBox::textColourId, juce::Colour (0xffeeeeee));
        presetActionBox.setColour (juce::ComboBox::arrowColourId, juce::Colour (0xffdddddd));
        presetActionBox.onChange = [this] { onPresetActionChanged(); };
        addAndMakeVisible (presetActionBox);

        // Gain horizontal bar slider
        gainSlider.setSliderStyle (juce::Slider::LinearBar);
        gainSlider.setTextBoxIsEditable (false);
        gainSlider.setTextValueSuffix (" dB");
        gainSlider.setScrollWheelEnabled (false);
        gainSlider.setSliderSnapsToMousePosition (false);
        addAndMakeVisible (gainSlider);
        gainAttach = std::make_unique<SliderAttach> (apvts, "masterGain", gainSlider);

        // Dry/Wet horizontal bar slider
        mixSlider.setSliderStyle (juce::Slider::LinearBar);
        mixSlider.setTextBoxIsEditable (false);
        mixSlider.setTextValueSuffix ("%");
        mixSlider.setScrollWheelEnabled (false);
        mixSlider.setSliderSnapsToMousePosition (true);
        addAndMakeVisible (mixSlider);
        mixAttach = std::make_unique<SliderAttach> (apvts, "mix", mixSlider);
    }

    void switchHead (int newHead)
    {
        newHead = juce::jlimit (0, 4, newHead);
        if (newHead == currentHead) return;
        currentHead = newHead;
        repaint();
        if (onHeadChanged) onHeadChanged (currentHead);
    }

    int getCurrentHead() const { return currentHead; }

    std::function<void(int)> onHeadChanged;
    std::function<void()> onPresetSaveAs;
    std::function<void()> onPresetLoad;

    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colour (0xff1E1E22));
        g.fillRect (getLocalBounds());

        // Bottom border
        g.setColour (juce::Colour (0xff3A3A40));
        g.drawLine (0.0f, static_cast<float> (getHeight()),
                    static_cast<float> (getWidth()), static_cast<float> (getHeight()), 0.5f);

        // Nav triangles
        {
            auto pb = prevBtn.getBounds().toFloat();
            float cx = pb.getCentreX(), cy = pb.getCentreY(), sz = 7.0f;
            juce::Path tri;
            tri.addTriangle (cx + sz, cy - sz, cx + sz, cy + sz, cx - sz, cy);
            g.setColour (juce::Colour (0xffdddddd));
            g.fillPath (tri);
        }
        {
            auto nb = nextBtn.getBounds().toFloat();
            float cx = nb.getCentreX(), cy = nb.getCentreY(), sz = 7.0f;
            juce::Path tri;
            tri.addTriangle (cx - sz, cy - sz, cx - sz, cy + sz, cx + sz, cy);
            g.setColour (juce::Colour (0xffdddddd));
            g.fillPath (tri);
        }

        // Head badge
        auto badgeColour = headColours[currentHead];
        float badgeSize = 26.0f;
        float badgeX = prevBtn.getRight() + 3.0f;
        float badgeY = getHeight() * 0.5f - badgeSize * 0.5f;
        g.setColour (badgeColour);
        g.fillRoundedRectangle (badgeX, badgeY, badgeSize, badgeSize, 7.0f);
        g.setColour (juce::Colour (0xff1A1A1E));
        g.setFont (juce::Font ("Avenir", 14.0f, juce::Font::bold));
        g.drawText (juce::String (currentHead + 1),
                    static_cast<int> (badgeX), static_cast<int> (badgeY),
                    static_cast<int> (badgeSize), static_cast<int> (badgeSize),
                    juce::Justification::centred);

        // Labels above sliders
        g.setColour (juce::Colour (0xff999999));
        g.setFont (juce::Font ("Avenir", 9.0f, juce::Font::plain));
        g.drawText ("Gain", gainSlider.getBounds().translated (0, -11).withHeight (10),
                    juce::Justification::centredLeft);
        g.drawText ("D/W", mixSlider.getBounds().translated (0, -11).withHeight (10),
                    juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (8, 4);
        int h = area.getHeight();
        int btnH = 24;
        int cy = area.getY() + (h - btnH) / 2;

        // [<] [badge] [>]
        prevBtn.setBounds (area.removeFromLeft (24).withHeight (btnH).withY (cy));
        area.removeFromLeft (34); // badge space
        nextBtn.setBounds (area.removeFromLeft (24).withHeight (btnH).withY (cy));
        area.removeFromLeft (8);

        // Plugin name
        nameLabel.setBounds (area.removeFromLeft (132).withHeight (h));
        area.removeFromLeft (4);

        // Left-side dropdowns (Presets on the left, Live/File on the right with spacing)
        presetActionBox.setBounds (area.removeFromLeft (98).withHeight (btnH).withY (cy));
        area.removeFromLeft (12);
        sourceModeBox.setBounds (area.removeFromLeft (60).withHeight (btnH).withY (cy));

        // Right side: horizontal bar sliders
        int barW = 70;
        int barH = 16;
        int barGap = 8;
        int barY = cy + (btnH - barH) / 2 + 4;

        mixSlider.setBounds (area.getRight() - barW, barY, barW, barH);
        gainSlider.setBounds (area.getRight() - barW * 2 - barGap, barY, barW, barH);
    }

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    const std::array<juce::Colour, 5>& headColours;
    int currentHead = 0;

    juce::TextButton prevBtn, nextBtn;
    juce::Label nameLabel;
    juce::ComboBox sourceModeBox;
    juce::ComboBox presetActionBox;
    juce::Slider gainSlider, mixSlider;

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttach> gainAttach, mixAttach;
    std::unique_ptr<ComboAttach> sourceModeAttach;
    bool presetActionResetInProgress = false;

    void onModeChanged()
    {
        // Keep D/W controllable in both modes so switching during playback
        // doesn't force a fully-wet state and mute expected dry behavior.
        mixSlider.setEnabled (true);
        mixSlider.setAlpha (1.0f);
    }

    void onPresetActionChanged()
    {
        if (presetActionResetInProgress)
            return;

        const auto selectedId = presetActionBox.getSelectedId();

        if (selectedId == 1)
        {
            if (onPresetSaveAs) onPresetSaveAs();
        }
        else if (selectedId == 2)
        {
            if (onPresetLoad) onPresetLoad();
        }

        // Return to placeholder text after executing an action.
        presetActionResetInProgress = true;
        presetActionBox.setSelectedId (0, juce::dontSendNotification);
        presetActionResetInProgress = false;
    }
};
