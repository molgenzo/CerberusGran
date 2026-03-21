#include "PluginEditor.h"

CerberusGranAudioProcessorEditor::CerberusGranAudioProcessorEditor (CerberusGranAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      audioProcessor (p),
      waveformDisplay (p, headColours),
      globalBar (p.apvts)
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
        auto* col = new EngineColumn (p.apvts, i, headColours[i]);
        columns.add (col);
        addAndMakeVisible (col);
    }

    addAndMakeVisible (globalBar);

    setSize (1300, 850);
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

    // Global bar at top
    globalBar.setBounds (area.removeFromTop (40));

    // Waveform below global bar
    waveformDisplay.setBounds (area.removeFromTop (130).reduced (8, 4));

    // 5 engine columns fill the middle
    auto columnsArea = area.reduced (4, 4);
    int colW = columnsArea.getWidth() / kNumCols;

    for (int i = 0; i < kNumCols; ++i)
    {
        columns[i]->setBounds (columnsArea.removeFromLeft (colW).reduced (2));
    }
}

void CerberusGranAudioProcessorEditor::timerCallback()
{
    if (audioProcessor.getSourceMode() == AudioSourceMode::Live)
        waveformDisplay.updateLiveWaveform();

    waveformDisplay.repaint();
}
