#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "../RotaryKnob.h"
#include "../Modulation/StepSequencer.h"

class StepSequencerPanel : public juce::Component
{
public:
    StepSequencerPanel (CerberusGranAudioProcessor& p)
        : processor (p),
          rateKnob ("Rate", " Hz"),
          smoothKnob ("Smooth", "")
    {
        auto accent = juce::Colour (0xff8B5CF6);

        rateKnob.setAccentColour (accent);
        addAndMakeVisible (rateKnob);
        rateAttach = std::make_unique<SliderAttach> (p.apvts, "seq_rate", rateKnob.getSlider());

        smoothKnob.setAccentColour (accent);
        addAndMakeVisible (smoothKnob);
        smoothAttach = std::make_unique<SliderAttach> (p.apvts, "seq_smooth", smoothKnob.getSlider());

        bipolarBtn.setButtonText ("Bipolar");
        bipolarBtn.setClickingTogglesState (true);
        stylePillButton (bipolarBtn);
        addAndMakeVisible (bipolarBtn);
        bipolarAttach = std::make_unique<ButtonAttach> (p.apvts, "seq_bipolar", bipolarBtn);

        clearBtn.setButtonText ("Clear");
        clearBtn.setColour (juce::TextButton::buttonColourId, juce::Colour (0xffd8d8dc));
        clearBtn.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff555560));
        clearBtn.onClick = [this] {
            processor.modEngine.stepSeq.clearSteps();
            repaint();
        };
        addAndMakeVisible (clearBtn);

        // Play mode buttons
        const char* modeNames[] = { "Fwd", "Rev", "PP", "Rnd" };
        for (int i = 0; i < 4; ++i)
        {
            auto* b = new juce::TextButton (modeNames[i]);
            b->setClickingTogglesState (true);
            b->setRadioGroupId (202);
            stylePillButton (*b);
            b->onClick = [this, i] {
                if (auto* p = processor.apvts.getParameter ("seq_playmode"))
                    p->setValueNotifyingHost (static_cast<float> (i) / 3.0f);
            };
            playModeBtns.add (b);
            addAndMakeVisible (b);
        }
        // Sync initial state
        int currentMode = static_cast<int> (processor.apvts.getRawParameterValue ("seq_playmode")->load());
        playModeBtns[juce::jlimit (0, 3, currentMode)]->setToggleState (true, juce::dontSendNotification);
    }

    void paint (juce::Graphics& g) override
    {
        auto& seq = processor.modEngine.stepSeq;
        int length = seq.getLength();

        // Step display background
        g.setColour (juce::Colour (0xfff4f4f7));
        g.fillRoundedRectangle (stepAreaBounds.toFloat(), 8.0f);
        g.setColour (juce::Colour (0xffc0c0c5));
        g.drawRoundedRectangle (stepAreaBounds.toFloat().reduced (0.5f), 8.0f, 0.5f);

        // Centre line
        float midY = stepAreaBounds.getCentreY();
        g.setColour (juce::Colour (0xffbbbbc0));
        g.drawHorizontalLine ((int) midY, static_cast<float> (stepAreaBounds.getX()),
                                           static_cast<float> (stepAreaBounds.getRight()));

        // Step columns
        bool bipolar = processor.apvts.getRawParameterValue ("seq_bipolar")->load() >= 0.5f;
        int currentStep = seq.getCurrentStep();
        float colW = (float) stepAreaBounds.getWidth() / (float) StepSequencer::kMaxSteps;

        for (int i = 0; i < StepSequencer::kMaxSteps; ++i)
        {
            float x = stepAreaBounds.getX() + i * colW;
            bool active = (i < length);
            bool isCurrent = (i == currentStep) && active;

            // Column bar
            float val = seq.getStepValue (i);
            float barX = x + 1.5f;
            float barW = colW - 3.0f;

            juce::Colour barCol = active ? juce::Colour (0xff8B5CF6) : juce::Colour (0xffaaaaaf);
            if (isCurrent) barCol = barCol.brighter (0.2f);

            g.setColour (barCol.withAlpha (active ? 0.85f : 0.35f));

            if (bipolar)
            {
                // Bar from centre up or down
                float halfH = stepAreaBounds.getHeight() * 0.45f;
                float h = std::abs (val) * halfH;
                if (val >= 0)
                    g.fillRect (barX, midY - h, barW, h);
                else
                    g.fillRect (barX, midY, barW, h);
            }
            else
            {
                // Bar from bottom up
                float h = juce::jmax (0.0f, val) * stepAreaBounds.getHeight() * 0.9f;
                float bottom = stepAreaBounds.getBottom() - 4.0f;
                g.fillRect (barX, bottom - h, barW, h);
            }

            // Current step highlight
            if (isCurrent)
            {
                g.setColour (juce::Colour (0xff8B5CF6).withAlpha (0.15f));
                g.fillRect (x, (float) stepAreaBounds.getY(), colW, (float) stepAreaBounds.getHeight());
            }

            // Column divider
            if (i > 0)
            {
                g.setColour (juce::Colour (0xffe0e0e5));
                g.drawVerticalLine ((int) x, (float) stepAreaBounds.getY(), (float) stepAreaBounds.getBottom());
            }
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (4);

        // Top 65%: step display
        int displayH = (int) (area.getHeight() * 0.65f);
        stepAreaBounds = area.removeFromTop (displayH);
        area.removeFromTop (8);

        // Bottom: controls row
        int knobW = 60;
        rateKnob.setBounds   (area.removeFromLeft (knobW));
        area.removeFromLeft (4);
        smoothKnob.setBounds (area.removeFromLeft (knobW));
        area.removeFromLeft (12);

        // Play mode buttons
        int pmW = 38;
        for (auto* b : playModeBtns)
        {
            b->setBounds (area.removeFromLeft (pmW).withSizeKeepingCentre (pmW, 22));
            area.removeFromLeft (2);
        }
        area.removeFromLeft (10);

        bipolarBtn.setBounds (area.removeFromLeft (70).withSizeKeepingCentre (70, 22));
        area.removeFromLeft (6);
        clearBtn.setBounds   (area.removeFromLeft (60).withSizeKeepingCentre (60, 22));
    }

    void mouseDown (const juce::MouseEvent& e) override  { editStep (e); }
    void mouseDrag (const juce::MouseEvent& e) override  { editStep (e); }

private:
    void editStep (const juce::MouseEvent& e)
    {
        if (! stepAreaBounds.contains (e.getPosition())) return;

        float colW = (float) stepAreaBounds.getWidth() / (float) StepSequencer::kMaxSteps;
        int col = juce::jlimit (0, StepSequencer::kMaxSteps - 1,
                                (int) ((e.x - stepAreaBounds.getX()) / colW));

        bool bipolar = processor.apvts.getRawParameterValue ("seq_bipolar")->load() >= 0.5f;
        float midY = stepAreaBounds.getCentreY();
        float val;
        if (bipolar)
        {
            float halfH = stepAreaBounds.getHeight() * 0.45f;
            val = (midY - e.y) / halfH;
            val = juce::jlimit (-1.0f, 1.0f, val);
        }
        else
        {
            float bottom = stepAreaBounds.getBottom() - 4.0f;
            float h = bottom - e.y;
            val = juce::jlimit (0.0f, 1.0f, h / (stepAreaBounds.getHeight() * 0.9f));
        }

        processor.modEngine.stepSeq.setStepValue (col, val);
        repaint();
    }

    void stylePillButton (juce::TextButton& b)
    {
        b.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xffd8d8dc));
        b.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff8B5CF6));
        b.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xff555560));
        b.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xffffffff));
    }

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;

    CerberusGranAudioProcessor& processor;
    juce::Rectangle<int> stepAreaBounds;

    RotaryKnob rateKnob, smoothKnob;
    juce::TextButton bipolarBtn, clearBtn;
    juce::OwnedArray<juce::TextButton> playModeBtns;

    std::unique_ptr<SliderAttach> rateAttach, smoothAttach;
    std::unique_ptr<ButtonAttach> bipolarAttach;
};
