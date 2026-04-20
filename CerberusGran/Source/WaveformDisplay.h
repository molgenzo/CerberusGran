#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class WaveformDisplay : public juce::Component,
                        public juce::FileDragAndDropTarget
{
public:
    WaveformDisplay (CerberusGranAudioProcessor& p,
                     const std::array<juce::Colour, 5>& colours)
        : processor (p), headColours (colours),
          thumbnail (512, p.getFormatManager(), p.getThumbnailCache())
    {
        browseButton.setButtonText ("Browse...");
        browseButton.onClick = [this] { openFileChooser(); };
        addAndMakeVisible (browseButton);

        clearButton.setButtonText ("Clear");
        clearButton.onClick = [this] { clearLoadedFile(); };
        addAndMakeVisible (clearButton);

        updateSourceModeUi();
    }

    void setActiveHead (int h) { activeHeadIndex = juce::jlimit (0, 4, h); }

    void updateLiveWaveform()
    {
        auto& rb = processor.getRingBuffer();
        int ringSize = rb.getBufferSize();
        if (ringSize == 0) return;

        int wp = rb.getWritePosition();
        int samplesPerPoint = juce::jmax (1, ringSize / kWaveformPoints);

        for (int i = 0; i < kWaveformPoints; ++i)
        {
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

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        g.setColour (juce::Colour (0xff222226));
        g.fillRoundedRectangle (bounds, 6.0f);
        g.setColour (juce::Colour (0xff3A3A40));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 0.5f);

        auto inner = getLocalBounds().reduced (6);
        auto statusArea = inner.removeFromBottom (24);
        auto statusTextArea = statusArea.withTrimmedRight (156);
        auto waveformArea = inner;
        bool isLive = (processor.getSourceMode() == AudioSourceMode::Live);

        if (isLive)
        {
            float midY = waveformArea.getCentreY();
            float halfH = waveformArea.getHeight() * 0.5f;
            int w = waveformArea.getWidth();

            juce::Path path;
            for (int px = 0; px < w; ++px)
            {
                int idx = (px * kWaveformPoints) / w;
                if (idx >= kWaveformPoints) idx = kWaveformPoints - 1;
                float y = midY - liveWaveform[idx] * halfH;
                if (px == 0) path.startNewSubPath (static_cast<float> (waveformArea.getX() + px), y);
                else path.lineTo (static_cast<float> (waveformArea.getX() + px), y);
            }
            for (int px = w - 1; px >= 0; --px)
            {
                int idx = (px * kWaveformPoints) / w;
                if (idx >= kWaveformPoints) idx = kWaveformPoints - 1;
                path.lineTo (static_cast<float> (waveformArea.getX() + px), midY + liveWaveform[idx] * halfH);
            }
            path.closeSubPath();

            g.setColour (juce::Colour (0xff444450).withAlpha (0.6f));
            g.fillPath (path);
            g.setColour (juce::Colour (0xff666680));
            g.strokePath (path, juce::PathStrokeType (0.5f));
        }
        else if (fileLoaded && thumbnail.getTotalLength() > 0.0)
        {
            g.setColour (juce::Colour (0xff666680));
            thumbnail.drawChannels (g, waveformArea, 0.0, thumbnail.getTotalLength(), 1.0f);
        }
        else
        {
            g.setColour (juce::Colour (0xff555555));
            g.setFont (13.0f);
            g.drawFittedText (isLive ? "Live input mode" : "Drop audio or click Browse", waveformArea, juce::Justification::centred, 1);
        }

        if (!isLive)
        {
            g.setColour (juce::Colour (0xff8E8E96));
            g.setFont (11.0f);
            const auto statusText = loadError.isNotEmpty()
                ? ("Load failed: " + loadError)
                : (fileLoaded ? ("Loaded: " + loadedFileName) : "No file loaded");
            g.drawFittedText (statusText, statusTextArea, juce::Justification::centredLeft, 1);
        }

        if (dragOver)
        {
            g.setColour (juce::Colours::white.withAlpha (0.05f));
            g.fillRoundedRectangle (bounds, 6.0f);
        }

        // Sync grid lines — evenly spaced across the entire waveform display
        if (processor.anySyncActive.load (std::memory_order_relaxed))
        {
            float gridMs = processor.syncGridMs.load (std::memory_order_relaxed);
            if (gridMs > 0.0f)
            {
                float sr = static_cast<float> (processor.getCurrentSampleRate());
                float bufferSamples = isLive
                    ? static_cast<float> (processor.getRingBuffer().getBufferSize())
                    : (thumbnail.getTotalLength() > 0 ? static_cast<float> (thumbnail.getTotalLength()) * sr : 1.0f);

                float gridSamples = gridMs * 0.001f * sr;
                if (gridSamples > 0.0f && bufferSamples > 0.0f)
                {
                    float pixelsPerGrid = (gridSamples / bufferSamples) * static_cast<float> (waveformArea.getWidth());

                    // Only draw if lines won't be too dense (min 1px apart)
                    if (pixelsPerGrid >= 1.0f)
                    {
                        g.setColour (juce::Colour (0xffffffff).withAlpha (0.07f));
                        for (float px = pixelsPerGrid; px < static_cast<float> (waveformArea.getWidth()); px += pixelsPerGrid)
                        {
                            int x = waveformArea.getX() + static_cast<int> (px);
                            g.drawVerticalLine (x, static_cast<float> (waveformArea.getY()),
                                               static_cast<float> (waveformArea.getBottom()));
                        }
                    }
                }
            }
        }

        // Head position markers — draw active head last (on top)
        // First pass: inactive heads. Second pass: active head.
        for (int pass = 0; pass < 2; ++pass)
        for (int i = 0; i < 5; ++i)
        {
            bool isActive = (i == activeHeadIndex);
            if ((pass == 0 && isActive) || (pass == 1 && !isActive)) continue;

            auto* ep = processor.apvts.getRawParameterValue ("head" + juce::String (i) + "_enable");
            if (ep == nullptr || ep->load() < 0.5f) continue;

            float pos = *processor.apvts.getRawParameterValue ("head" + juce::String (i) + "_position");
            float spread = *processor.apvts.getRawParameterValue ("head" + juce::String (i) + "_spread");

            // Snap position to grid when this head is in sync mode
            int rateMode = static_cast<int> (*processor.apvts.getRawParameterValue ("head" + juce::String (i) + "_rateMode"));
            if (rateMode == 1) // Sync
            {
                float gridMs = processor.syncGridMs.load (std::memory_order_relaxed);
                if (gridMs > 0.0f)
                {
                    float bufferMs;
                    if (isLive)
                        bufferMs = static_cast<float> (processor.getRingBuffer().getBufferSize())
                                 / static_cast<float> (processor.getCurrentSampleRate()) * 1000.0f;
                    else
                        bufferMs = static_cast<float> (thumbnail.getTotalLength()) * 1000.0f;

                    if (bufferMs > 0.0f)
                    {
                        float gridPct = (gridMs / bufferMs) * 100.0f;
                        if (gridPct > 0.01f)
                            pos = std::round (pos / gridPct) * gridPct;
                    }
                }
            }

            int x = waveformArea.getX() + static_cast<int> ((pos / 100.0f) * waveformArea.getWidth());

            // Spread region
            if (spread > 0.1f)
            {
                int spreadPx = static_cast<int> ((spread / 100.0f) * waveformArea.getWidth());

                if (isActive)
                {
                    // Prominent bounding box for active head
                    auto boxRect = juce::Rectangle<float> (
                        static_cast<float> (x - spreadPx), static_cast<float> (waveformArea.getY()),
                        static_cast<float> (spreadPx * 2), static_cast<float> (waveformArea.getHeight()));
                    g.setColour (headColours[i].withAlpha (0.2f));
                    g.fillRoundedRectangle (boxRect, 3.0f);
                    g.setColour (headColours[i].withAlpha (0.6f));
                    g.drawRoundedRectangle (boxRect.reduced (0.5f), 3.0f, 1.5f);
                }
                else
                {
                    g.setColour (headColours[i].withAlpha (0.08f));
                    g.fillRect (x - spreadPx, waveformArea.getY(), spreadPx * 2, waveformArea.getHeight());
                    g.setColour (headColours[i].withAlpha (0.15f));
                    g.drawVerticalLine (x - spreadPx, static_cast<float> (waveformArea.getY()), static_cast<float> (waveformArea.getBottom()));
                    g.drawVerticalLine (x + spreadPx, static_cast<float> (waveformArea.getY()), static_cast<float> (waveformArea.getBottom()));
                }
            }
            // Position line
            float lineAlpha = isActive ? 0.9f : 0.5f;
            g.setColour (headColours[i].withAlpha (lineAlpha));
            g.drawVerticalLine (x, static_cast<float> (waveformArea.getY()), static_cast<float> (waveformArea.getBottom()));

            // Triangle marker
            juce::Path tri;
            float ty = static_cast<float> (waveformArea.getBottom()) - 10.0f;
            tri.addTriangle (x - 4.0f, ty + 8.0f, x + 4.0f, ty + 8.0f, static_cast<float> (x), ty);
            g.setColour (headColours[i]);
            g.fillPath (tri);

            // Draw active grain playheads for this head
            auto& head = processor.getGrainEngine().getHead (i);
            int grainCount = head.getActiveGrainCount();
            auto& snapshots = head.getGrainSnapshots();

            for (int gi = 0; gi < grainCount; ++gi)
            {
                auto& snap = snapshots[gi];
                if (!snap.active) continue;

                float gx = waveformArea.getX() + snap.normPosition * waveformArea.getWidth();
                float gw = snap.normLength * waveformArea.getWidth();
                if (gw < 2.0f) gw = 2.0f;

                // Grain bar: colored rectangle centered vertically in waveform
                float barH = 4.0f;
                float centreY = static_cast<float> (waveformArea.getCentreY());
                float barY = centreY - 12.0f + (gi % 6) * 5.0f; // stagger around centre

                // Fade alpha based on progress (bright at start, fades out)
                float alpha = 0.7f * (1.0f - snap.progress * 0.5f);
                g.setColour (headColours[i].withAlpha (alpha));
                g.fillRoundedRectangle (gx, barY, gw, barH, 1.5f);
            }
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (10, 8);
        auto bottomBar = area.removeFromBottom (24);
        bottomBar.translate (0, 3);
        clearButton.setBounds (bottomBar.removeFromRight (60));
        bottomBar.removeFromRight (6);
        browseButton.setBounds (bottomBar.removeFromRight (84));
    }

    void updateSourceModeUi()
    {
        const bool isFileMode = (processor.getSourceMode() == AudioSourceMode::File);
        browseButton.setVisible (isFileMode);
        clearButton.setVisible (isFileMode);
        browseButton.setEnabled (isFileMode);
        clearButton.setEnabled (isFileMode);
    }

    // FileDragAndDropTarget
    bool isInterestedInFileDrag (const juce::StringArray& files) override
    {
        if (processor.getSourceMode() == AudioSourceMode::Live)
            return false;

        for (auto& f : files)
            if (juce::File (f).hasFileExtension ("wav;aif;aiff;mp3;flac;ogg"))
                return true;
        return false;
    }

    void fileDragEnter (const juce::StringArray& files, int, int) override
    {
        if (isInterestedInFileDrag (files)) { dragOver = true; repaint(); }
    }

    void fileDragExit (const juce::StringArray&) override
    {
        if (dragOver) { dragOver = false; repaint(); }
    }

    void filesDropped (const juce::StringArray& files, int, int) override
    {
        dragOver = false;
        for (auto& f : files)
        {
            juce::File file (f);
            if (file.existsAsFile())
            {
                handleFileLoad (file);
                break;
            }
        }
    }

    void openFileChooser()
    {
        if (processor.getSourceMode() == AudioSourceMode::Live)
            return;

        fileChooser = std::make_unique<juce::FileChooser> (
            "Select an audio file",
            juce::File::getSpecialLocation (juce::File::userMusicDirectory),
            "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");

        const auto flags = juce::FileBrowserComponent::openMode
                         | juce::FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
        {
            auto file = chooser.getResult();
            if (file != juce::File() && file.existsAsFile())
                handleFileLoad (file);
            fileChooser.reset();
        });
    }

    void handleFileLoad (const juce::File& file)
    {
        if (!file.existsAsFile())
        {
            loadError = "File does not exist";
            fileLoaded = false;
            repaint();
            return;
        }

        if (!processor.loadSampleFile (file))
        {
            loadError = "Unsupported or unreadable format";
            fileLoaded = false;
            repaint();
            return;
        }

        thumbnail.setSource (new juce::FileInputSource (file));
        fileLoaded = true;
        loadedFileName = file.getFileName();
        loadError.clear();

        if (auto* p = processor.apvts.getParameter ("sourceMode"))
            p->setValueNotifyingHost (1.0f);

        updateSourceModeUi();
        repaint();
    }

    void clearLoadedFile()
    {
        processor.clearSampleFile();
        thumbnail.clear();
        fileLoaded = false;
        loadedFileName.clear();
        loadError.clear();
        repaint();
    }

private:
    CerberusGranAudioProcessor& processor;
    const std::array<juce::Colour, 5>& headColours;

    juce::AudioThumbnail thumbnail;
    juce::TextButton browseButton, clearButton;
    std::unique_ptr<juce::FileChooser> fileChooser;
    bool fileLoaded = false;
    bool dragOver = false;
    juce::String loadedFileName;
    juce::String loadError;
    int activeHeadIndex = 0;

    static constexpr int kWaveformPoints = 800;
    std::array<float, kWaveformPoints> liveWaveform {};
};
