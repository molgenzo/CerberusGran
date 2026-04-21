#pragma once
#include <JuceHeader.h>
#include <set>
#include "RotaryKnob.h"
#include "FilterResponseDisplay.h"

enum class FXType { None = -1, Filter = 0, Bitcrush = 1, Delay = 2, Reverb = 3 };

// Circular toggle dot — fills solid green when active
class ActiveDot : public juce::Component
{
public:
    std::function<void()> onClick;
    bool active = false;
    juce::Colour accentColour { 0xff4CAF50 };

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (3.0f);
        float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
        float cx = b.getCentreX(), cy = b.getCentreY();

        g.setColour (active ? accentColour : juce::Colour (0xff555555));
        g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        if (onClick) onClick();
    }
};

class FXSlotCard : public juce::Component
{
public:
    FXSlotCard (juce::AudioProcessorValueTreeState& apvts, int headIndex, juce::Colour accent)
        : apvtsRef (apvts), head (headIndex), accentColour (accent)
    {
        fxSelector.addItem ("None", 1);
        fxSelector.addItem ("Filter", 2);
        fxSelector.addItem ("Bitcrush", 3);
        fxSelector.addItem ("Delay", 4);
        fxSelector.addItem ("Reverb", 5);
        fxSelector.setSelectedId (1, juce::dontSendNotification);
        fxSelector.setTextWhenNothingSelected ("+ add FX");
        fxSelector.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2A2A30));
        fxSelector.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
        fxSelector.setColour (juce::ComboBox::textColourId, juce::Colour (0xffcccccc));
        fxSelector.onChange = [this] { onFxTypeChanged(); };
        addAndMakeVisible (fxSelector);

        chevronBtn.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        chevronBtn.setColour (juce::TextButton::textColourOffId, juce::Colours::transparentBlack);
        chevronBtn.onClick = [this] { setCollapsed (!collapsed); };
        addAndMakeVisible (chevronBtn);

        activeDot.accentColour = accent;
        addAndMakeVisible (activeDot);
        activeDot.onClick = [this] { toggleActive(); };

        createFilterKnobs();
        createCrushKnobs();
        createDelayKnobs();
        createReverbKnobs();

        hideAllKnobs();
    }

    void setCollapsed (bool c)
    {
        collapsed = c;
        updateKnobVisibility();

        if (auto* parent = getParentComponent())
        {
            parent->resized();
            if (auto* grandparent = parent->getParentComponent())
                grandparent->resized();
        }
    }

    bool isCollapsed() const { return collapsed; }
    FXType getSelectedType() const { return selectedType; }

    std::function<void()> onSelectionChanged;

    void setDisabledEffects (const std::set<FXType>& usedByOthers)
    {
        for (int i = 0; i < fxSelector.getNumItems(); ++i)
        {
            int itemId = fxSelector.getItemId (i);
            if (itemId <= 1) continue;
            FXType fx = static_cast<FXType> (itemId - 2);
            bool disabled = usedByOthers.count (fx) > 0;
            fxSelector.setItemEnabled (itemId, !disabled);
        }
    }

    int getIdealHeight() const
    {
        if (selectedType == FXType::None) return kHeaderHeight;
        if (collapsed) return kHeaderHeight;

        int bodyH = (selectedType == FXType::Filter) ? kFilterBodyHeight : kBodyHeight;
        return kHeaderHeight + bodyH;
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff252528));
        g.fillRoundedRectangle (b, 4.0f);
        g.setColour (juce::Colour (0xff3A3A40));
        g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 0.5f);

        if (selectedType != FXType::None)
        {
            float cx = static_cast<float> (getWidth()) - 16.0f;
            float cy = kHeaderHeight * 0.5f;

            juce::Path tri;
            if (collapsed)
                tri.addTriangle (cx - 5.0f, cy - 3.0f, cx + 5.0f, cy - 3.0f, cx, cy + 4.0f);
            else
                tri.addTriangle (cx - 5.0f, cy + 3.0f, cx + 5.0f, cy + 3.0f, cx, cy - 4.0f);
            g.setColour (juce::Colour (0xff888888));
            g.fillPath (tri);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds();
        auto header = area.removeFromTop (kHeaderHeight);

        activeDot.setBounds (header.removeFromLeft (28));
        chevronBtn.setBounds (header.removeFromRight (28));
        fxSelector.setBounds (header.reduced (2, 2));

        if (collapsed || selectedType == FXType::None) return;

        auto body = area.reduced (4, 2);

        switch (selectedType)
        {
            case FXType::Filter:
            {
                // Row 0 (22px): filter type button (full width)
                // Row 1 (flex) : frequency response display
                // Row 2 (106px): 6 knobs in a 3x2 grid
                auto typeRow = body.removeFromTop (22);
                filterTypeButton.setBounds (typeRow.reduced (1, 1));
                body.removeFromTop (3);

                const int knobRowH = 52;
                const int knobBlock = knobRowH * 2 + 2;
                auto knobArea = body.removeFromBottom (knobBlock);

                body.removeFromBottom (3);
                if (filterResponseDisplay)
                    filterResponseDisplay->setBounds (body);

                auto row1 = knobArea.removeFromTop (knobRowH);
                knobArea.removeFromTop (2);
                auto row2 = knobArea;

                int colW1 = row1.getWidth() / 3;
                filterKnobs[0]->setBounds (row1.removeFromLeft (colW1));   // Cutoff
                filterKnobs[1]->setBounds (row1.removeFromLeft (colW1));   // Res
                filterKnobs[2]->setBounds (row1);                          // Pan

                int colW2 = row2.getWidth() / 3;
                filterKnobs[3]->setBounds (row2.removeFromLeft (colW2));   // Drive
                filterKnobs[4]->setBounds (row2.removeFromLeft (colW2));   // CombFrq
                filterKnobs[5]->setBounds (row2);                          // Mix
                break;
            }
            case FXType::Bitcrush:
            {
                int knobW = body.getWidth() / 2;
                crushKnobs[0]->setBounds (body.removeFromLeft (knobW));
                crushKnobs[1]->setBounds (body);
                break;
            }
            case FXType::Delay:
            {
                bool delaySync = (delayTimeModeBox.getSelectedId() == 2);
                auto modeRow = body.removeFromTop (20);
                auto modeLeft = modeRow.removeFromLeft (modeRow.getWidth() / 2);
                delayTimeModeBox.setBounds (modeLeft.reduced (1, 0));
                if (delaySync)
                    delaySyncTypeBox.setBounds (modeRow.reduced (1, 0));

                body.removeFromTop (2);

                int knobW = body.getWidth() / 3;
                if (delaySync)
                    delaySyncDivKnob->setBounds (body.removeFromLeft (knobW));
                else
                    delayKnobs[0]->setBounds (body.removeFromLeft (knobW));
                delayKnobs[1]->setBounds (body.removeFromLeft (knobW));
                delayKnobs[2]->setBounds (body);
                break;
            }
            case FXType::Reverb:
            {
                int knobW = body.getWidth() / 3;
                reverbKnobs[0]->setBounds (body.removeFromLeft (knobW));
                reverbKnobs[1]->setBounds (body.removeFromLeft (knobW));
                reverbKnobs[2]->setBounds (body);
                break;
            }
            default: break;
        }
    }

private:
    static constexpr int kHeaderHeight     = 30;
    static constexpr int kBodyHeight       = 96;
    static constexpr int kFilterBodyHeight = 210;

    juce::AudioProcessorValueTreeState& apvtsRef;
    int head;
    juce::Colour accentColour;
    FXType selectedType = FXType::None;
    bool collapsed = true;
    bool isActive = false;

    juce::ComboBox fxSelector;
    juce::TextButton chevronBtn;
    ActiveDot activeDot;

    // Filter: 6 knobs + custom type button (avoids JUCE ComboBoxAttachment
    // off-by-one bug when using addSectionHeading)
    std::array<RotaryKnob*, 6> filterKnobs {};
    juce::TextButton filterTypeButton;
    std::unique_ptr<juce::ParameterAttachment> filterTypeAttach;
    std::unique_ptr<FilterResponseDisplay> filterResponseDisplay;
    int currentFilterTypeIdx = 0;

    std::array<RotaryKnob*, 2> crushKnobs {};

    std::array<RotaryKnob*, 3> delayKnobs {};
    juce::ComboBox delayTimeModeBox;
    RotaryKnob* delaySyncDivKnob = nullptr;
    juce::ComboBox delaySyncTypeBox;

    std::array<RotaryKnob*, 3> reverbKnobs {};

    juce::OwnedArray<RotaryKnob> allKnobs;

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    juce::OwnedArray<SliderAttach> sliderAttachments;
    std::unique_ptr<ComboAttach> delayTimeModeAttach, delaySyncTypeAttach;
    std::unique_ptr<SliderAttach> delaySyncDivAttach;

    static juce::StringArray filterTypeNames()
    {
        return { "LP", "HP", "BP",
                 "Comb+ LP", "Comb+ BP",
                 "Comb- LP", "Comb- BP",
                 "Scream LP", "Scream BP" };
    }

    juce::String paramId (const juce::String& name) const
    {
        return "head" + juce::String (head) + "_" + name;
    }

    RotaryKnob* makeKnob (const juce::String& label, const juce::String& suffix,
                           const juce::String& param)
    {
        auto* k = new RotaryKnob (label, suffix);
        k->setAccentColour (accentColour);
        k->setVisible (false);
        addAndMakeVisible (k);
        allKnobs.add (k);
        sliderAttachments.add (new SliderAttach (apvtsRef, paramId (param), k->getSlider()));
        return k;
    }

    void createFilterKnobs()
    {
        filterKnobs[0] = makeKnob ("Cutoff",  " Hz", "filterCutoff");
        filterKnobs[1] = makeKnob ("Res",     "",    "filterRes");
        filterKnobs[2] = makeKnob ("Pan",     "",    "filterPan");
        filterKnobs[3] = makeKnob ("Drive",   "",    "filterDrive");
        filterKnobs[4] = makeKnob ("CombFrq", " Hz", "filterCombFreq");
        filterKnobs[5] = makeKnob ("Mix",     "",    "filterMix");

        // Pan readout: C / L xx / R xx
        filterKnobs[2]->getSlider().textFromValueFunction = [] (double v)
        {
            if (std::abs (v) < 0.005) return juce::String ("C");
            if (v < 0) return "L " + juce::String (static_cast<int> (std::round (-v * 100.0)));
            return "R " + juce::String (static_cast<int> (std::round (v * 100.0)));
        };
        filterKnobs[2]->getSlider().updateText();

        filterKnobs[3]->getSlider().textFromValueFunction = [] (double v)
        { return juce::String (static_cast<int> (std::round (v * 100.0))) + "%"; };
        filterKnobs[3]->getSlider().updateText();

        filterKnobs[5]->getSlider().textFromValueFunction = [] (double v)
        { return juce::String (static_cast<int> (std::round (v * 100.0))) + "%"; };
        filterKnobs[5]->getSlider().updateText();

        // Filter type: TextButton + PopupMenu (Serum-style grouped menu)
        // We do NOT use ComboBoxAttachment here, because JUCE has a known
        // off-by-one bug when a ComboBox has addSectionHeading() entries
        // while bound to a choice parameter via ComboBoxAttachment.
        // Instead, use ParameterAttachment for value sync and a popup menu
        // for selection — complete control, no bug.
        filterTypeButton.setColour (juce::TextButton::buttonColourId,  juce::Colour (0xff2A2A30));
        filterTypeButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff2A2A30));
        filterTypeButton.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xffcccccc));
        filterTypeButton.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xffcccccc));
        filterTypeButton.onClick = [this] { showFilterTypeMenu(); };
        filterTypeButton.setVisible (false);
        addAndMakeVisible (filterTypeButton);

        if (auto* p = apvtsRef.getParameter (paramId ("filterType")))
        {
            filterTypeAttach = std::make_unique<juce::ParameterAttachment> (
                *p,
                [this] (float denormValue)
                {
                    const auto names = filterTypeNames();
                    int idx = juce::jlimit (0, names.size() - 1,
                                            static_cast<int> (std::round (denormValue)));
                    DBG ("  [Attach CB] denormValue=" << denormValue
                         << "  -> idx=" << idx << "  (" << names[idx] << ")"
                         << "  head=" << head);
                    currentFilterTypeIdx = idx;
                    filterTypeButton.setButtonText (names[idx]);
                },
                nullptr);
            filterTypeAttach->sendInitialUpdate();
        }

        // Frequency response visualiser
        filterResponseDisplay = std::make_unique<FilterResponseDisplay> (apvtsRef, head, accentColour);
        filterResponseDisplay->setVisible (false);
        addAndMakeVisible (filterResponseDisplay.get());
    }

    void showFilterTypeMenu()
    {
        juce::PopupMenu m;
        const auto names = filterTypeNames();

        // PopupMenu headers/separators are purely cosmetic — item IDs are
        // controlled by us so there is no interaction with JUCE's
        // ComboBoxAttachment off-by-one issue.
        m.addSectionHeader ("Standard");
        for (int i = 0; i < 3; ++i)
            m.addItem (i + 1, names[i], true, i == currentFilterTypeIdx);

        m.addSeparator();
        m.addSectionHeader ("Comb");
        for (int i = 3; i < 7; ++i)
            m.addItem (i + 1, names[i], true, i == currentFilterTypeIdx);

        m.addSeparator();
        m.addSectionHeader ("Dist");
        for (int i = 7; i < 9; ++i)
            m.addItem (i + 1, names[i], true, i == currentFilterTypeIdx);

        auto opts = juce::PopupMenu::Options()
                       .withTargetComponent (&filterTypeButton)
                       .withMinimumWidth (filterTypeButton.getWidth());

        m.showMenuAsync (opts, [this] (int result)
        {
            DBG ("=== showFilterTypeMenu callback ===");
            DBG ("  menu result = " << result);
            if (result <= 0) { DBG ("  (no selection, aborting)"); return; }
            int idx = result - 1;
            DBG ("  target idx = " << idx << "  (" << filterTypeNames()[idx] << ")");

            auto* p = apvtsRef.getParameter (paramId ("filterType"));
            DBG ("  getParameter returned: " << (p ? "non-null" : "NULL"));
            if (p == nullptr) return;

            auto* choiceP = dynamic_cast<juce::AudioParameterChoice*> (p);
            DBG ("  dynamic_cast<AudioParameterChoice*>: " << (choiceP ? "SUCCESS" : "FAILED"));
            if (choiceP != nullptr)
                DBG ("  current index BEFORE write = " << choiceP->getIndex());

            if (choiceP)
            {
                p->beginChangeGesture();
                *choiceP = idx;
                p->endChangeGesture();
                DBG ("  current index AFTER write = " << choiceP->getIndex());
            }
            else
            {
                const int numChoices = p->getAllValueStrings().size();
                const float normalised = juce::jlimit (0.0f, 1.0f,
                    (static_cast<float> (idx) + 0.5f) / static_cast<float> (juce::jmax (1, numChoices)));
                DBG ("  fallback path, normalised = " << normalised);
                p->beginChangeGesture();
                p->setValueNotifyingHost (normalised);
                p->endChangeGesture();
            }

            DBG ("  currentFilterTypeIdx (UI cache) = " << currentFilterTypeIdx);
            DBG ("  button text = '" << filterTypeButton.getButtonText() << "'");
        });
    }

    void createCrushKnobs()
    {
        crushKnobs[0] = makeKnob ("Depth", " bit", "crushBits");
        crushKnobs[1] = makeKnob ("SR", "", "crushRate");
    }

    void createDelayKnobs()
    {
        delayKnobs[0] = makeKnob ("Time", " ms", "delayTime");
        delayKnobs[1] = makeKnob ("Fdbk", "", "delayFeedback");
        delayKnobs[2] = makeKnob ("Mix", "", "delayMix");

        delayTimeModeBox.addItemList ({ "Time", "Sync" }, 1);
        delayTimeModeBox.setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
        delayTimeModeBox.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
        delayTimeModeBox.onChange = [this] { updateDelayModeVisibility(); };
        delayTimeModeBox.setVisible (false);
        addAndMakeVisible (delayTimeModeBox);
        delayTimeModeAttach = std::make_unique<ComboAttach> (apvtsRef, paramId ("delayTimeMode"), delayTimeModeBox);

        delaySyncDivKnob = new RotaryKnob ("Div", "");
        delaySyncDivKnob->setAccentColour (accentColour);
        delaySyncDivKnob->getSlider().setRange (0, 5, 1);
        delaySyncDivKnob->getSlider().textFromValueFunction = [] (double v)
        {
            static const char* labels[] = { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32" };
            return juce::String (labels[juce::jlimit (0, 5, static_cast<int> (v))]);
        };
        delaySyncDivKnob->getSlider().updateText();
        delaySyncDivKnob->setVisible (false);
        addAndMakeVisible (delaySyncDivKnob);
        allKnobs.add (delaySyncDivKnob);
        delaySyncDivAttach = std::make_unique<SliderAttach> (apvtsRef, paramId ("delaySyncDiv"), delaySyncDivKnob->getSlider());

        delaySyncTypeBox.addItemList ({ "Norm", "Trip", "Dot" }, 1);
        delaySyncTypeBox.setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
        delaySyncTypeBox.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
        delaySyncTypeBox.setVisible (false);
        addAndMakeVisible (delaySyncTypeBox);
        delaySyncTypeAttach = std::make_unique<ComboAttach> (apvtsRef, paramId ("delaySyncType"), delaySyncTypeBox);
    }

    void createReverbKnobs()
    {
        reverbKnobs[0] = makeKnob ("Size", "", "reverbSize");
        reverbKnobs[1] = makeKnob ("Damp", "", "reverbDamp");
        reverbKnobs[2] = makeKnob ("Mix", "", "reverbMix");
    }

    void hideAllKnobs()
    {
        for (auto* k : allKnobs) k->setVisible (false);
        filterTypeButton.setVisible (false);
        if (filterResponseDisplay) filterResponseDisplay->setVisible (false);
        delayTimeModeBox.setVisible (false);
        delaySyncTypeBox.setVisible (false);
    }

    void updateKnobVisibility()
    {
        hideAllKnobs();
        if (collapsed || selectedType == FXType::None) return;

        switch (selectedType)
        {
            case FXType::Filter:
                for (auto* k : filterKnobs) if (k) k->setVisible (true);
                filterTypeButton.setVisible (true);
                if (filterResponseDisplay) filterResponseDisplay->setVisible (true);
                break;
            case FXType::Bitcrush:
                for (auto* k : crushKnobs) if (k) k->setVisible (true);
                break;
            case FXType::Delay:
            {
                bool delaySync = (delayTimeModeBox.getSelectedId() == 2);
                delayTimeModeBox.setVisible (true);
                delayKnobs[0]->setVisible (!delaySync);
                delaySyncDivKnob->setVisible (delaySync);
                delaySyncTypeBox.setVisible (delaySync);
                delayKnobs[1]->setVisible (true);
                delayKnobs[2]->setVisible (true);
                break;
            }
            case FXType::Reverb:
                for (auto* k : reverbKnobs) if (k) k->setVisible (true);
                break;
            default: break;
        }
    }

    void onFxTypeChanged()
    {
        if (selectedType != FXType::None)
        {
            juce::String prevName;
            switch (selectedType)
            {
                case FXType::Filter:   prevName = "filterOn"; break;
                case FXType::Bitcrush: prevName = "crushOn"; break;
                case FXType::Delay:    prevName = "delayOn"; break;
                case FXType::Reverb:   prevName = "reverbOn"; break;
                default: break;
            }
            if (auto* p = apvtsRef.getParameter (paramId (prevName)))
                p->setValueNotifyingHost (0.0f);
        }

        int sel = fxSelector.getSelectedId();
        if (sel <= 1)
            selectedType = FXType::None;
        else
            selectedType = static_cast<FXType> (sel - 2);

        if (selectedType != FXType::None)
        {
            isActive = true;
            juce::String pName;
            switch (selectedType)
            {
                case FXType::Filter:   pName = "filterOn"; break;
                case FXType::Bitcrush: pName = "crushOn"; break;
                case FXType::Delay:    pName = "delayOn"; break;
                case FXType::Reverb:   pName = "reverbOn"; break;
                default: break;
            }
            if (auto* p = apvtsRef.getParameter (paramId (pName)))
                p->setValueNotifyingHost (1.0f);
            activeDot.active = true;
            activeDot.repaint();
        }
        else
        {
            isActive = false;
            activeDot.active = false;
            activeDot.repaint();
        }

        updateKnobVisibility();

        if (selectedType == FXType::None && !collapsed)
            setCollapsed (true);
        else if (selectedType != FXType::None && collapsed)
            setCollapsed (false);
        else
        {
            resized();
            if (auto* parent = getParentComponent())
            {
                parent->resized();
                if (auto* gp = parent->getParentComponent()) gp->resized();
            }
        }

        if (onSelectionChanged)
            onSelectionChanged();
    }

    void toggleActive()
    {
        if (selectedType == FXType::None) return;

        isActive = !isActive;
        juce::String pName;

        switch (selectedType)
        {
            case FXType::Filter:   pName = "filterOn"; break;
            case FXType::Bitcrush: pName = "crushOn"; break;
            case FXType::Delay:    pName = "delayOn"; break;
            case FXType::Reverb:   pName = "reverbOn"; break;
            default: return;
        }

        if (auto* p = apvtsRef.getParameter (paramId (pName)))
            p->setValueNotifyingHost (isActive ? 1.0f : 0.0f);

        activeDot.active = isActive;
        activeDot.repaint();
        repaint();
    }

    void updateActiveState()
    {
        juce::String pName;
        switch (selectedType)
        {
            case FXType::Filter:   pName = "filterOn"; break;
            case FXType::Bitcrush: pName = "crushOn"; break;
            case FXType::Delay:    pName = "delayOn"; break;
            case FXType::Reverb:   pName = "reverbOn"; break;
            default: isActive = false; return;
        }

        if (auto* p = apvtsRef.getRawParameterValue (paramId (pName)))
            isActive = p->load() >= 0.5f;

        activeDot.active = isActive;
        activeDot.repaint();
    }

    void updateDelayModeVisibility()
    {
        if (selectedType != FXType::Delay) return;
        updateKnobVisibility();
        resized();
        if (auto* parent = getParentComponent())
        {
            parent->resized();
            if (auto* gp = parent->getParentComponent()) gp->resized();
        }
    }
};