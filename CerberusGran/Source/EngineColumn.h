#pragma once
#include <JuceHeader.h>
#include "RotaryKnob.h"
#include "GrainShapeKnob.h"
#include "FXChainPanel.h"

class EngineColumn : public juce::Component
{
public:
    EngineColumn (juce::AudioProcessorValueTreeState& apvts, int headIndex, juce::Colour accent)
        : index (headIndex), accentColour (accent),
          fxPanel (apvts, headIndex, accent)
    {
        auto id = [headIndex] (const juce::String& name)
        { return "head" + juce::String (headIndex) + "_" + name; };

        // Enable toggle
        enableBtn.getProperties().set ("accentColour", (juce::int64) accent.getARGB());
        addAndMakeVisible (enableBtn);
        enableAttach = std::make_unique<ButtonAttach> (apvts, id ("enable"), enableBtn);

        // Knobs
        auto makeKnob = [&] (const juce::String& label, const juce::String& suffix,
                             const juce::String& paramName) -> RotaryKnob*
        {
            auto* k = new RotaryKnob (label, suffix);
            k->setAccentColour (accent);
            addAndMakeVisible (k);
            knobAttachments.add (new SliderAttach (apvts, id (paramName), k->getSlider()));
            return k;
        };

        posKnob     = makeKnob ("Pos",     "%",  "position");
        spreadKnob  = makeKnob ("Spread",  "",   "spread");
        rateKnob    = makeKnob ("Rate",    "ms", "rate");
        lenKnob     = makeKnob ("Len",     "ms", "length");

        // Chainlink toggle (links Size to Rate as a ratio)
        sizeLinkBtn.setButtonText ("Link");
        sizeLinkBtn.setClickingTogglesState (true);
        sizeLinkBtn.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2A2A30));
        sizeLinkBtn.setColour (juce::TextButton::buttonOnColourId, accent.withAlpha (0.6f));
        sizeLinkBtn.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff888888));
        sizeLinkBtn.setColour (juce::TextButton::textColourOnId, juce::Colour (0xffeeeeee));
        sizeLinkBtn.onClick = [this] { updateSizeLinkVisibility(); };
        addAndMakeVisible (sizeLinkBtn);
        sizeLinkAttach = std::make_unique<ButtonAttach> (apvts, id ("sizeLink"), sizeLinkBtn);

        // Size ratio selector (visible when linked)
        sizeRatioBox.addItemList ({ "1:4", "1:2", "1:1", "2:1", "4:1", "8:1", "16:1" }, 1);
        sizeRatioBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2A2A30));
        sizeRatioBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff3A3A40));
        sizeRatioBox.setVisible (false);
        addAndMakeVisible (sizeRatioBox);
        sizeRatioAttach = std::make_unique<ComboAttach> (apvts, id ("sizeRatio"), sizeRatioBox);
        pitchKnob   = makeKnob ("Pitch",   "st", "pitch");
        gainKnob    = makeKnob ("Gain",    "dB", "gain");
        reverseKnob = makeKnob ("Reverse", "%",  "reverse");

        // Rate mode toggle (Time / Sync)
        rateModeBox.addItemList ({ "Time", "Sync" }, 1);
        rateModeBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2A2A30));
        rateModeBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff3A3A40));
        rateModeBox.onChange = [this] { updateRateModeVisibility(); };
        addAndMakeVisible (rateModeBox);
        rateModeAttach = std::make_unique<ComboAttach> (apvts, id ("rateMode"), rateModeBox);

        // Sync division selector
        syncDivBox.addItemList ({ "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64", "1/128", "1/256" }, 1);
        syncDivBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2A2A30));
        syncDivBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff3A3A40));
        syncDivBox.setVisible (false);
        addAndMakeVisible (syncDivBox);
        syncDivAttach = std::make_unique<ComboAttach> (apvts, id ("rateSyncDiv"), syncDivBox);

        // Sync type selector (Normal / Triplet / Dotted)
        syncTypeBox.addItemList ({ "Norm", "Trip", "Dot" }, 1);
        syncTypeBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2A2A30));
        syncTypeBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff3A3A40));
        syncTypeBox.setVisible (false);
        addAndMakeVisible (syncTypeBox);
        syncTypeAttach = std::make_unique<ComboAttach> (apvts, id ("rateSyncType"), syncTypeBox);

        // Grain shape knob (continuous morph)
        shapeKnob.setAccentColour (accent);
        addAndMakeVisible (shapeKnob);
        shapeAttach = std::make_unique<SliderAttach> (apvts, id ("shape"), shapeKnob.getSlider());

        // FX panel in viewport for scrolling
        fxViewport.setViewedComponent (&fxPanel, false);
        fxViewport.setScrollBarsShown (true, false);
        addAndMakeVisible (fxViewport);

        // Listen for enable to dim
        enableAttachmentCb = std::make_unique<juce::ParameterAttachment> (
            *apvts.getParameter (id ("enable")),
            [this] (float v) { setAlpha (v >= 0.5f ? 1.0f : 0.35f); repaint(); },
            nullptr);
        enableAttachmentCb->sendInitialUpdate();

        // Initial visibility
        updateRateModeVisibility();
        updateSizeLinkVisibility();
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff1E1E22));
        g.fillRoundedRectangle (b, 6.0f);
        g.setColour (accentColour.withAlpha (0.3f));
        g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 0.5f);

        // Badge circle with number
        g.setColour (accentColour);
        g.fillEllipse (8.0f, 8.0f, 22.0f, 22.0f);
        g.setColour (juce::Colour (0xff1A1A1E));
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (juce::String (index + 1), 8, 8, 22, 22, juce::Justification::centred);

        // "GRAIN" label
        g.setColour (juce::Colour (0xff888888));
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawText ("GRAIN", 6, 36, getWidth() - 12, 14, juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (6);

        // Badge row
        auto badgeRow = area.removeFromTop (30);
        enableBtn.setBounds (badgeRow.removeFromRight (44).withHeight (22).withY (badgeRow.getY() + 4));

        area.removeFromTop (22); // "GRAIN" label space

        // Knob grid: 2 columns x 4 rows
        int knobH = 80;
        int halfW = area.getWidth() / 2;

        auto row1 = area.removeFromTop (knobH);
        posKnob->setBounds (row1.removeFromLeft (halfW));
        spreadKnob->setBounds (row1);

        auto row2 = area.removeFromTop (knobH);
        auto rateArea = row2.removeFromLeft (halfW);

        // Rate mode selector at top of rate area
        rateModeBox.setBounds (rateArea.removeFromTop (20).reduced (2, 0));

        if (rateModeBox.getSelectedId() == 2) // Sync
        {
            // Show division and type dropdowns
            syncDivBox.setBounds (rateArea.removeFromTop (22).reduced (2, 1));
            syncTypeBox.setBounds (rateArea.removeFromTop (22).reduced (2, 1));
        }
        else
        {
            // Show rate knob
            rateKnob->setBounds (rateArea);
        }

        // Length area with chainlink
        auto lenArea = row2;
        sizeLinkBtn.setBounds (lenArea.removeFromTop (18).reduced (2, 0));

        if (sizeLinkBtn.getToggleState())
        {
            // Linked: show ratio selector instead of length knob
            sizeRatioBox.setBounds (lenArea.removeFromTop (24).reduced (2, 1));
        }
        else
        {
            lenKnob->setBounds (lenArea);
        }

        auto row3 = area.removeFromTop (knobH);
        pitchKnob->setBounds (row3.removeFromLeft (halfW));
        gainKnob->setBounds (row3);

        auto row4 = area.removeFromTop (knobH);
        reverseKnob->setBounds (row4.removeFromLeft (halfW));

        // Spacing before grain shape
        area.removeFromTop (8);

        // Grain shape knob (centered, taller for the preview)
        shapeKnob.setBounds (area.removeFromTop (80));

        // Spacing before FX chain
        area.removeFromTop (10);

        // FX chain panel fills remaining space
        fxViewport.setBounds (area);
        fxPanel.setSize (area.getWidth() - 10, fxPanel.getIdealHeight());
    }

private:
    int index;
    juce::Colour accentColour;

    juce::ToggleButton enableBtn;

    RotaryKnob* posKnob = nullptr;
    RotaryKnob* spreadKnob = nullptr;
    RotaryKnob* rateKnob = nullptr;
    RotaryKnob* lenKnob = nullptr;
    RotaryKnob* pitchKnob = nullptr;
    RotaryKnob* gainKnob = nullptr;
    RotaryKnob* reverseKnob = nullptr;

    GrainShapeKnob shapeKnob;
    FXChainPanel fxPanel;
    juce::Viewport fxViewport;

    // Rate sync controls
    juce::ComboBox rateModeBox, syncDivBox, syncTypeBox;

    // Chainlink controls
    juce::TextButton sizeLinkBtn;
    juce::ComboBox sizeRatioBox;

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ButtonAttach> enableAttach;
    std::unique_ptr<SliderAttach> shapeAttach;
    std::unique_ptr<ComboAttach> rateModeAttach, syncDivAttach, syncTypeAttach, sizeRatioAttach;
    std::unique_ptr<ButtonAttach> sizeLinkAttach;
    juce::OwnedArray<SliderAttach> knobAttachments;
    std::unique_ptr<juce::ParameterAttachment> enableAttachmentCb;

    void updateRateModeVisibility()
    {
        bool isSync = (rateModeBox.getSelectedId() == 2);
        rateKnob->setVisible (!isSync);
        syncDivBox.setVisible (isSync);
        syncTypeBox.setVisible (isSync);
        resized();
    }

    void updateSizeLinkVisibility()
    {
        bool linked = sizeLinkBtn.getToggleState();
        lenKnob->setVisible (!linked);
        sizeRatioBox.setVisible (linked);
        resized();
    }
};
