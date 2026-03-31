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
    }

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
        bool isLive = (processor.getSourceMode() == AudioSourceMode::Live);

        if (isLive)
        {
            float midY = inner.getCentreY();
            float halfH = inner.getHeight() * 0.5f;
            int w = inner.getWidth();

            juce::Path path;
            for (int px = 0; px < w; ++px)
            {
                int idx = (px * kWaveformPoints) / w;
                if (idx >= kWaveformPoints) idx = kWaveformPoints - 1;
                float y = midY - liveWaveform[idx] * halfH;
                if (px == 0) path.startNewSubPath (static_cast<float> (inner.getX() + px), y);
                else path.lineTo (static_cast<float> (inner.getX() + px), y);
            }
            for (int px = w - 1; px >= 0; --px)
            {
                int idx = (px * kWaveformPoints) / w;
                if (idx >= kWaveformPoints) idx = kWaveformPoints - 1;
                path.lineTo (static_cast<float> (inner.getX() + px), midY + liveWaveform[idx] * halfH);
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
            thumbnail.drawChannels (g, inner, 0.0, thumbnail.getTotalLength(), 1.0f);
        }
        else
        {
            g.setColour (juce::Colour (0xff555555));
            g.setFont (13.0f);
            g.drawFittedText ("Drop audio / Live", inner, juce::Justification::centredRight, 1);
        }

        if (dragOver)
        {
            g.setColour (juce::Colours::white.withAlpha (0.05f));
            g.fillRoundedRectangle (bounds, 6.0f);
        }

        // Sync grid lines
        if (processor.anySyncActive.load (std::memory_order_relaxed))
        {
            float gridMs = processor.syncGridMs.load (std::memory_order_relaxed);
            if (gridMs > 0.0f)
            {
                // Grid spacing as fraction of the buffer
                float bufferMs;
                if (isLive)
                    bufferMs = static_cast<float> (processor.getRingBuffer().getBufferSize())
                             / static_cast<float> (processor.getCurrentSampleRate()) * 1000.0f;
                else
                    bufferMs = static_cast<float> (thumbnail.getTotalLength()) * 1000.0f;

                if (bufferMs > 0.0f)
                {
                    float gridFraction = gridMs / bufferMs;
                    int numLines = static_cast<int> (1.0f / gridFraction);
                    numLines = juce::jmin (numLines, 200); // cap to avoid drawing thousands

                    g.setColour (juce::Colour (0xffffffff).withAlpha (0.06f));
                    for (int i = 1; i < numLines; ++i)
                    {
                        float xNorm = i * gridFraction;
                        int x = inner.getX() + static_cast<int> (xNorm * inner.getWidth());
                        g.drawVerticalLine (x, static_cast<float> (inner.getY()),
                                           static_cast<float> (inner.getBottom()));
                    }
                }
            }
        }

        // Head position markers
        for (int i = 0; i < 5; ++i)
        {
            auto* ep = processor.apvts.getRawParameterValue ("head" + juce::String (i) + "_enable");
            if (ep == nullptr || ep->load() < 0.5f) continue;

            float pos = *processor.apvts.getRawParameterValue ("head" + juce::String (i) + "_position");
            float spread = *processor.apvts.getRawParameterValue ("head" + juce::String (i) + "_spread");
            int x = inner.getX() + static_cast<int> ((pos / 100.0f) * inner.getWidth());

            // Spread region (shaded area around position)
            if (spread > 0.1f)
            {
                int spreadPx = static_cast<int> ((spread / 100.0f) * inner.getWidth());
                g.setColour (headColours[i].withAlpha (0.12f));
                g.fillRect (x - spreadPx, inner.getY(), spreadPx * 2, inner.getHeight());
                // Spread edges
                g.setColour (headColours[i].withAlpha (0.2f));
                g.drawVerticalLine (x - spreadPx, static_cast<float> (inner.getY()), static_cast<float> (inner.getBottom()));
                g.drawVerticalLine (x + spreadPx, static_cast<float> (inner.getY()), static_cast<float> (inner.getBottom()));
            }

            // Position line
            g.setColour (headColours[i].withAlpha (0.7f));
            g.drawVerticalLine (x, static_cast<float> (inner.getY()), static_cast<float> (inner.getBottom()));

            // Triangle marker
            juce::Path tri;
            float ty = static_cast<float> (inner.getBottom()) - 10.0f;
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

                float gx = inner.getX() + snap.normPosition * inner.getWidth();
                float gw = snap.normLength * inner.getWidth();
                if (gw < 2.0f) gw = 2.0f;

                // Grain bar: colored rectangle centered vertically in waveform
                float barH = 4.0f;
                float centreY = static_cast<float> (inner.getCentreY());
                float barY = centreY - 12.0f + (gi % 6) * 5.0f; // stagger around centre

                // Fade alpha based on progress (bright at start, fades out)
                float alpha = 0.7f * (1.0f - snap.progress * 0.5f);
                g.setColour (headColours[i].withAlpha (alpha));
                g.fillRoundedRectangle (gx, barY, gw, barH, 1.5f);
            }
        }
    }

    // FileDragAndDropTarget
    bool isInterestedInFileDrag (const juce::StringArray& files) override
    {
        for (auto& f : files)
            if (f.endsWith (".wav") || f.endsWith (".aif") || f.endsWith (".aiff") ||
                f.endsWith (".mp3") || f.endsWith (".flac") || f.endsWith (".ogg"))
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
                processor.loadSampleFile (file);
                thumbnail.setSource (new juce::FileInputSource (file));
                fileLoaded = true;
                repaint();
                break;
            }
        }
    }

private:
    CerberusGranAudioProcessor& processor;
    const std::array<juce::Colour, 5>& headColours;

    juce::AudioThumbnail thumbnail;
    bool fileLoaded = false;
    bool dragOver = false;

    static constexpr int kWaveformPoints = 800;
    std::array<float, kWaveformPoints> liveWaveform {};
};
