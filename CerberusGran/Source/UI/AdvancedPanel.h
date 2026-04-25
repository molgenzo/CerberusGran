#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "LFOPanel.h"
#include "StepSequencerPanel.h"

class AdvancedPanel : public juce::Component, private juce::Timer
{
public:
    AdvancedPanel (CerberusGranAudioProcessor& p)
        : processor (p),
          lfoPanel (p),
          seqPanel (p)
    {
        // Tab buttons — radio group so only one active
        lfoTabBtn.setButtonText ("LFO");
        lfoTabBtn.setClickingTogglesState (true);
        lfoTabBtn.setRadioGroupId (101);
        lfoTabBtn.setToggleState (true, juce::dontSendNotification);
        lfoTabBtn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xffd8d8dc));
        lfoTabBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xfff6f6f8));
        lfoTabBtn.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xff555560));
        lfoTabBtn.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xff1a1a1e));
        lfoTabBtn.onClick = [this] { showTab (0); };
        addAndMakeVisible (lfoTabBtn);

        seqTabBtn.setButtonText ("Step Seq");
        seqTabBtn.setClickingTogglesState (true);
        seqTabBtn.setRadioGroupId (101);
        seqTabBtn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xffd8d8dc));
        seqTabBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xfff6f6f8));
        seqTabBtn.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xff555560));
        seqTabBtn.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xff1a1a1e));
        seqTabBtn.onClick = [this] { showTab (1); };
        addAndMakeVisible (seqTabBtn);

        // Assign button — toggles "assigning" state in the editor above
        assignBtn.setButtonText ("Assign");
        assignBtn.setClickingTogglesState (true);
        assignBtn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xffd8d8dc));
        assignBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff33333a));
        assignBtn.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xff555560));
        assignBtn.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xffeeeeee));
        assignBtn.onClick = [this] {
            if (onAssignToggled)
                onAssignToggled (currentTab, assignBtn.getToggleState());
        };
        addAndMakeVisible (assignBtn);

        addChildComponent (lfoPanel);
        addChildComponent (seqPanel);
        lfoPanel.setVisible (true);

        startTimerHz (30);
    }

    ~AdvancedPanel() override { stopTimer(); }

    // Called from editor to turn assign mode on/off from outside
    void setAssignState (bool active)
    {
        assignBtn.setToggleState (active, juce::dontSendNotification);
    }

    int getCurrentSourceIndex() const { return currentTab; }

    std::function<void (int sourceIndex, bool on)> onAssignToggled;

    void paint (juce::Graphics& g) override
    {
        // Light theme background
        g.fillAll (juce::Colour (0xffe8e8ec));

        // Top divider line
        g.setColour (juce::Colour (0xff9a9aa0));
        g.drawHorizontalLine (0, 0.0f, static_cast<float> (getWidth()));

        // Section title
        g.setColour (juce::Colour (0xff1a1a1e));
        g.setFont (juce::Font ("Avenir", 22.0f, juce::Font::bold));
        g.drawText ("Modulation", 12, 4, 280, 28,
                    juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (8);
        area.removeFromTop (30); // space for "Modulation" title

        // Top row: tabs + Assign button on right
        auto tabRow = area.removeFromTop (26);
        lfoTabBtn.setBounds (tabRow.removeFromLeft (80));
        tabRow.removeFromLeft (4);
        seqTabBtn.setBounds (tabRow.removeFromLeft (80));
        assignBtn.setBounds (tabRow.removeFromRight (80));

        area.removeFromTop (6);

        // Body: selected panel fills remaining
        lfoPanel.setBounds (area);
        seqPanel.setBounds (area);
    }

    void timerCallback() override
    {
        if (currentTab == 0) lfoPanel.repaint();
        else                 seqPanel.repaint();
    }

private:
    void showTab (int idx)
    {
        currentTab = idx;
        lfoPanel.setVisible (idx == 0);
        seqPanel.setVisible (idx == 1);
        // Exit assign mode on tab change so user explicitly opts in on the new source
        if (assignBtn.getToggleState())
        {
            assignBtn.setToggleState (false, juce::dontSendNotification);
            if (onAssignToggled) onAssignToggled (idx, false);
        }
    }

    CerberusGranAudioProcessor& processor;
    juce::TextButton lfoTabBtn, seqTabBtn, assignBtn;
    LFOPanel lfoPanel;
    StepSequencerPanel seqPanel;
    int currentTab = 0;
};
