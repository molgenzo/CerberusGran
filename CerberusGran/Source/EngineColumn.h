#pragma once
#include <JuceHeader.h>
#include "RotaryKnob.h"
#include "GrainShapeKnob.h"
#include "FXChainPanel.h"

// Chain link icon button — draws two interlocking links
class ChainLinkButton : public juce::ToggleButton
{
public:
    juce::Colour accentColour { 0xff8B5CF6 };

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override
    {
        juce::ignoreUnused (highlighted, down);

        auto b = getLocalBounds().toFloat().reduced (2.0f);
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.35f;

        juce::Colour col = getToggleState() ? accentColour : juce::Colour (0xff555555);
        g.setColour (col);

        // Two overlapping rounded rects to form a chain link
        float linkW = r * 1.6f;
        float linkH = r * 0.8f;
        float offset = r * 0.4f;

        g.drawRoundedRectangle (cx - linkW * 0.5f - offset, cy - linkH * 0.5f, linkW, linkH, linkH * 0.4f, 1.5f);
        g.drawRoundedRectangle (cx - linkW * 0.5f + offset, cy - linkH * 0.5f, linkW, linkH, linkH * 0.4f, 1.5f);
    }
};

class EngineColumn : public juce::Component
{
public:
    EngineColumn (juce::AudioProcessorValueTreeState& apvts, int headIndex, juce::Colour accent)
        : index (headIndex), accentColour (accent),
          fxPanel (apvts, headIndex, accent)
    {
        auto id = [headIndex] (const juce::String& name)
        { return "head" + juce::String (headIndex) + "_" + name; };

        enableBtn.getProperties().set ("accentColour", (juce::int64) accent.getARGB());
        addAndMakeVisible (enableBtn);
        enableAttach = std::make_unique<ButtonAttach> (apvts, id ("enable"), enableBtn);

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
        pitchKnob   = makeKnob ("Pitch",   "st", "pitch");
        gainKnob    = makeKnob ("Gain",    "dB", "gain");
        reverseKnob = makeKnob ("Reverse", "%",  "reverse");

        // Sync division knob
        syncDivKnob = new RotaryKnob ("Div", "");
        syncDivKnob->setAccentColour (accent);
        syncDivKnob->getSlider().setRange (0, 8, 1);
        syncDivKnob->getSlider().textFromValueFunction = [] (double v)
        {
            static const char* labels[] = { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64", "1/128", "1/256" };
            return juce::String (labels[juce::jlimit (0, 8, static_cast<int> (v))]);
        };
        syncDivKnob->getSlider().updateText();
        syncDivKnob->setVisible (false);
        addAndMakeVisible (syncDivKnob);
        syncDivAttach = std::make_unique<SliderAttach> (apvts, id ("rateSyncDiv"), syncDivKnob->getSlider());

        // Sync toggle button (replaces dropdown)
        syncBtn.setButtonText ("Sync");
        syncBtn.setClickingTogglesState (true);
        syncBtn.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        syncBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
        syncBtn.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff555555));
        syncBtn.setColour (juce::TextButton::textColourOnId, accent);
        syncBtn.onClick = [this, &apvts, id]
        {
            bool on = syncBtn.getToggleState();
            if (auto* p = apvts.getParameter (id ("rateMode")))
                p->setValueNotifyingHost (on ? 1.0f : 0.0f);
            updateRateModeVisibility();
        };
        addAndMakeVisible (syncBtn);

        // Read initial state
        if (auto* p = apvts.getRawParameterValue (id ("rateMode")))
            syncBtn.setToggleState (p->load() >= 0.5f, juce::dontSendNotification);

        // Sync type (Norm / Trip / Dot) — only visible when sync is on
        syncTypeBox.addItemList ({ "Norm", "Trip", "Dot" }, 1);
        syncTypeBox.setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
        syncTypeBox.setEnabled (false);
        syncTypeBox.setAlpha (0.3f);
        addAndMakeVisible (syncTypeBox);
        syncTypeAttach = std::make_unique<ComboAttach> (apvts, id ("rateSyncType"), syncTypeBox);

        // Link text button
        sizeLinkBtn.setButtonText ("Link");
        sizeLinkBtn.setClickingTogglesState (true);
        sizeLinkBtn.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        sizeLinkBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
        sizeLinkBtn.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff555555));
        sizeLinkBtn.setColour (juce::TextButton::textColourOnId, accent);
        sizeLinkBtn.onClick = [this] { updateSizeLinkVisibility(); };
        addAndMakeVisible (sizeLinkBtn);
        sizeLinkAttach = std::make_unique<ButtonAttach> (apvts, id ("sizeLink"), sizeLinkBtn);

        // Size ratio knob
        sizeRatioKnob = new RotaryKnob ("Ratio", "");
        sizeRatioKnob->setAccentColour (accent);
        sizeRatioKnob->getSlider().setRange (0, 6, 1);
        sizeRatioKnob->getSlider().textFromValueFunction = [] (double v)
        {
            static const char* labels[] = { "1:4", "1:2", "1:1", "2:1", "4:1", "8:1", "16:1" };
            return juce::String (labels[juce::jlimit (0, 6, static_cast<int> (v))]);
        };
        sizeRatioKnob->getSlider().updateText();
        sizeRatioKnob->setVisible (false);
        addAndMakeVisible (sizeRatioKnob);
        sizeRatioAttach = std::make_unique<SliderAttach> (apvts, id ("sizeRatio"), sizeRatioKnob->getSlider());

        // Grain shape knob
        shapeKnob.setAccentColour (accent);
        addAndMakeVisible (shapeKnob);
        shapeAttach = std::make_unique<SliderAttach> (apvts, id ("shape"), shapeKnob.getSlider());

        // FX panel
        fxViewport.setViewedComponent (&fxPanel, false);
        fxViewport.setScrollBarsShown (true, false);
        addAndMakeVisible (fxViewport);

        // Dim when disabled
        enableAttachmentCb = std::make_unique<juce::ParameterAttachment> (
            *apvts.getParameter (id ("enable")),
            [this] (float v) { setAlpha (v >= 0.5f ? 1.0f : 0.35f); repaint(); },
            nullptr);
        enableAttachmentCb->sendInitialUpdate();

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

        g.setColour (accentColour);
        g.fillEllipse (8.0f, 8.0f, 22.0f, 22.0f);
        g.setColour (juce::Colour (0xff1A1A1E));
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (juce::String (index + 1), 8, 8, 22, 22, juce::Justification::centred);

        g.setColour (juce::Colour (0xff888888));
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawText ("GRAIN", 6, 36, getWidth() - 12, 14, juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (8);
        int halfW = area.getWidth() / 2;
        int knobH = 78;
        int pad = 10; // vertical padding between rows

        // Badge + enable
        auto badgeRow = area.removeFromTop (30);
        enableBtn.setBounds (badgeRow.removeFromRight (44).withHeight (22).withY (badgeRow.getY() + 4));
        area.removeFromTop (16); // GRAIN label

        // Helper: place two components in a row, each in its half with equal inset
        int inset = 10;
        auto placeRow = [&] (juce::Component* left, juce::Component* right)
        {
            auto row = area.removeFromTop (knobH);
            auto leftArea = juce::Rectangle<int> (row.getX() + inset, row.getY(), halfW - inset * 2, knobH);
            auto rightArea = juce::Rectangle<int> (row.getX() + halfW + inset, row.getY(), halfW - inset * 2, knobH);
            if (left)  left->setBounds (leftArea);
            if (right) right->setBounds (rightArea);
            area.removeFromTop (pad);
        };

        // Row 1: Pos | Spread
        placeRow (posKnob, spreadKnob);

        // Sync + note type on LEFT half, Link on RIGHT half — same row
        bool isSync = syncBtn.getToggleState();
        auto modeRow = area.removeFromTop (20);
        auto modeLeft = modeRow.removeFromLeft (halfW);
        int modeHalf = modeLeft.getWidth() / 2;
        syncBtn.setBounds (modeLeft.removeFromLeft (modeHalf).reduced (4, 0));
        syncTypeBox.setBounds (modeLeft.reduced (4, 0));
        // Link on the right half, centered
        sizeLinkBtn.setBounds (juce::Rectangle<int> (modeRow.getX() + inset, modeRow.getY(), halfW - inset * 2, 20));
        area.removeFromTop (2);

        // Row 2: Rate (or Div) | Len (or Ratio) — same centering as all rows
        {
            auto row = area.removeFromTop (knobH);
            auto leftArea = juce::Rectangle<int> (row.getX() + inset, row.getY(), halfW - inset * 2, knobH);
            auto rightArea = juce::Rectangle<int> (row.getX() + halfW + inset, row.getY(), halfW - inset * 2, knobH);

            if (isSync)
                syncDivKnob->setBounds (leftArea);
            else
                rateKnob->setBounds (leftArea);

            if (sizeLinkBtn.getToggleState())
                sizeRatioKnob->setBounds (rightArea);
            else
                lenKnob->setBounds (rightArea);

            area.removeFromTop (pad);
        }

        // Row 3: Pitch | Shape — same centering
        {
            auto row = area.removeFromTop (knobH);
            auto leftArea = juce::Rectangle<int> (row.getX() + inset, row.getY(), halfW - inset * 2, knobH);
            auto rightArea = juce::Rectangle<int> (row.getX() + halfW + inset, row.getY(), halfW - inset * 2, knobH);
            pitchKnob->setBounds (leftArea);
            shapeKnob.setBounds (rightArea);
            area.removeFromTop (pad);
        }

        // Row 4: Gain | Reverse
        placeRow (gainKnob, reverseKnob);

        // Spacing before FX chain
        area.removeFromTop (6);

        // FX chain
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
    RotaryKnob* syncDivKnob = nullptr;
    RotaryKnob* sizeRatioKnob = nullptr;

    GrainShapeKnob shapeKnob;
    FXChainPanel fxPanel;
    juce::Viewport fxViewport;

    juce::TextButton syncBtn;
    juce::ComboBox syncTypeBox;
    juce::TextButton sizeLinkBtn;

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ButtonAttach> enableAttach, sizeLinkAttach;
    std::unique_ptr<SliderAttach> shapeAttach, syncDivAttach, sizeRatioAttach;
    std::unique_ptr<ComboAttach> syncTypeAttach;
    juce::OwnedArray<SliderAttach> knobAttachments;
    std::unique_ptr<juce::ParameterAttachment> enableAttachmentCb;

    void updateRateModeVisibility()
    {
        bool isSync = syncBtn.getToggleState();
        rateKnob->setVisible (!isSync);
        syncDivKnob->setVisible (isSync);
        syncTypeBox.setEnabled (isSync);
        syncTypeBox.setAlpha (isSync ? 1.0f : 0.3f);
        resized();
    }

    void updateSizeLinkVisibility()
    {
        bool linked = sizeLinkBtn.getToggleState();
        lenKnob->setVisible (!linked);
        sizeRatioKnob->setVisible (linked);
        resized();
    }
};
