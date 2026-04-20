#include "PluginEditor.h"

CerberusGranAudioProcessorEditor::CerberusGranAudioProcessorEditor (CerberusGranAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      audioProcessor (p),
      waveformDisplay (p, headColours),
      globalBar (p.apvts, headColours)
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
        col->setVisible (i == 0); // only show head 0 initially
    }

    addAndMakeVisible (globalBar);

    // Wire head navigation from GlobalBar
    globalBar.onHeadChanged = [this] (int newHead) { switchToHead (newHead); };
    globalBar.onPresetSaveAs = [this] { savePresetAs(); };
    globalBar.onPresetLoad = [this] { loadPresetFromDisk(); };

    // Tight sizing: topBar(48) + waveform(80) + labels(24) + fxPanel(2*118+4=240) + padding(24) = 416
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

    // Global bar at top
    globalBar.setBounds (area.removeFromTop (48));

    // Waveform below global bar
    waveformDisplay.setBounds (area.removeFromTop (80).reduced (8, 4));

    // Single engine column fills remaining space
    auto columnArea = area.reduced (2, 2);
    for (int i = 0; i < kNumCols; ++i)
        columns[i]->setBounds (columnArea);
}

void CerberusGranAudioProcessorEditor::timerCallback()
{
    waveformDisplay.updateSourceModeUi();

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

void CerberusGranAudioProcessorEditor::savePresetAs()
{
    presetFileChooser = std::make_unique<juce::FileChooser> (
        "Save preset as",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
        "*.pocketpreset");

    const auto flags = juce::FileBrowserComponent::saveMode
                     | juce::FileBrowserComponent::canSelectFiles
                     | juce::FileBrowserComponent::warnAboutOverwriting;

    presetFileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
    {
        auto target = chooser.getResult();
        if (target == juce::File())
            return;

        if (!target.hasFileExtension ("pocketpreset"))
            target = target.withFileExtension (".pocketpreset");

        if (!audioProcessor.savePresetToFile (target))
        {
            juce::NativeMessageBox::showMessageBoxAsync (
                juce::MessageBoxIconType::WarningIcon,
                "Preset Save Failed",
                "Could not save preset to the selected file.");
        }

        presetFileChooser.reset();
    });
}

void CerberusGranAudioProcessorEditor::loadPresetFromDisk()
{
    const bool wasFileMode = (audioProcessor.getSourceMode() == AudioSourceMode::File);

    presetFileChooser = std::make_unique<juce::FileChooser> (
        "Load preset",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
        "*.pocketpreset;*.xml");

    const auto flags = juce::FileBrowserComponent::openMode
                     | juce::FileBrowserComponent::canSelectFiles;

    presetFileChooser->launchAsync (flags, [this, wasFileMode] (const juce::FileChooser& chooser)
    {
        auto source = chooser.getResult();
        if (source == juce::File())
            return;

        if (!audioProcessor.loadPresetFromFile (source))
        {
            juce::NativeMessageBox::showMessageBoxAsync (
                juce::MessageBoxIconType::WarningIcon,
                "Preset Load Failed",
                "Could not load preset from the selected file.");
        }
        else
        {
            if (auto* modeParam = audioProcessor.apvts.getParameter ("sourceMode"))
                modeParam->setValueNotifyingHost (wasFileMode ? 1.0f : 0.0f);
        }

        presetFileChooser.reset();
    });
}
