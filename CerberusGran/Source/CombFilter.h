#pragma once
#include <JuceHeader.h>


//
// Type mapping (shared with HeadFXChain filterType):
//   0  LP    (standard)
//   1  HP    (standard)
//   2  BP    (standard)
//   3  Comb+ LP   — positive feedback, inner LP
//   4  Comb+ BP   — positive feedback, inner BP
//   5  Comb- LP   — negative feedback, inner LP
//   6  Comb- BP   — negative feedback, inner BP
//   7  Scream LP  — positive feedback + saturation, inner LP
//   8  Scream BP  — positive feedback + saturation, inner BP
class CombFilter
{
public:
    void prepare (double sr, int /*maxBlockSize*/)
    {
        sampleRate = sr;
        int maxD = static_cast<int> (sr * 0.06); // 60 ms max → ~16 Hz lowest comb tuning
        maxDelaySamples = juce::jmax (256, maxD);
        delayLine.setSize (2, maxDelaySamples);
        delayLine.clear();
        writePos = 0;

        juce::dsp::ProcessSpec spec { sr, 512u, 2 };
        innerFilter.prepare (spec);
        innerFilter.reset();
    }

    void reset()
    {
        delayLine.clear();
        writePos = 0;
        innerFilter.reset();
    }

    void setType (int t)        { type = t; }
    void setCombFreq (float hz) { combFreq = juce::jlimit (20.0f, 5000.0f, hz); }
    void setCutoff (float hz)   { cutoff = juce::jlimit (20.0f, 20000.0f, hz); }
    void setResonance (float r) { resonance = r; }
    void setDrive (float d)     { drive = juce::jlimit (0.0f, 1.0f, d); }

    // Returns true if the current type is one of the comb variants (3–8)
    static bool isCombType (int t) { return t >= 3 && t <= 8; }

    void process (juce::AudioBuffer<float>& buffer, int numSamples)
    {
        if (! isCombType (type)) return; // handled externally by standard SVF

        const bool negativeFb = (type == 5 || type == 6);
        const bool bpInside   = (type == 4 || type == 6 || type == 8);
        const bool scream     = (type == 7 || type == 8);

        innerFilter.setCutoffFrequency (cutoff);
        innerFilter.setResonance (juce::jlimit (0.5f, 10.0f, resonance));
        innerFilter.setType (bpInside ? juce::dsp::StateVariableTPTFilterType::bandpass
                                      : juce::dsp::StateVariableTPTFilterType::lowpass);

        float delaySamps = static_cast<float> (sampleRate) / juce::jmax (20.0f, combFreq);
        delaySamps = juce::jmin (delaySamps, static_cast<float> (maxDelaySamples - 2));

        // Map resonance (0.1..10) → feedback amount (0.3..0.97)
        float fbAmount = juce::jlimit (0.0f, 0.97f, 0.3f + resonance * 0.068f);
        if (scream) fbAmount = juce::jmin (0.97f, fbAmount + drive * 0.2f);

        // Scream parameters — deliberately harsh even at drive=0, because
        // the character must be obviously different from a clean comb.
        // At drive=0 we already get moderate fold + pregain; drive pushes it into extreme territory.
        const float screamPreGain = 2.5f + drive * 10.0f;    // already distorting at drive=0
        const float screamBias    = 0.2f  + drive * 0.4f;    // asymmetry → even harmonics
        const float screamFold    = 0.6f  + drive * 2.0f;    // wave-folding threshold
        const float screamPostComp = 1.0f / std::sqrt (screamPreGain);

        // Clean comb: drive is very gentle, barely audible until >50%
        const float combDriveGain = 1.0f + drive * 1.5f;
        const float combDriveComp = 1.0f / std::sqrt (1.0f + drive * 0.5f);

        const int channels = juce::jmin (2, buffer.getNumChannels());

        // Wave-folder: reflect signal back when |x| exceeds threshold.
        // Creates aggressive odd & even harmonics — much harsher than tanh alone.
        // Example: at threshold=1, input 1.5 folds to 0.5 (reflected around threshold).
        auto waveFold = [] (float x, float thresh) -> float
        {
            const float inv2t = 0.5f / thresh;
            // Triangle wave: f(x) = 2 * thresh * |frac((x + thresh) * inv2t) - 0.5| * 2 - thresh
            float normalised = (x + thresh) * inv2t;
            float frac = normalised - std::floor (normalised);
            return (std::abs (frac - 0.5f) * 4.0f - 1.0f) * thresh;
        };

        for (int i = 0; i < numSamples; ++i)
        {
            for (int ch = 0; ch < channels; ++ch)
            {
                float x = buffer.getSample (ch, i);

                // Linear interpolated read from delay line
                double rp = static_cast<double> (writePos) - delaySamps;
                while (rp < 0.0) rp += maxDelaySamples;
                while (rp >= maxDelaySamples) rp -= maxDelaySamples;

                int idx0 = static_cast<int> (rp);
                int idx1 = idx0 + 1;
                if (idx1 >= maxDelaySamples) idx1 -= maxDelaySamples;
                float frac = static_cast<float> (rp - idx0);

                float delayed = delayLine.getSample (ch, idx0) * (1.0f - frac)
                              + delayLine.getSample (ch, idx1) * frac;

                // Inner filter on feedback path
                float filtered = innerFilter.processSample (ch, delayed);

                const float fbSign = negativeFb ? -1.0f : 1.0f;
                float fbVal = fbSign * fbAmount * filtered;

                if (scream)
                {
                    // SCREAM PATH — wave-folding + asymmetric tanh INSIDE feedback loop.
                    // This is fundamentally different from the Comb path:
                    //   - Wave-folder produces harsh, metallic harmonics (not just tanh smoothing)
                    //   - Folded signal re-enters the delay line, so each circulation
                    //     accumulates more harmonics (exponential buildup)
                    //   - Result: distinct "screaming" timbre even at drive=0
                    float pre = (x + fbVal) * screamPreGain + screamBias;
                    float folded = waveFold (pre, screamFold);
                    float shaped = std::tanh (folded * 0.8f) * screamPostComp;

                    // Extreme drive: add hard clip on top for maximum harshness
                    if (drive > 0.7f)
                    {
                        float hardAmt = (drive - 0.7f) * 3.33f; // 0..1 at drive 0.7..1.0
                        float hard = juce::jlimit (-0.9f, 0.9f, shaped * (2.0f + hardAmt * 4.0f));
                        shaped = shaped * (1.0f - hardAmt) + hard * hardAmt;
                    }

                    delayLine.setSample (ch, writePos, shaped);
                }
                else
                {
                    // CLEAN COMB PATH — gentle tanh only on feedback if drive > 0
                    if (drive > 0.01f)
                        fbVal = std::tanh (fbVal * combDriveGain) * combDriveComp;

                    delayLine.setSample (ch, writePos, x + fbVal);
                }

                // Soft-limit to keep runaway resonance bounded
                float out = delayed;
                if (out >  1.5f) out =  1.5f + std::tanh (out - 1.5f);
                if (out < -1.5f) out = -1.5f + std::tanh (out + 1.5f);

                buffer.setSample (ch, i, out);
            }

            if (++writePos >= maxDelaySamples) writePos = 0;
        }
    }

private:
    double sampleRate = 44100.0;
    juce::AudioBuffer<float> delayLine;
    int writePos = 0;
    int maxDelaySamples = 0;
    juce::dsp::StateVariableTPTFilter<float> innerFilter;

    int   type      = 3;
    float combFreq  = 440.0f;
    float cutoff    = 2000.0f;
    float resonance = 0.7f;
    float drive     = 0.0f;
};