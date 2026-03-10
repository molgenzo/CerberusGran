#include "PluginEditor.h"

// ============================================================================
// HeadPanel
// ============================================================================
static void setupRotarySlider (juce::Slider& s, const juce::String& suffix)
{
    s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
    if (suffix.isNotEmpty()) s.setTextValueSuffix (suffix);
}

HeadPanel::HeadPanel (CerberusGranAudioProcessor& p, int headIndex, juce::Colour headColour)
    : processor (p), index (headIndex), colour (headColour)
{
    auto id = [headIndex] (const juce::String& name)
    { return "head" + juce::String (headIndex) + "_" + name; };

    // Title
    titleLabel.setText ("Head " + juce::String (headIndex), juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, headColour);
    addAndMakeVisible (titleLabel);

    collapseButton.setButtonText ("+/-");
    collapseButton.onClick = [this]
    {
        setCollapsed (!collapsed);
        if (auto* parent = getParentComponent())
            parent->resized();
    };
    addAndMakeVisible (collapseButton);

    // Enable button - reuse collapseButton area or add a toggle
    enableAttach = std::make_unique<ButtonAttach> (p.apvts, id ("enable"), collapseButton);

    // Grain knobs
    auto setupKnob = [this] (juce::Slider& s, const juce::String& suffix = "")
    {
        setupRotarySlider (s, suffix);
        addAndMakeVisible (s);
    };

    setupKnob (positionSlider);
    setupKnob (posScatterSlider);
    setupKnob (densitySlider, " Hz");
    setupKnob (durationSlider, " ms");
    setupKnob (pitchSlider, "x");
    setupKnob (pitchScatterSlider);
    setupKnob (gainSlider);
    setupKnob (panSlider);

    windowBox.addItemList ({ "Hann", "Gaussian", "Tukey", "Triangle" }, 1);
    addAndMakeVisible (windowBox);

    // Attach grain params
    positionAttach    = std::make_unique<SliderAttach> (p.apvts, id ("position"), positionSlider);
    posScatterAttach  = std::make_unique<SliderAttach> (p.apvts, id ("posScatter"), posScatterSlider);
    densityAttach     = std::make_unique<SliderAttach> (p.apvts, id ("density"), densitySlider);
    durationAttach    = std::make_unique<SliderAttach> (p.apvts, id ("duration"), durationSlider);
    pitchAttach       = std::make_unique<SliderAttach> (p.apvts, id ("pitch"), pitchSlider);
    pitchScatterAttach= std::make_unique<SliderAttach> (p.apvts, id ("pitchScatter"), pitchScatterSlider);
    gainAttach        = std::make_unique<SliderAttach> (p.apvts, id ("gain"), gainSlider);
    panAttach         = std::make_unique<SliderAttach> (p.apvts, id ("pan"), panSlider);
    windowAttach      = std::make_unique<ComboAttach>  (p.apvts, id ("window"), windowBox);

    // Filter
    filterBypassBtn.setButtonText ("Filter");
    addAndMakeVisible (filterBypassBtn);
    filterBypassAttach = std::make_unique<ButtonAttach> (p.apvts, id ("filterBypass"), filterBypassBtn);

    filterTypeBox.addItemList ({ "LP", "HP", "BP" }, 1);
    addAndMakeVisible (filterTypeBox);
    filterTypeAttach = std::make_unique<ComboAttach> (p.apvts, id ("filterType"), filterTypeBox);

    setupKnob (filterCutoffSlider, " Hz");
    setupKnob (filterResSlider);
    filterCutoffAttach = std::make_unique<SliderAttach> (p.apvts, id ("filterCutoff"), filterCutoffSlider);
    filterResAttach    = std::make_unique<SliderAttach> (p.apvts, id ("filterRes"), filterResSlider);

    // Saturator
    satBypassBtn.setButtonText ("Sat");
    addAndMakeVisible (satBypassBtn);
    satBypassAttach = std::make_unique<ButtonAttach> (p.apvts, id ("satBypass"), satBypassBtn);

    setupKnob (satDriveSlider, "x");
    satDriveAttach = std::make_unique<SliderAttach> (p.apvts, id ("satDrive"), satDriveSlider);

    // Bitcrusher
    crushBypassBtn.setButtonText ("Crush");
    addAndMakeVisible (crushBypassBtn);
    crushBypassAttach = std::make_unique<ButtonAttach> (p.apvts, id ("crushBypass"), crushBypassBtn);

    setupKnob (crushBitsSlider, " bit");
    setupKnob (crushRateSlider);
    crushBitsAttach = std::make_unique<SliderAttach> (p.apvts, id ("crushBits"), crushBitsSlider);
    crushRateAttach = std::make_unique<SliderAttach> (p.apvts, id ("crushRate"), crushRateSlider);

    // Delay
    delayBypassBtn.setButtonText ("Delay");
    addAndMakeVisible (delayBypassBtn);
    delayBypassAttach = std::make_unique<ButtonAttach> (p.apvts, id ("delayBypass"), delayBypassBtn);

    setupKnob (delayTimeSlider, " ms");
    setupKnob (delayFeedbackSlider);
    setupKnob (delayMixSlider);
    delayTimeAttach     = std::make_unique<SliderAttach> (p.apvts, id ("delayTime"), delayTimeSlider);
    delayFeedbackAttach = std::make_unique<SliderAttach> (p.apvts, id ("delayFeedback"), delayFeedbackSlider);
    delayMixAttach      = std::make_unique<SliderAttach> (p.apvts, id ("delayMix"), delayMixSlider);

    // LFO
    setupKnob (lfoRateSlider, " Hz");
    setupKnob (lfoDepthSlider);
    lfoRateAttach  = std::make_unique<SliderAttach> (p.apvts, id ("lfoRate"), lfoRateSlider);
    lfoDepthAttach = std::make_unique<SliderAttach> (p.apvts, id ("lfoDepth"), lfoDepthSlider);

    lfoShapeBox.addItemList ({ "Sine", "Tri", "Saw", "Sq", "S&H" }, 1);
    addAndMakeVisible (lfoShapeBox);
    lfoShapeAttach = std::make_unique<ComboAttach> (p.apvts, id ("lfoShape"), lfoShapeBox);

    lfoTargetBox.addItemList ({ "Off", "Position", "Pitch", "Cutoff" }, 1);
    addAndMakeVisible (lfoTargetBox);
    lfoTargetAttach = std::make_unique<ComboAttach> (p.apvts, id ("lfoTarget"), lfoTargetBox);
}

void HeadPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (colour.withAlpha (0.08f));
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (colour.withAlpha (0.4f));
    g.drawRoundedRectangle (bounds.reduced (1.0f), 6.0f, 1.0f);
}

void HeadPanel::resized()
{
    auto area = getLocalBounds().reduced (6);
    auto topRow = area.removeFromTop (24);
    titleLabel.setBounds (topRow.removeFromLeft (80));
    collapseButton.setBounds (topRow.removeFromRight (40));

    if (collapsed) return;

    int knobW = 65, knobH = 70;

    // Row 1: Grain knobs
    auto row1 = area.removeFromTop (knobH);
    positionSlider.setBounds (row1.removeFromLeft (knobW));
    posScatterSlider.setBounds (row1.removeFromLeft (knobW));
    densitySlider.setBounds (row1.removeFromLeft (knobW));
    durationSlider.setBounds (row1.removeFromLeft (knobW));
    pitchSlider.setBounds (row1.removeFromLeft (knobW));
    pitchScatterSlider.setBounds (row1.removeFromLeft (knobW));
    gainSlider.setBounds (row1.removeFromLeft (knobW));
    panSlider.setBounds (row1.removeFromLeft (knobW));
    windowBox.setBounds (row1.removeFromLeft (80).withHeight (24));

    // Row 2: DSP chain
    auto row2 = area.removeFromTop (knobH);

    // Filter section
    filterBypassBtn.setBounds (row2.removeFromLeft (50).withHeight (22));
    filterTypeBox.setBounds (row2.removeFromLeft (45).withHeight (22));
    filterCutoffSlider.setBounds (row2.removeFromLeft (knobW));
    filterResSlider.setBounds (row2.removeFromLeft (knobW));

    row2.removeFromLeft (8);

    // Saturator
    satBypassBtn.setBounds (row2.removeFromLeft (40).withHeight (22));
    satDriveSlider.setBounds (row2.removeFromLeft (knobW));

    row2.removeFromLeft (8);

    // Bitcrusher
    crushBypassBtn.setBounds (row2.removeFromLeft (50).withHeight (22));
    crushBitsSlider.setBounds (row2.removeFromLeft (knobW));
    crushRateSlider.setBounds (row2.removeFromLeft (knobW));

    row2.removeFromLeft (8);

    // Delay
    delayBypassBtn.setBounds (row2.removeFromLeft (50).withHeight (22));
    delayTimeSlider.setBounds (row2.removeFromLeft (knobW));
    delayFeedbackSlider.setBounds (row2.removeFromLeft (knobW));
    delayMixSlider.setBounds (row2.removeFromLeft (knobW));

    // Row 3: LFO
    auto row3 = area.removeFromTop (knobH);
    auto lfoLabel = row3.removeFromLeft (30);
    lfoRateSlider.setBounds (row3.removeFromLeft (knobW));
    lfoDepthSlider.setBounds (row3.removeFromLeft (knobW));
    lfoShapeBox.setBounds (row3.removeFromLeft (60).withHeight (22));
    row3.removeFromLeft (4);
    lfoTargetBox.setBounds (row3.removeFromLeft (80).withHeight (22));
}

void HeadPanel::setupSlider (juce::Slider& s, const juce::String& suffix)
{
    setupRotarySlider (s, suffix);
    addAndMakeVisible (s);
}

// ============================================================================
// CerberusGranAudioProcessorEditor
// ============================================================================
CerberusGranAudioProcessorEditor::CerberusGranAudioProcessorEditor (CerberusGranAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      audioProcessor (p),
      thumbnail (512, p.getFormatManager(), p.getThumbnailCache())
{
    headColours = {
        juce::Colour (0xffff6b6b),  // Coral
        juce::Colour (0xffffa726),  // Amber
        juce::Colour (0xff66bb6a),  // Green
        juce::Colour (0xff4dd0e1),  // Cyan
        juce::Colour (0xffab47bc)   // Violet
    };

    for (int i = 0; i < kNumHeads; ++i)
    {
        auto* panel = new HeadPanel (p, i, headColours[i]);
        headPanels.add (panel);
        headContainer.addAndMakeVisible (panel);
    }

    headViewport.setViewedComponent (&headContainer, false);
    headViewport.setScrollBarsShown (true, false);
    addAndMakeVisible (headViewport);

    // Global controls
    setupSlider (masterGainSlider);
    setupSlider (dryWetSlider);
    masterGainAttach = std::make_unique<SliderAttach> (p.apvts, "masterGain", masterGainSlider);
    dryWetAttach     = std::make_unique<SliderAttach> (p.apvts, "dryWet", dryWetSlider);

    sourceModeBox.addItemList ({ "Live", "File" }, 1);
    addAndMakeVisible (sourceModeBox);
    sourceModeAttach = std::make_unique<ComboAttach> (p.apvts, "sourceMode", sourceModeBox);

    setSize (1000, 700);
    startTimerHz (30);
}

CerberusGranAudioProcessorEditor::~CerberusGranAudioProcessorEditor()
{
    stopTimer();
}

void CerberusGranAudioProcessorEditor::setupSlider (juce::Slider& s, const juce::String& suffix)
{
    setupRotarySlider (s, suffix);
    addAndMakeVisible (s);
}

void CerberusGranAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a2e));

    // Waveform area
    auto waveformArea = getLocalBounds().removeFromTop (120).reduced (8);

    if (dragOver)
    {
        g.setColour (juce::Colours::white.withAlpha (0.1f));
        g.fillRect (waveformArea);
    }

    g.setColour (juce::Colour (0xff2a2a4e));
    g.fillRect (waveformArea);
    g.setColour (juce::Colours::white.withAlpha (0.3f));
    g.drawRect (waveformArea);

    if (fileLoaded && thumbnail.getTotalLength() > 0.0)
    {
        g.setColour (juce::Colour (0xff4ecdc4));
        thumbnail.drawChannels (g, waveformArea, 0.0, thumbnail.getTotalLength(), 1.0f);
    }
    else
    {
        g.setColour (juce::Colours::white.withAlpha (0.3f));
        g.setFont (14.0f);
        g.drawFittedText ("Drop audio file here | Live mode captures DAW input",
                          waveformArea, juce::Justification::centred, 1);
    }

    // Draw position markers for each head
    for (int i = 0; i < kNumHeads; ++i)
    {
        auto* enabledParam = audioProcessor.apvts.getRawParameterValue ("head" + juce::String (i) + "_enable");
        if (enabledParam == nullptr || enabledParam->load() < 0.5f)
            continue;

        float pos = *audioProcessor.apvts.getRawParameterValue (
            "head" + juce::String (i) + "_position");
        int x = waveformArea.getX() + static_cast<int> (pos * waveformArea.getWidth());

        g.setColour (headColours[i].withAlpha (0.8f));
        g.drawVerticalLine (x, static_cast<float> (waveformArea.getY()),
                           static_cast<float> (waveformArea.getBottom()));

        // Scatter range
        float scatter = *audioProcessor.apvts.getRawParameterValue (
            "head" + juce::String (i) + "_posScatter");
        if (scatter > 0.01f)
        {
            int scatterPx = static_cast<int> (scatter * waveformArea.getWidth());
            g.setColour (headColours[i].withAlpha (0.15f));
            g.fillRect (x - scatterPx, waveformArea.getY(), scatterPx * 2, waveformArea.getHeight());
        }
    }

    // Bottom bar label
    auto bottomBar = getLocalBounds().removeFromBottom (50);
    g.setColour (juce::Colour (0xff12122e));
    g.fillRect (bottomBar);
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.setFont (12.0f);
    g.drawText ("CerberusGran", bottomBar.removeFromLeft (120), juce::Justification::centred);
}

void CerberusGranAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (120); // Waveform
    auto bottomBar = area.removeFromBottom (50);

    // Bottom controls
    auto ctrl = bottomBar.reduced (8, 8);
    ctrl.removeFromLeft (120); // Logo space
    masterGainSlider.setBounds (ctrl.removeFromLeft (65));
    dryWetSlider.setBounds (ctrl.removeFromLeft (65));
    sourceModeBox.setBounds (ctrl.removeFromRight (90).withHeight (24).withY (ctrl.getY() + 6));

    // Head panels in scrollable viewport
    headViewport.setBounds (area);

    int panelHeight = 240;
    int collapsedHeight = 32;
    int totalH = 0;

    for (auto* panel : headPanels)
    {
        int h = panel->isCollapsed() ? collapsedHeight : panelHeight;
        panel->setBounds (0, totalH, headViewport.getWidth() - 14, h);
        totalH += h + 4;
    }

    headContainer.setSize (headViewport.getWidth() - 14, totalH);
}

void CerberusGranAudioProcessorEditor::timerCallback()
{
    repaint (getLocalBounds().removeFromTop (120)); // Just repaint waveform area for position markers
}

bool CerberusGranAudioProcessorEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
    {
        if (f.endsWith (".wav") || f.endsWith (".aif") || f.endsWith (".aiff") ||
            f.endsWith (".mp3") || f.endsWith (".flac") || f.endsWith (".ogg"))
            return true;
    }
    return false;
}

void CerberusGranAudioProcessorEditor::fileDragEnter (const juce::StringArray& files, int, int)
{
    if (isInterestedInFileDrag (files))
    {
        dragOver = true;
        repaint (getLocalBounds().removeFromTop (120));
    }
}

void CerberusGranAudioProcessorEditor::fileDragExit (const juce::StringArray&)
{
    if (dragOver)
    {
        dragOver = false;
        repaint (getLocalBounds().removeFromTop (120));
    }
}

void CerberusGranAudioProcessorEditor::filesDropped (const juce::StringArray& files, int, int)
{
    dragOver = false;

    for (auto& f : files)
    {
        juce::File file (f);
        if (file.existsAsFile())
        {
            audioProcessor.loadSampleFile (file);
            thumbnail.setSource (new juce::FileInputSource (file));
            fileLoaded = true;
            repaint();
            break;
        }
    }
}
