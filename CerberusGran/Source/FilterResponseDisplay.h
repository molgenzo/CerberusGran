#pragma once
#include <JuceHeader.h>


class FilterResponseDisplay : public juce::Component,
                              private juce::Timer
{
public:
    FilterResponseDisplay (juce::AudioProcessorValueTreeState& state,
                           int headIndex,
                           juce::Colour accent)
        : apvts (state), head (headIndex), accentColour (accent)
    {
        // Timer is started lazily when the component becomes visible
    }

    ~FilterResponseDisplay() override { stopTimer(); }

    void setSampleRate (double sr) { sampleRate = sr; }

    void visibilityChanged() override
    {
        if (isVisible())
        {
            if (! isTimerRunning())
                startTimerHz (20);
        }
        else
        {
            stopTimer();
        }
    }

    void parentHierarchyChanged() override
    {
        // If a parent becomes hidden, suspend updates
        if (! isShowing()) stopTimer();
        else if (isVisible() && ! isTimerRunning()) startTimerHz (20);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Dark gradient background
        juce::ColourGradient bgGrad (juce::Colour (0xff1c1c24), bounds.getCentreX(), bounds.getY(),
                                     juce::Colour (0xff15151b), bounds.getCentreX(), bounds.getBottom(), false);
        g.setGradientFill (bgGrad);
        g.fillRoundedRectangle (bounds, 3.0f);

        // Diagonal stripe texture
        {
            juce::Graphics::ScopedSaveState save (g);
            juce::Path clip;
            clip.addRoundedRectangle (bounds, 3.0f);
            g.reduceClipRegion (clip);

            g.setColour (juce::Colour (0x14ffffff));
            const float spacing = 5.0f;
            for (float x = -bounds.getHeight(); x < bounds.getWidth() + bounds.getHeight(); x += spacing)
            {
                juce::Path p;
                p.startNewSubPath (bounds.getX() + x, bounds.getBottom());
                p.lineTo (bounds.getX() + x + bounds.getHeight(), bounds.getY());
                g.strokePath (p, juce::PathStrokeType (0.7f));
            }
        }

        g.setColour (juce::Colour (0xff3A3A40));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 3.0f, 0.5f);

        // Read parameters
        auto getParam = [&] (const juce::String& n) -> float
        {
            if (auto* v = apvts.getRawParameterValue ("head" + juce::String (head) + "_" + n))
                return v->load();
            return 0.0f;
        };

        const int   type     = static_cast<int> (getParam ("filterType"));
        const float cutoff   = getParam ("filterCutoff");
        const float res      = getParam ("filterRes");
        const float combFreq = getParam ("filterCombFreq");
        const float drive    = getParam ("filterDrive");
        const bool  filterOn = getParam ("filterOn") >= 0.5f;

        auto inner = bounds.reduced (4.0f, 3.0f);
        const int numPts = static_cast<int> (inner.getWidth());
        constexpr float fMin = 20.0f;
        constexpr float fMax = 20000.0f;

        auto magToY = [&] (float mag) -> float
        {
            float db = 20.0f * std::log10 (juce::jmax (1.0e-6f, mag));
            constexpr float maxDb =  18.0f;
            constexpr float minDb = -48.0f;
            float norm = juce::jlimit (0.0f, 1.0f, (db - minDb) / (maxDb - minDb));
            return inner.getBottom() - norm * inner.getHeight();
        };

        juce::Path curve;
        for (int i = 0; i <= numPts; ++i)
        {
            float t = static_cast<float> (i) / static_cast<float> (numPts);
            float freq = fMin * std::pow (fMax / fMin, t);
            float mag  = computeMagnitude (freq, type, cutoff, res, combFreq, drive);
            float x = inner.getX() + static_cast<float> (i);
            float y = magToY (mag);
            if (i == 0) curve.startNewSubPath (x, y);
            else        curve.lineTo (x, y);
        }

        float alpha = filterOn ? 1.0f : 0.35f;

        // Fill under curve
        juce::Path filled = curve;
        filled.lineTo (inner.getRight(), inner.getBottom());
        filled.lineTo (inner.getX(),     inner.getBottom());
        filled.closeSubPath();

        juce::ColourGradient fillGrad (accentColour.withAlpha (0.25f * alpha),
                                       inner.getCentreX(), inner.getY(),
                                       accentColour.withAlpha (0.02f * alpha),
                                       inner.getCentreX(), inner.getBottom(), false);
        g.setGradientFill (fillGrad);
        g.fillPath (filled);

        // Glow + curve
        g.setColour (accentColour.withAlpha (0.25f * alpha));
        g.strokePath (curve, juce::PathStrokeType (3.0f));
        g.setColour (accentColour.withAlpha (0.95f * alpha).brighter (0.15f));
        g.strokePath (curve, juce::PathStrokeType (1.3f));
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    int head;
    juce::Colour accentColour;
    double sampleRate = 44100.0;

    void timerCallback() override
    {
        if (isShowing())
            repaint();
        else
            stopTimer();
    }

    // Analytic magnitude response. 2nd-order textbook formulas for standard
    // filters, classic comb response multiplied by an inner LP/BP envelope
    // for comb variants.
    float computeMagnitude (float freq, int type, float cutoff, float res,
                            float combFreq, float drive) const
    {
        constexpr float twoPi = juce::MathConstants<float>::twoPi;

        const float q = juce::jlimit (0.5f, 10.0f, res);

        auto lp2 = [&] (float fc) -> float
        {
            float r = freq / juce::jmax (1.0f, fc);
            float a = 1.0f - r * r;
            float b = r / q;
            return 1.0f / juce::jmax (1.0e-6f, std::sqrt (a * a + b * b));
        };
        auto hp2 = [&] (float fc) -> float
        {
            float r = freq / juce::jmax (1.0f, fc);
            float a = 1.0f - r * r;
            float b = r / q;
            return (r * r) / juce::jmax (1.0e-6f, std::sqrt (a * a + b * b));
        };
        auto bp2 = [&] (float fc) -> float
        {
            float r = freq / juce::jmax (1.0f, fc);
            float a = 1.0f - r * r;
            float b = r / q;
            return b / juce::jmax (1.0e-6f, std::sqrt (a * a + b * b));
        };

        if (type == 0) return lp2 (cutoff);
        if (type == 1) return hp2 (cutoff);
        if (type == 2) return bp2 (cutoff);

        // Comb variants (3..8)
        const bool neg    = (type == 5 || type == 6);
        const bool bpIn   = (type == 4 || type == 6 || type == 8);
        const bool scream = (type == 7 || type == 8);

        float gFb = juce::jlimit (0.0f, 0.97f, 0.3f + res * 0.068f);
        if (scream) gFb = juce::jmin (0.97f, gFb + drive * 0.15f);

        const float delaySamps = static_cast<float> (sampleRate) / juce::jmax (20.0f, combFreq);
        const float w = twoPi * freq / static_cast<float> (sampleRate);
        const float wd = w * delaySamps;
        const float sign = neg ? -1.0f : 1.0f;

        float combMag = 1.0f / juce::jmax (1.0e-6f,
            std::sqrt (1.0f - 2.0f * sign * gFb * std::cos (wd) + gFb * gFb));
        combMag = juce::jmin (combMag, 6.0f);

        const float inner = bpIn ? bp2 (cutoff) : lp2 (cutoff);

        return combMag * inner * 0.45f;
    }
};