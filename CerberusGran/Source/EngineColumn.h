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
        pitchKnob   = makeKnob ("Pitch",   "st", "pitch");
        gainKnob    = makeKnob ("Gain",    "dB", "gain");
        reverseKnob = makeKnob ("Reverse", "%",  "reverse");

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
        rateKnob->setBounds (row2.removeFromLeft (halfW));
        lenKnob->setBounds (row2);

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

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ButtonAttach> enableAttach;
    std::unique_ptr<SliderAttach> shapeAttach;
    juce::OwnedArray<SliderAttach> knobAttachments;
    std::unique_ptr<juce::ParameterAttachment> enableAttachmentCb;
};
