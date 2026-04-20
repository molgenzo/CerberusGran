#pragma once
#include <JuceHeader.h>
#include <set>
#include "RotaryKnob.h"

enum class FXType { None = -1, Filter = 0, Bitcrush = 1, Delay = 2, Reverb = 3 };

// Circular toggle dot
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
    static constexpr int kFixedHeight = 118;

    FXSlotCard (juce::AudioProcessorValueTreeState& apvts, int headIndex, juce::Colour accent)
        : apvtsRef (apvts), head (headIndex), accentColour (accent)
    {
        // FX type selector
        fxSelector.addItem ("None", 1);
        fxSelector.addItem ("Filter", 2);
        fxSelector.addItem ("Bitcrush", 3);
        fxSelector.addItem ("Delay", 4);
        fxSelector.addItem ("Reverb", 5);
        fxSelector.setSelectedId (1, juce::dontSendNotification);
        fxSelector.setTextWhenNothingSelected ("+ add FX");
        fxSelector.setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
        fxSelector.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
        fxSelector.setColour (juce::ComboBox::textColourId, juce::Colour (0xffcccccc));
        fxSelector.onChange = [this] { onFxTypeChanged(); };
        addAndMakeVisible (fxSelector);

        // Active dot
        activeDot.accentColour = accent;
        addAndMakeVisible (activeDot);
        activeDot.onClick = [this] { toggleActive(); };

        // Create ALL knob sets upfront (hidden by default)
        createFilterKnobs();
        createCrushKnobs();
        createDelayKnobs();
        createReverbKnobs();

        hideAllKnobs();
    }

    FXType getSelectedType() const { return selectedType; }
    int getIdealHeight() const { return kFixedHeight; }

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

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();

        // Slot background — per-head accent tint
        g.setColour (accentColour.withAlpha (0.20f));
        g.fillRoundedRectangle (b, 10.0f);

        // Header pill bar — dark rounded rectangle at top
        auto headerRect = b.removeFromTop (kHeaderHeight).reduced (2.0f, 2.0f);
        g.setColour (juce::Colour (0xff1E1E24));
        g.fillRoundedRectangle (headerRect, headerRect.getHeight() * 0.45f);
    }

    void resized() override
    {
        auto area = getLocalBounds();

        // Header pill area
        auto header = area.removeFromTop (kHeaderHeight).reduced (2, 2);
        activeDot.setBounds (header.removeFromLeft (22));
        fxSelector.setBounds (header.reduced (2, 1));

        // Body: knobs area
        auto body = area.reduced (4, 2);

        if (selectedType == FXType::None) return;

        switch (selectedType)
        {
            case FXType::Filter:
            {
                int knobW = body.getWidth() / 3;
                filterKnobs[0]->setBounds (body.removeFromLeft (knobW));
                filterKnobs[1]->setBounds (body.removeFromLeft (knobW));
                filterTypeBox.setBounds (body.withHeight (18).withY (body.getCentreY() - 9));
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
                auto modeRow = body.removeFromTop (18);
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
    static constexpr int kHeaderHeight = 26;

    juce::AudioProcessorValueTreeState& apvtsRef;
    int head;
    juce::Colour accentColour;
    FXType selectedType = FXType::None;
    bool isActive = false;

    juce::ComboBox fxSelector;
    ActiveDot activeDot;

    // Filter knobs
    std::array<RotaryKnob*, 2> filterKnobs {};
    juce::ComboBox filterTypeBox;

    // Crush knobs
    std::array<RotaryKnob*, 2> crushKnobs {};

    // Delay knobs + sync controls
    std::array<RotaryKnob*, 3> delayKnobs {};
    juce::ComboBox delayTimeModeBox;
    RotaryKnob* delaySyncDivKnob = nullptr;
    juce::ComboBox delaySyncTypeBox;

    // Reverb knobs
    std::array<RotaryKnob*, 3> reverbKnobs {};

    // Owned knobs
    juce::OwnedArray<RotaryKnob> allKnobs;

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    juce::OwnedArray<SliderAttach> sliderAttachments;
    std::unique_ptr<ComboAttach> filterTypeAttach;
    std::unique_ptr<ComboAttach> delayTimeModeAttach, delaySyncTypeAttach;
    std::unique_ptr<SliderAttach> delaySyncDivAttach;

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
        filterKnobs[0] = makeKnob ("Cut", "", "filterCutoff");
        filterKnobs[1] = makeKnob ("Res", "", "filterRes");

        filterTypeBox.addItemList ({ "LP", "HP", "BP" }, 1);
        filterTypeBox.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
        filterTypeBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2A2A30));
        filterTypeBox.setVisible (false);
        addAndMakeVisible (filterTypeBox);
        filterTypeAttach = std::make_unique<ComboAttach> (apvtsRef, paramId ("filterType"), filterTypeBox);
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
        delayTimeModeBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2A2A30));
        delayTimeModeBox.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
        delayTimeModeBox.onChange = [this] { updateDelayModeVisibility(); };
        delayTimeModeBox.setVisible (false);
        addAndMakeVisible (delayTimeModeBox);
        delayTimeModeAttach = std::make_unique<ComboAttach> (apvtsRef, paramId ("delayTimeMode"), delayTimeModeBox);

        delaySyncDivKnob = new RotaryKnob ("Div", "");
        delaySyncDivKnob->setAccentColour (juce::Colour (0xff8B5CF6));
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
        delaySyncTypeBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2A2A30));
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
        filterTypeBox.setVisible (false);
        delayTimeModeBox.setVisible (false);
        delaySyncTypeBox.setVisible (false);
    }

    void updateKnobVisibility()
    {
        hideAllKnobs();
        if (selectedType == FXType::None) return;

        switch (selectedType)
        {
            case FXType::Filter:
                for (auto* k : filterKnobs) if (k) k->setVisible (true);
                filterTypeBox.setVisible (true);
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
        // Turn off the previously selected effect
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

        // Auto-enable the new effect
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
        resized();

        if (auto* parent = getParentComponent())
        {
            parent->resized();
            if (auto* gp = parent->getParentComponent()) gp->resized();
        }

        if (onSelectionChanged) onSelectionChanged();
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
