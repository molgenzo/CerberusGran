#pragma once
#include <JuceHeader.h>
#include "RotaryKnob.h"
#include "GrainShapeKnob.h"
#include "FXChainPanel.h"
#include "Modulation/ModulationEngine.h"

class EngineColumn : public juce::Component
{
public:
    static inline juce::Colour sectionPurple { 0xff8B5CF6 };

    EngineColumn (juce::AudioProcessorValueTreeState& apvts, int headIndex, juce::Colour accent,
                  ModulationEngine* modEngineIn = nullptr)
        : index (headIndex), accentColour (accent),
          fxPanel (apvts, headIndex, accent),
          modEngine (modEngineIn)
    {
        auto id = [headIndex] (const juce::String& name)
        { return "head" + juce::String (headIndex) + "_" + name; };

        // Cache modulatable param IDs keyed by card slot 0..7
        cardParamIds[0] = id ("position");
        cardParamIds[1] = id ("spread");
        cardParamIds[2] = id ("rate");
        cardParamIds[3] = id ("length");
        cardParamIds[4] = id ("pitch");
        cardParamIds[5] = id ("shape");
        cardParamIds[6] = id ("gain");
        cardParamIds[7] = id ("reverse");

        enableBtn.getProperties().set ("accentColour", (juce::int64) accent.getARGB());
        addAndMakeVisible (enableBtn);
        enableAttach = std::make_unique<ButtonAttach> (apvts, id ("enable"), enableBtn);

        freezeBtn.setButtonText ("Freeze");
        freezeBtn.setClickingTogglesState (true);
        freezeBtn.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2E2E34));
        freezeBtn.setColour (juce::TextButton::buttonOnColourId, accent);
        freezeBtn.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffaaaaaa));
        freezeBtn.setColour (juce::TextButton::textColourOnId, juce::Colour (0xff1A1A1E));
        addAndMakeVisible (freezeBtn);
        freezeAttach = std::make_unique<ButtonAttach> (apvts, id ("freeze"), freezeBtn);

        // Advanced button — sits in FX Chain label row
        advancedBtn.setButtonText ("Advanced");
        advancedBtn.setClickingTogglesState (true);
        advancedBtn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff2E2E34));
        advancedBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffd8d8dc));
        advancedBtn.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xffaaaaaa));
        advancedBtn.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xff1A1A1E));
        advancedBtn.onClick = [this] {
            if (onAdvancedToggled)
                onAdvancedToggled (advancedBtn.getToggleState());
        };
        addAndMakeVisible (advancedBtn);

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

        syncDivKnob = new RotaryKnob ("Div", "");
        syncDivKnob->setAccentColour (accent);
        syncDivKnob->getSlider().setRange (0, 8, 1);
        syncDivKnob->getSlider().textFromValueFunction = [] (double v) {
            static const char* l[] = {"1/1","1/2","1/4","1/8","1/16","1/32","1/64","1/128","1/256"};
            return juce::String (l[juce::jlimit (0, 8, (int)v)]);
        };
        syncDivKnob->getSlider().updateText();
        syncDivKnob->setVisible (false);
        addAndMakeVisible (syncDivKnob);
        syncDivAttach = std::make_unique<SliderAttach> (apvts, id ("rateSyncDiv"), syncDivKnob->getSlider());

        syncBtn.setButtonText ("Sync");
        syncBtn.setClickingTogglesState (true);
        syncBtn.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        syncBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
        syncBtn.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff999999));
        syncBtn.setColour (juce::TextButton::textColourOnId, juce::Colour (0xffeeeeee));
        syncBtn.onClick = [this, &apvts, id] {
            if (auto* p = apvts.getParameter (id ("rateMode")))
                p->setValueNotifyingHost (syncBtn.getToggleState() ? 1.0f : 0.0f);
            updateRateModeVisibility();
        };
        addAndMakeVisible (syncBtn);
        if (auto* p = apvts.getRawParameterValue (id ("rateMode")))
            syncBtn.setToggleState (p->load() >= 0.5f, juce::dontSendNotification);

        syncTypeBox.addItemList ({ "Norm", "Trip", "Dot" }, 1);
        syncTypeBox.setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
        syncTypeBox.setEnabled (false);
        syncTypeBox.setAlpha (0.3f);
        addAndMakeVisible (syncTypeBox);
        syncTypeAttach = std::make_unique<ComboAttach> (apvts, id ("rateSyncType"), syncTypeBox);

        sizeLinkBtn.setButtonText ("Link");
        sizeLinkBtn.setClickingTogglesState (true);
        sizeLinkBtn.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        sizeLinkBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
        sizeLinkBtn.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff999999));
        sizeLinkBtn.setColour (juce::TextButton::textColourOnId, juce::Colour (0xffeeeeee));
        sizeLinkBtn.onClick = [this] { updateSizeLinkVisibility(); };
        addAndMakeVisible (sizeLinkBtn);
        sizeLinkAttach = std::make_unique<ButtonAttach> (apvts, id ("sizeLink"), sizeLinkBtn);

        sizeRatioKnob = new RotaryKnob ("Ratio", "");
        sizeRatioKnob->setAccentColour (accent);
        sizeRatioKnob->getSlider().setRange (0, 6, 1);
        sizeRatioKnob->getSlider().textFromValueFunction = [] (double v) {
            static const char* l[] = {"1:4","1:2","1:1","2:1","4:1","8:1","16:1"};
            return juce::String (l[juce::jlimit (0, 6, (int)v)]);
        };
        sizeRatioKnob->getSlider().updateText();
        sizeRatioKnob->setVisible (false);
        addAndMakeVisible (sizeRatioKnob);
        sizeRatioAttach = std::make_unique<SliderAttach> (apvts, id ("sizeRatio"), sizeRatioKnob->getSlider());

        shapeKnob.setAccentColour (accent);
        addAndMakeVisible (shapeKnob);
        shapeAttach = std::make_unique<SliderAttach> (apvts, id ("shape"), shapeKnob.getSlider());

        addAndMakeVisible (fxPanel);

        enableAttachmentCb = std::make_unique<juce::ParameterAttachment> (
            *apvts.getParameter (id ("enable")),
            [this] (float v) { setAlpha (v >= 0.5f ? 1.0f : 0.35f); repaint(); },
            nullptr);
        enableAttachmentCb->sendInitialUpdate();

        updateRateModeVisibility();
        updateSizeLinkVisibility();
    }

    // Enable/disable assignment mode — disables knob interaction so EngineColumn gets the clicks
    void setAssignMode (int sourceIndex, bool active)
    {
        assignMode = active;
        assignSource = sourceIndex;

        // Disable knob click capture when in assign mode so we intercept drags
        auto toggle = [active] (juce::Component* c)
        { if (c) c->setInterceptsMouseClicks (!active, !active); };

        toggle (posKnob); toggle (spreadKnob);
        toggle (rateKnob); toggle (syncDivKnob);
        toggle (lenKnob);  toggle (sizeRatioKnob);
        toggle (pitchKnob); toggle (&shapeKnob);
        toggle (gainKnob);  toggle (reverseKnob);

        repaint();
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! assignMode || modEngine == nullptr) return;
        draggingCard = findCardAt (e.getPosition());
        if (draggingCard >= 0)
        {
            float current = modEngine->getConnectionAmount (assignSource, cardParamIds[draggingCard]);
            dragStartAmount = current;
            dragStartY = e.y;
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! assignMode || modEngine == nullptr || draggingCard < 0) return;

        // Drag up = positive amount, drag down = negative. 100 px = full range.
        float delta = (dragStartY - e.y) / 100.0f;
        float newAmount = juce::jlimit (-1.0f, 1.0f, dragStartAmount + delta);
        modEngine->addOrUpdateConnection (assignSource, cardParamIds[draggingCard], newAmount);
        repaint();
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (assignMode && draggingCard >= 0)
        {
            // If amount snapped to ~zero, remove connection
            float a = modEngine->getConnectionAmount (assignSource, cardParamIds[draggingCard]);
            if (std::abs (a) < 0.01f)
                modEngine->removeConnection (assignSource, cardParamIds[draggingCard]);
            draggingCard = -1;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        // Section labels
        g.setColour (juce::Colour (0xffcccccc));
        g.setFont (juce::Font ("Avenir", 13.0f, juce::Font::bold));
        g.drawText ("Grain Control", grainLabelBounds, juce::Justification::centredLeft);
        g.drawText ("FX Chain", fxLabelBounds, juce::Justification::centredLeft);

        // Main section panels — per-head accent colour
        g.setColour (accentColour.withAlpha (0.30f));
        g.fillRoundedRectangle (grainPanelBounds.toFloat(), 16.0f);
        g.fillRoundedRectangle (fxPanelBounds.toFloat(), 16.0f);

        // Knob card backgrounds
        g.setColour (accentColour.withAlpha (0.22f));
        for (auto& r : knobCardBounds)
            g.fillRoundedRectangle (r.toFloat(), 10.0f);
    }

    void paintOverChildren (juce::Graphics& g) override
    {
        if (modEngine == nullptr) return;

        // Draw modulation rings around each knob whenever a connection exists,
        // plus an outline on all knobs when in assign mode.
        for (size_t i = 0; i < knobCardBounds.size() && i < cardParamIds.size(); ++i)
        {
            auto card = knobCardBounds[i].toFloat();
            // Ring circle roughly matches the knob's rendered circle — upper portion of card
            auto conns = modEngine->getConnectionsForParam (cardParamIds[i]);

            // Ring geometry — upper portion of card
            float radius = juce::jmin (card.getWidth(), card.getHeight() - 30.0f) * 0.5f;
            float cx = card.getCentreX();
            float cy = card.getY() + radius + 4.0f;

            if (assignMode)
            {
                // Dashed light outline on all knobs to cue "assignable"
                juce::Colour col (0xff8B5CF6);
                g.setColour (col.withAlpha (0.35f));
                g.drawEllipse (cx - radius - 2.0f, cy - radius - 2.0f,
                               (radius + 2.0f) * 2.0f, (radius + 2.0f) * 2.0f, 1.0f);
            }

            // Existing connection rings — draw an arc proportional to amount per source
            float ringRadius = radius + 3.0f;
            for (const auto& c : conns)
            {
                juce::Colour srcCol = (c.sourceIndex == ModulationEngine::kLFO)
                    ? juce::Colour (0xff8B5CF6) // purple for LFO
                    : juce::Colour (0xff2DD4BF); // teal for Seq
                float amt = juce::jlimit (-1.0f, 1.0f, c.amount);
                float startAngle = juce::MathConstants<float>::pi * 1.5f; // 12 o'clock
                float sweep = amt * juce::MathConstants<float>::twoPi * 0.9f;

                juce::Path arc;
                arc.addCentredArc (cx, cy, ringRadius, ringRadius, 0.0f,
                                   startAngle, startAngle + sweep, true);
                g.setColour (srcCol.withAlpha (assignMode ? 0.9f : 0.6f));
                g.strokePath (arc, juce::PathStrokeType (assignMode ? 3.0f : 2.0f));
                ringRadius += 3.5f; // stack multiple connections
            }
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (6, 4);

        int gap = 8;
        int grainW = static_cast<int> (area.getWidth() * 0.50f);
        auto grainHalf = area.removeFromLeft (grainW);
        area.removeFromLeft (gap);
        auto fxHalf = area;

        // Label row
        auto grainLabelRow = grainHalf.removeFromTop (22);
        auto fxLabelRow = fxHalf.removeFromTop (22);
        grainHalf.removeFromTop (4);
        fxHalf.removeFromTop (4);

        // FX label area: "FX Chain" on left + Advanced button on right
        fxLabelBounds = fxLabelRow;
        advancedBtn.setBounds (fxLabelRow.removeFromRight (80).withHeight (20).withY (fxLabelRow.getY()));

        grainLabelBounds = grainLabelRow.removeFromLeft (100);
        enableBtn.setBounds (grainLabelRow.removeFromRight (44).withHeight (20).withY (grainLabelRow.getY()));
        freezeBtn.setBounds (grainLabelRow.removeFromRight (55).withHeight (20).withY (grainLabelRow.getY()));

        grainPanelBounds = grainHalf;
        fxPanelBounds = fxHalf;

        // === Grain Control: 4x2 grid of knob cards ===
        auto gp = grainPanelBounds.reduced (6, 4);
        bool isSync = syncBtn.getToggleState();
        bool isLinked = sizeLinkBtn.getToggleState();

        int cols = 4, rows = 2;
        int ctrlH = 18; // Sync/Note/Link row height
        int cardGap = 4; // gap between cards

        // Available for cards after ctrl row
        int availW = gp.getWidth();
        int availH = gp.getHeight() - ctrlH - 2; // 2px gap below ctrl row
        int cardW = (availW - cardGap * (cols - 1)) / cols;
        int cardH = (availH - cardGap * (rows - 1)) / rows;

        knobCardBounds.clear();

        auto cardRect = [&] (int col, int row) -> juce::Rectangle<int>
        {
            int x = gp.getX() + col * (cardW + cardGap);
            int y = gp.getY() + ctrlH + 2 + row * (cardH + cardGap);
            return { x, y, cardW, cardH };
        };

        // Sync/Note/Link row — above the grid, spanning cols 2-3
        {
            // Use the full width of cols 2+3 for Sync + NoteType + Link
            int ctrlX = gp.getX() + 2 * (cardW + cardGap);
            int ctrlY = gp.getY();
            int totalW = cardW * 2 + cardGap; // both columns combined
            int syncW = totalW * 25 / 100;  // "Sync"
            int noteW = totalW * 35 / 100;  // "Norm ▼"
            int linkW = totalW - syncW - noteW; // "Link"
            syncBtn.setBounds (ctrlX, ctrlY, syncW, ctrlH);
            syncTypeBox.setBounds (ctrlX + syncW, ctrlY, noteW, ctrlH);
            sizeLinkBtn.setBounds (ctrlX + syncW + noteW, ctrlY, linkW, ctrlH);
        }

        // Row 0 cards
        auto c00 = cardRect (0, 0);  knobCardBounds.push_back (c00);
        auto c01 = cardRect (1, 0);  knobCardBounds.push_back (c01);
        auto c02 = cardRect (2, 0);  knobCardBounds.push_back (c02);
        auto c03 = cardRect (3, 0);  knobCardBounds.push_back (c03);

        posKnob->setBounds (c00);
        spreadKnob->setBounds (c01);
        (isSync ? syncDivKnob : rateKnob)->setBounds (c02);
        (isLinked ? sizeRatioKnob : lenKnob)->setBounds (c03);

        // Row 1 cards
        auto c10 = cardRect (0, 1);  knobCardBounds.push_back (c10);
        auto c11 = cardRect (1, 1);  knobCardBounds.push_back (c11);
        auto c12 = cardRect (2, 1);  knobCardBounds.push_back (c12);
        auto c13 = cardRect (3, 1);  knobCardBounds.push_back (c13);

        pitchKnob->setBounds (c10);
        shapeKnob.setBounds (c11);
        gainKnob->setBounds (c12);
        reverseKnob->setBounds (c13);

        // === FX Chain Panel ===
        fxPanel.setBounds (fxPanelBounds.reduced (8));
    }

private:
    int index;
    juce::Colour accentColour;
    juce::Rectangle<int> grainLabelBounds, fxLabelBounds;
    juce::Rectangle<int> grainPanelBounds, fxPanelBounds;
    std::vector<juce::Rectangle<int>> knobCardBounds;
    std::array<juce::String, 8> cardParamIds;

    ModulationEngine* modEngine = nullptr;
    bool assignMode = false;
    int  assignSource = 0;
    int  draggingCard = -1;
    float dragStartAmount = 0.0f;
    int  dragStartY = 0;

    int findCardAt (juce::Point<int> p) const
    {
        for (size_t i = 0; i < knobCardBounds.size(); ++i)
            if (knobCardBounds[i].contains (p))
                return static_cast<int> (i);
        return -1;
    }

    juce::ToggleButton enableBtn;
    juce::TextButton freezeBtn;

public:
    juce::TextButton advancedBtn;
    std::function<void (bool)> onAdvancedToggled;

private:

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
    juce::TextButton syncBtn;
    juce::ComboBox syncTypeBox;
    juce::TextButton sizeLinkBtn;

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ButtonAttach> enableAttach, sizeLinkAttach, freezeAttach;
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
