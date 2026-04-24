#include "PluginEditor.h"

CerberusGranAudioProcessorEditor::CerberusGranAudioProcessorEditor (CerberusGranAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      audioProcessor (p),
      waveformDisplay (p, headColours),
      globalBar (p.apvts, headColours),
      advancedPanel (p)
{
    setLookAndFeel (&cerberusLnf);

    headColours = {
        juce::Colour (0xff8B5CF6),  // Purple
        juce::Colour (0xff2DD4BF),  // Teal
        juce::Colour (0xffF97066),  // Coral
        juce::Colour (0xffEC4899),  // Pink
        juce::Colour (0xff3B82F6)   // Blue
    };

    addAndMakeVisible (waveformDisplay);

    for (int i = 0; i < kNumCols; ++i)
    {
        auto* col = new EngineColumn (p.apvts, i, headColours[i], &p.modEngine);
        columns.add (col);
        addAndMakeVisible (col);
        col->setVisible (i == 0);

        col->onAdvancedToggled = [this] (bool on) { toggleAdvanced (on); };
    }

    addAndMakeVisible (globalBar);

    // Advanced panel hidden until toggled
    addChildComponent (advancedPanel);
    advancedPanel.setVisible (false);

    advancedPanel.onAssignToggled = [this] (int srcIdx, bool on) {
        // Broadcast assign mode to all EngineColumns (only the visible one will respond to mouse)
        for (auto* col : columns)
            col->setAssignMode (srcIdx, on);
    };

    globalBar.onHeadChanged = [this] (int newHead) { switchToHead (newHead); };

    setSize (620, 416);
    startTimerHz (30);
}

CerberusGranAudioProcessorEditor::~CerberusGranAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void CerberusGranAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1A1A1E));
}

void CerberusGranAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // Advanced panel pinned to bottom when visible
    if (advancedVisible)
    {
        auto advArea = area.removeFromBottom (kAdvancedHeight);
        advancedPanel.setBounds (advArea);
    }

    // Global bar at top
    globalBar.setBounds (area.removeFromTop (48));

    // Waveform below global bar
    waveformDisplay.setBounds (area.removeFromTop (80).reduced (8, 4));

    // Engine column fills remaining space
    auto columnArea = area.reduced (2, 2);
    for (int i = 0; i < kNumCols; ++i)
        columns[i]->setBounds (columnArea);
}

void CerberusGranAudioProcessorEditor::timerCallback()
{
    if (audioProcessor.getSourceMode() == AudioSourceMode::Live)
        waveformDisplay.updateLiveWaveform();

    waveformDisplay.repaint();
}

void CerberusGranAudioProcessorEditor::switchToHead (int headIndex)
{
    headIndex = juce::jlimit (0, kNumCols - 1, headIndex);
    if (headIndex == currentHeadIndex) return;

    columns[currentHeadIndex]->setVisible (false);
    currentHeadIndex = headIndex;
    columns[currentHeadIndex]->setVisible (true);
    waveformDisplay.setActiveHead (headIndex);
}

void CerberusGranAudioProcessorEditor::toggleAdvanced (bool on)
{
    advancedVisible = on;
    advancedPanel.setVisible (on);

    // Sync all EngineColumn Advanced buttons to match the new state
    for (auto* col : columns)
        col->advancedBtn.setToggleState (on, juce::dontSendNotification);

    int baseH = 416;
    setSize (getWidth(), on ? baseH + kAdvancedHeight : baseH);
}
