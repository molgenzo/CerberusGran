#include "PluginEditor.h"

// ============================================================================
// HeadStrip — one compact row per grain head
// ============================================================================
static void setupLinearSlider (juce::Slider& s, const juce::String& suffix = "")
{
    s.setSliderStyle (juce::Slider::LinearBar);
    s.setTextBoxIsEditable (true);
    if (suffix.isNotEmpty()) s.setTextValueSuffix (suffix);
}

HeadStrip::HeadStrip (CerberusGranAudioProcessor& p, int headIndex, juce::Colour headColour)
    : processor (p), index (headIndex), colour (headColour)
{
    auto id = [headIndex] (const juce::String& name)
    { return "head" + juce::String (headIndex) + "_" + name; };

    enableBtn.setButtonText (juce::String (headIndex + 1));
    enableBtn.setColour (juce::ToggleButton::tickColourId, headColour);
    addAndMakeVisible (enableBtn);
    enableAttach = std::make_unique<ButtonAttach> (p.apvts, id ("enable"), enableBtn);

    setupLinearSlider (posSlider, " %");
    setupLinearSlider (spreadSlider, " %");
    setupLinearSlider (rateSlider, " ms");
    setupLinearSlider (lengthSlider, " ms");
    setupLinearSlider (pitchSlider, " st");
    setupLinearSlider (gainSlider, " dB");

    addAndMakeVisible (posSlider);
    addAndMakeVisible (spreadSlider);
    addAndMakeVisible (rateSlider);
    addAndMakeVisible (lengthSlider);
    addAndMakeVisible (pitchSlider);
    addAndMakeVisible (gainSlider);

    posAttach    = std::make_unique<SliderAttach> (p.apvts, id ("position"), posSlider);
    spreadAttach = std::make_unique<SliderAttach> (p.apvts, id ("spread"), spreadSlider);
    rateAttach   = std::make_unique<SliderAttach> (p.apvts, id ("rate"), rateSlider);
    lengthAttach = std::make_unique<SliderAttach> (p.apvts, id ("length"), lengthSlider);
    pitchAttach  = std::make_unique<SliderAttach> (p.apvts, id ("pitch"), pitchSlider);
    gainAttach   = std::make_unique<SliderAttach> (p.apvts, id ("gain"), gainSlider);

    shapeBox.addItemList ({ "Hann", "Gauss", "Tukey", "Tri" }, 1);
    addAndMakeVisible (shapeBox);
    shapeAttach = std::make_unique<ComboAttach> (p.apvts, id ("shape"), shapeBox);

    reverseBtn.setButtonText ("R");
    addAndMakeVisible (reverseBtn);
    reverseAttach = std::make_unique<ButtonAttach> (p.apvts, id ("reverse"), reverseBtn);
}

void HeadStrip::paint (juce::Graphics& g)
{
    g.setColour (colour.withAlpha (0.06f));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
    g.setColour (colour.withAlpha (0.25f));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 4.0f, 1.0f);
}

void HeadStrip::resized()
{
    auto area = getLocalBounds().reduced (4, 2);

    enableBtn.setBounds (area.removeFromLeft (36));
    area.removeFromLeft (4);

    int sliderW = (area.getWidth() - 80 - 30 - 20) / 6; // 6 sliders, shape box, reverse btn, gaps

    posSlider.setBounds (area.removeFromLeft (sliderW));
    area.removeFromLeft (2);
    spreadSlider.setBounds (area.removeFromLeft (sliderW));
    area.removeFromLeft (2);
    rateSlider.setBounds (area.removeFromLeft (sliderW));
    area.removeFromLeft (2);
    lengthSlider.setBounds (area.removeFromLeft (sliderW));
    area.removeFromLeft (2);
    pitchSlider.setBounds (area.removeFromLeft (sliderW));
    area.removeFromLeft (2);

    shapeBox.setBounds (area.removeFromLeft (70));
    area.removeFromLeft (2);

    reverseBtn.setBounds (area.removeFromLeft (28));
    area.removeFromLeft (2);

    gainSlider.setBounds (area);
}

// ============================================================================
// Main Editor
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

    for (int i = 0; i < kNumHeadStrips; ++i)
    {
        auto* strip = new HeadStrip (p, i, headColours[i]);
        headStrips.add (strip);
        addAndMakeVisible (strip);
    }

    // Global controls
    setupLinearSlider (masterGainSlider);
    masterGainSlider.setTextValueSuffix ("x");
    addAndMakeVisible (masterGainSlider);
    masterGainAttach = std::make_unique<SliderAttach> (p.apvts, "masterGain", masterGainSlider);

    setupLinearSlider (mixSlider);
    mixSlider.setTextValueSuffix (" %");
    addAndMakeVisible (mixSlider);
    mixAttach = std::make_unique<SliderAttach> (p.apvts, "mix", mixSlider);

    freezeBtn.setButtonText ("Freeze");
    addAndMakeVisible (freezeBtn);
    freezeAttach = std::make_unique<ButtonAttach> (p.apvts, "freeze", freezeBtn);

    sourceModeBox.addItemList ({ "Live", "File" }, 1);
    addAndMakeVisible (sourceModeBox);
    sourceModeAttach = std::make_unique<ComboAttach> (p.apvts, "sourceMode", sourceModeBox);

    setSize (820, 560);
    startTimerHz (30);
}

CerberusGranAudioProcessorEditor::~CerberusGranAudioProcessorEditor()
{
    stopTimer();
}

void CerberusGranAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xfff0ebe3));  // Warm cream background

    // Waveform area
    auto waveformArea = getLocalBounds().removeFromTop (220).reduced (10);

    if (dragOver)
    {
        g.setColour (juce::Colours::black.withAlpha (0.05f));
        g.fillRoundedRectangle (waveformArea.toFloat(), 6.0f);
    }

    g.setColour (juce::Colour (0xffe8e0d4));
    g.fillRoundedRectangle (waveformArea.toFloat(), 6.0f);
    g.setColour (juce::Colour (0xffc0b8a8));
    g.drawRoundedRectangle (waveformArea.toFloat(), 6.0f, 1.0f);

    bool isLive = (audioProcessor.getSourceMode() == AudioSourceMode::Live);

    if (isLive)
    {
        // Draw live scrolling waveform from ring buffer
        auto inner = waveformArea.reduced (4);
        float midY = inner.getCentreY();
        float halfH = inner.getHeight() * 0.5f;
        int w = inner.getWidth();

        juce::Path waveformPath;
        bool pathStarted = false;

        for (int px = 0; px < w; ++px)
        {
            int idx = (px * kWaveformPoints) / w;
            if (idx >= kWaveformPoints) idx = kWaveformPoints - 1;

            float amp = liveWaveform[idx];
            float y = midY - amp * halfH;

            if (!pathStarted)
            {
                waveformPath.startNewSubPath (static_cast<float> (inner.getX() + px), y);
                pathStarted = true;
            }
            else
            {
                waveformPath.lineTo (static_cast<float> (inner.getX() + px), y);
            }
        }

        // Mirror for bottom half
        for (int px = w - 1; px >= 0; --px)
        {
            int idx = (px * kWaveformPoints) / w;
            if (idx >= kWaveformPoints) idx = kWaveformPoints - 1;

            float amp = liveWaveform[idx];
            float y = midY + amp * halfH;
            waveformPath.lineTo (static_cast<float> (inner.getX() + px), y);
        }

        waveformPath.closeSubPath();
        g.setColour (juce::Colour (0xff3a3a3a).withAlpha (0.7f));
        g.fillPath (waveformPath);
        g.setColour (juce::Colour (0xff3a3a3a));
        g.strokePath (waveformPath, juce::PathStrokeType (1.0f));
    }
    else if (fileLoaded && thumbnail.getTotalLength() > 0.0)
    {
        // Draw static file waveform
        g.setColour (juce::Colour (0xff3a3a3a));
        thumbnail.drawChannels (g, waveformArea.reduced (4), 0.0, thumbnail.getTotalLength(), 1.0f);
    }
    else
    {
        g.setColour (juce::Colour (0xff8a8070));
        g.setFont (14.0f);
        g.drawFittedText ("Drop audio file here  |  Live mode captures DAW input",
                          waveformArea, juce::Justification::centred, 1);
    }

    // Draw head position markers on waveform
    for (int i = 0; i < kNumHeadStrips; ++i)
    {
        auto* enabledParam = audioProcessor.apvts.getRawParameterValue ("head" + juce::String (i) + "_enable");
        if (enabledParam == nullptr || enabledParam->load() < 0.5f)
            continue;

        float pos = *audioProcessor.apvts.getRawParameterValue ("head" + juce::String (i) + "_position");
        float normPos = pos / 100.0f;
        int x = waveformArea.getX() + static_cast<int> (normPos * waveformArea.getWidth());

        g.setColour (headColours[i].withAlpha (0.8f));
        g.drawVerticalLine (x, static_cast<float> (waveformArea.getY()),
                           static_cast<float> (waveformArea.getBottom()));

        // Draw play head triangle
        juce::Path triangle;
        float ty = static_cast<float> (waveformArea.getBottom()) - 14.0f;
        triangle.addTriangle (static_cast<float> (x) - 5.0f, ty + 10.0f,
                              static_cast<float> (x) + 5.0f, ty + 10.0f,
                              static_cast<float> (x), ty);
        g.setColour (headColours[i]);
        g.fillPath (triangle);

        // Spread range
        float spread = *audioProcessor.apvts.getRawParameterValue ("head" + juce::String (i) + "_spread");
        if (spread > 0.5f)
        {
            int spreadPx = static_cast<int> ((spread / 100.0f) * waveformArea.getWidth());
            g.setColour (headColours[i].withAlpha (0.12f));
            g.fillRect (x - spreadPx, waveformArea.getY(), spreadPx * 2, waveformArea.getHeight());
        }
    }

    // Column headers above head strips
    auto headerArea = getLocalBounds();
    headerArea.removeFromTop (220);
    auto headerRow = headerArea.removeFromTop (18).reduced (10, 0);

    g.setColour (juce::Colour (0xff6a6050));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));

    headerRow.removeFromLeft (40); // enable btn space
    int colW = (headerRow.getWidth() - 80 - 30 - 20) / 6;

    g.drawText ("position",  headerRow.removeFromLeft (colW), juce::Justification::centred); headerRow.removeFromLeft (2);
    g.drawText ("spread",    headerRow.removeFromLeft (colW), juce::Justification::centred); headerRow.removeFromLeft (2);
    g.drawText ("rate",      headerRow.removeFromLeft (colW), juce::Justification::centred); headerRow.removeFromLeft (2);
    g.drawText ("length",    headerRow.removeFromLeft (colW), juce::Justification::centred); headerRow.removeFromLeft (2);
    g.drawText ("pitch",     headerRow.removeFromLeft (colW), juce::Justification::centred); headerRow.removeFromLeft (2);
    g.drawText ("shape",     headerRow.removeFromLeft (70),   juce::Justification::centred); headerRow.removeFromLeft (2);
    headerRow.removeFromLeft (30); // reverse
    g.drawText ("gain",      headerRow, juce::Justification::centred);

    // Bottom bar background
    auto bottomBar = getLocalBounds().removeFromBottom (44);
    g.setColour (juce::Colour (0xffe0d8cc));
    g.fillRect (bottomBar);
    g.setColour (juce::Colour (0xff6a6050));
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText ("CerberusGran", bottomBar.removeFromLeft (120), juce::Justification::centred);
}

void CerberusGranAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (220);   // waveform
    area.removeFromTop (18);    // column headers

    auto bottomBar = area.removeFromBottom (44);

    // Head strips
    for (auto* strip : headStrips)
    {
        strip->setBounds (area.removeFromTop (36).reduced (10, 2));
    }

    // Bottom bar controls
    auto ctrl = bottomBar.reduced (10, 8);
    ctrl.removeFromLeft (120); // logo space

    auto leftCtrl = ctrl.removeFromLeft (300);
    freezeBtn.setBounds (leftCtrl.removeFromLeft (70));
    leftCtrl.removeFromLeft (8);
    sourceModeBox.setBounds (leftCtrl.removeFromLeft (70).withHeight (24).withY (leftCtrl.getY()));

    auto rightCtrl = ctrl.removeFromRight (240);
    auto gainArea = rightCtrl.removeFromLeft (110);
    masterGainSlider.setBounds (gainArea.withHeight (24).withY (rightCtrl.getY()));
    rightCtrl.removeFromLeft (8);
    mixSlider.setBounds (rightCtrl.withHeight (24).withY (rightCtrl.getY()));
}

void CerberusGranAudioProcessorEditor::updateLiveWaveform()
{
    auto& rb = audioProcessor.getRingBuffer();
    int ringSize = rb.getBufferSize();
    if (ringSize == 0) return;

    int wp = rb.getWritePosition();

    // Read the most recent ringSize samples, downsampled to kWaveformPoints
    int samplesPerPoint = juce::jmax (1, ringSize / kWaveformPoints);

    for (int i = 0; i < kWaveformPoints; ++i)
    {
        // Walk backwards from write position
        int startSample = wp - ringSize + i * samplesPerPoint;

        float maxVal = 0.0f;
        for (int s = 0; s < samplesPerPoint; ++s)
        {
            float v = rb.readSample (0, static_cast<double> (startSample + s));
            float absV = std::fabs (v);
            if (absV > maxVal) maxVal = absV;
        }
        liveWaveform[i] = maxVal;
    }
}

void CerberusGranAudioProcessorEditor::timerCallback()
{
    if (audioProcessor.getSourceMode() == AudioSourceMode::Live)
        updateLiveWaveform();

    repaint (getLocalBounds().removeFromTop (220));
}

bool CerberusGranAudioProcessorEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
        if (f.endsWith (".wav") || f.endsWith (".aif") || f.endsWith (".aiff") ||
            f.endsWith (".mp3") || f.endsWith (".flac") || f.endsWith (".ogg"))
            return true;
    return false;
}

void CerberusGranAudioProcessorEditor::fileDragEnter (const juce::StringArray& files, int, int)
{
    if (isInterestedInFileDrag (files)) { dragOver = true; repaint(); }
}

void CerberusGranAudioProcessorEditor::fileDragExit (const juce::StringArray&)
{
    if (dragOver) { dragOver = false; repaint(); }
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
