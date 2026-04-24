#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "../RotaryKnob.h"

class LFOPanel : public juce::Component
{
public:
    LFOPanel (CerberusGranAudioProcessor& p)
        : processor (p),
          rateKnob ("Rate", " Hz"),
          depthKnob ("Depth", ""),
          phaseKnob ("Phase", "")
    {
        auto accent = juce::Colour (0xff8B5CF6);

        rateKnob.setAccentColour (accent);
        addAndMakeVisible (rateKnob);
        rateAttach = std::make_unique<SliderAttach> (p.apvts, "lfo_rate", rateKnob.getSlider());

        depthKnob.setAccentColour (accent);
        addAndMakeVisible (depthKnob);
        depthAttach = std::make_unique<SliderAttach> (p.apvts, "lfo_depth", depthKnob.getSlider());

        phaseKnob.setAccentColour (accent);
        addAndMakeVisible (phaseKnob);
        phaseAttach = std::make_unique<SliderAttach> (p.apvts, "lfo_phase", phaseKnob.getSlider());

        shapeBox.addItemList ({ "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H" }, 1);
        shapeBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xffd8d8dc));
        shapeBox.setColour (juce::ComboBox::textColourId, juce::Colour (0xff1a1a1e));
        addAndMakeVisible (shapeBox);
        shapeAttach = std::make_unique<ComboAttach> (p.apvts, "lfo_shape", shapeBox);

        bipolarBtn.setButtonText ("Bipolar");
        bipolarBtn.setClickingTogglesState (true);
        bipolarBtn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xffd8d8dc));
        bipolarBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff8B5CF6));
        bipolarBtn.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xff555560));
        bipolarBtn.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xffffffff));
        addAndMakeVisible (bipolarBtn);
        bipolarAttach = std::make_unique<ButtonAttach> (p.apvts, "lfo_bipolar", bipolarBtn);
    }

    void paint (juce::Graphics& g) override
    {
        // Preview area background
        auto preview = previewBounds.toFloat();
        g.setColour (juce::Colour (0xfff4f4f7));
        g.fillRoundedRectangle (preview, 8.0f);
        g.setColour (juce::Colour (0xffc0c0c5));
        g.drawRoundedRectangle (preview.reduced (0.5f), 8.0f, 0.5f);

        // Centre line
        float midY = preview.getCentreY();
        g.setColour (juce::Colour (0xffbbbbc0));
        g.drawHorizontalLine ((int) midY, preview.getX(), preview.getRight());

        // Draw LFO shape — one cycle across preview width
        const auto& lfo = processor.modEngine.lfo;
        juce::Path path;
        int numPts = juce::jmax (32, (int) preview.getWidth());
        bool bipolar = processor.apvts.getRawParameterValue ("lfo_bipolar")->load() >= 0.5f;
        float halfH = preview.getHeight() * 0.45f;

        for (int i = 0; i <= numPts; ++i)
        {
            float phase = (float) i / (float) numPts;
            float v = lfo.shapeValueAtPhase (phase);
            // When bipolar, v already in [-depth, depth]; when unipolar v in [0, depth]
            float normalised = bipolar ? v : (v * 2.0f - 1.0f);  // remap unipolar to [-1,1] for display
            float x = preview.getX() + phase * preview.getWidth();
            float y = midY - normalised * halfH;

            if (i == 0) path.startNewSubPath (x, y);
            else        path.lineTo (x, y);
        }

        g.setColour (juce::Colour (0xff8B5CF6));
        g.strokePath (path, juce::PathStrokeType (1.8f));

        // Playhead cursor
        float phase = lfo.getPhase();
        float px = preview.getX() + phase * preview.getWidth();
        g.setColour (juce::Colour (0xff8B5CF6).withAlpha (0.7f));
        g.drawVerticalLine ((int) px, preview.getY(), preview.getBottom());
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (4);

        // Top 60%: waveform preview
        int previewH = (int) (area.getHeight() * 0.55f);
        previewBounds = area.removeFromTop (previewH);
        area.removeFromTop (8);

        // Bottom: controls row
        int knobW = 70;
        int rowH = area.getHeight();
        rateKnob.setBounds  (area.removeFromLeft (knobW));
        area.removeFromLeft (4);
        depthKnob.setBounds (area.removeFromLeft (knobW));
        area.removeFromLeft (4);
        phaseKnob.setBounds (area.removeFromLeft (knobW));
        area.removeFromLeft (12);

        int boxW = 90;
        int boxH = 24;
        shapeBox.setBounds (area.removeFromLeft (boxW).withSizeKeepingCentre (boxW, boxH));
        area.removeFromLeft (10);
        bipolarBtn.setBounds (area.removeFromLeft (80).withSizeKeepingCentre (80, 22));
    }

private:
    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;

    CerberusGranAudioProcessor& processor;
    juce::Rectangle<int> previewBounds;

    RotaryKnob rateKnob, depthKnob, phaseKnob;
    juce::ComboBox shapeBox;
    juce::TextButton bipolarBtn;

    std::unique_ptr<SliderAttach> rateAttach, depthAttach, phaseAttach;
    std::unique_ptr<ComboAttach>  shapeAttach;
    std::unique_ptr<ButtonAttach> bipolarAttach;
};
