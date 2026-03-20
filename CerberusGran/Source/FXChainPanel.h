#pragma once
#include <JuceHeader.h>
#include "FXSlotCard.h"

class FXChainPanel : public juce::Component
{
public:
    FXChainPanel (juce::AudioProcessorValueTreeState& apvts, int headIndex, juce::Colour accent)
    {
        for (int i = 0; i < 4; ++i)
        {
            auto* card = new FXSlotCard (apvts, headIndex, accent);
            card->onSelectionChanged = [this] { updateAvailableEffects(); };
            cards.add (card);
            addAndMakeVisible (card);
        }
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colour (0xffaaaaaa));
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText ("FX CHAIN", 0, 0, getWidth(), 20, juce::Justification::centredLeft);
    }

    void resized() override
    {
        int y = 22;
        for (auto* card : cards)
        {
            int h = card->getIdealHeight();
            card->setBounds (0, y, getWidth(), h);
            y += h + 2;
        }

        setSize (getWidth(), getIdealHeight());
    }

    int getIdealHeight() const
    {
        int h = 22;
        for (auto* card : cards)
            h += card->getIdealHeight() + 2;
        return h;
    }

private:
    juce::OwnedArray<FXSlotCard> cards;

    void updateAvailableEffects()
    {
        // Collect which effects are used across all slots
        for (int i = 0; i < cards.size(); ++i)
        {
            std::set<FXType> usedByOthers;
            for (int j = 0; j < cards.size(); ++j)
            {
                if (j == i) continue;
                auto t = cards[j]->getSelectedType();
                if (t != FXType::None)
                    usedByOthers.insert (t);
            }
            cards[i]->setDisabledEffects (usedByOthers);
        }
    }
};
