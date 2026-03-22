#pragma once
#include <JuceHeader.h>

class GlobalBar : public juce::Component
{
public:
    GlobalBar (juce::AudioProcessorValueTreeState& apvts)
        : apvtsRef (apvts)
    {
        nameLabel.setText ("CerberusGran", juce::dontSendNotification);
        nameLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        nameLabel.setColour (juce::Label::textColourId, juce::Colour (0xffeeeeee));
        addAndMakeVisible (nameLabel);

        freezeBtn.setButtonText ("Freeze");
        freezeBtn.setClickingTogglesState (true);
        freezeBtn.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2A2A30));
        freezeBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff8B5CF6));
        freezeBtn.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffaaaaaa));
        freezeBtn.setColour (juce::TextButton::textColourOnId, juce::Colour (0xff1A1A1E));
        addAndMakeVisible (freezeBtn);
        freezeAttach = std::make_unique<ButtonAttach> (apvts, "freeze", freezeBtn);

        modeLabel.setText ("Mode", juce::dontSendNotification);
        modeLabel.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        modeLabel.setColour (juce::Label::textColourId, juce::Colour (0xff888888));
        modeLabel.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (modeLabel);

        sourceModeBox.addItemList ({ "Live", "File" }, 1);
        sourceModeBox.onChange = [this] { onModeChanged(); };
        addAndMakeVisible (sourceModeBox);
        sourceModeAttach = std::make_unique<ComboAttach> (apvts, "sourceMode", sourceModeBox);

        gainLabel.setText ("Gain", juce::dontSendNotification);
        gainLabel.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        gainLabel.setColour (juce::Label::textColourId, juce::Colour (0xff888888));
        gainLabel.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (gainLabel);

        mixLabel.setText ("Dry/Wet", juce::dontSendNotification);
        mixLabel.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        mixLabel.setColour (juce::Label::textColourId, juce::Colour (0xff888888));
        mixLabel.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (mixLabel);

        gainSlider.setSliderStyle (juce::Slider::LinearBar);
        gainSlider.setTextBoxIsEditable (true);
        gainSlider.setTextValueSuffix (" dB");
        gainSlider.setSliderSnapsToMousePosition (false);
        addAndMakeVisible (gainSlider);
        gainAttach = std::make_unique<SliderAttach> (apvts, "masterGain", gainSlider);

        mixSlider.setSliderStyle (juce::Slider::LinearBar);
        mixSlider.setTextBoxIsEditable (false);
        mixSlider.setTextValueSuffix ("%");
        mixSlider.setSliderSnapsToMousePosition (true);
        addAndMakeVisible (mixSlider);
        mixAttach = std::make_unique<SliderAttach> (apvts, "mix", mixSlider);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colour (0xff151518));
        g.fillRect (getLocalBounds());
        g.setColour (juce::Colour (0xff3A3A40));
        float bottom = static_cast<float> (getHeight());
        g.drawLine (0.0f, bottom, static_cast<float> (getWidth()), bottom, 0.5f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (10, 6);

        nameLabel.setBounds (area.removeFromLeft (120));
        area.removeFromLeft (20);

        freezeBtn.setBounds (area.removeFromLeft (60).withHeight (22).withY (area.getY() + 2));
        area.removeFromLeft (12);
        modeLabel.setBounds (area.removeFromLeft (40).withHeight (22).withY (area.getY() + 2));
        area.removeFromLeft (4);
        sourceModeBox.setBounds (area.removeFromLeft (65).withHeight (22).withY (area.getY() + 2));

        auto right = area.removeFromRight (310);
        mixSlider.setBounds (right.removeFromRight (90).withHeight (22).withY (right.getY() + 2));
        mixLabel.setBounds (right.removeFromRight (55).withHeight (22).withY (right.getY() + 2));
        right.removeFromRight (10);
        gainSlider.setBounds (right.removeFromRight (90).withHeight (22).withY (right.getY() + 2));
        gainLabel.setBounds (right.removeFromRight (36).withHeight (22).withY (right.getY() + 2));
    }

private:
    juce::AudioProcessorValueTreeState& apvtsRef;

    juce::Label nameLabel;
    juce::Label gainLabel, mixLabel, modeLabel;
    juce::TextButton freezeBtn;
    juce::ComboBox sourceModeBox;
    juce::Slider gainSlider, mixSlider;

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttach> gainAttach, mixAttach;
    std::unique_ptr<ButtonAttach> freezeAttach;
    std::unique_ptr<ComboAttach> sourceModeAttach;

    void onModeChanged()
    {
        bool isFile = (sourceModeBox.getSelectedId() == 2);

        if (isFile)
        {
            // Lock dry/wet to 100% in file mode
            if (auto* p = apvtsRef.getParameter ("mix"))
                p->setValueNotifyingHost (1.0f);
            mixSlider.setEnabled (false);
            mixSlider.setAlpha (0.4f);
        }
        else
        {
            mixSlider.setEnabled (true);
            mixSlider.setAlpha (1.0f);
        }
    }
};
