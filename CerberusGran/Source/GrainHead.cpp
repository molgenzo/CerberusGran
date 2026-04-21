#include "GrainHead.h"
#include "RingBuffer.h"

GrainHead::GrainHead() = default;

void GrainHead::prepare (double sr, int blockSize)
{
    sampleRate = sr;
    headBuffer.setSize (2, blockSize);
    fxChain.prepare (sr, blockSize);
}

void GrainHead::spawnGrain (const RingBuffer& ringBuffer,
                            const juce::AudioBuffer<float>* sampleBuffer,
                            bool liveMode)
{
    Grain* g = grainPool.acquire();
    if (g == nullptr) return;

    float pos;

    if (syncGrid && syncGridMs > 0.0f)
    {
        // Grid spacing as percentage of buffer
        float bufSamples = liveMode ? static_cast<float> (ringBuffer.getBufferSize())
                                    : (sampleBuffer ? static_cast<float> (sampleBuffer->getNumSamples()) : 1.0f);
        float gridPct = (syncGridMs * 0.001f * static_cast<float> (sampleRate)) / bufSamples * 100.0f;

        if (gridPct > 0.01f)
        {
            // Snap base position to nearest grid line
            float snappedPos = std::round (position / gridPct) * gridPct;

            // Spread: offset by random grid steps from snapped position
            if (spread > 0.0f)
            {
                int maxSteps = juce::jmax (1, static_cast<int> (spread / gridPct));
                int step = rng.nextInt (maxSteps * 2 + 1) - maxSteps;
                snappedPos += step * gridPct;
            }

            pos = juce::jlimit (0.0f, 100.0f, snappedPos) / 100.0f;
        }
        else
        {
            pos = juce::jlimit (0.0f, 100.0f, position) / 100.0f;
        }
    }
    else
    {
        // Free mode: random scatter
        float scatterAmt = spread * (rng.nextFloat() * 2.0f - 1.0f);
        pos = juce::jlimit (0.0f, 100.0f, position + scatterAmt) / 100.0f;
    }

    if (liveMode)
    {
        int ringSize = ringBuffer.getBufferSize();
        int wp = ringBuffer.getWritePosition();
        g->readPosition = static_cast<double> (wp) - pos * ringSize;
        if (g->readPosition < 0.0)
            g->readPosition += ringSize;
    }
    else if (sampleBuffer != nullptr && sampleBuffer->getNumSamples() > 0)
    {
        g->readPosition = pos * (sampleBuffer->getNumSamples() - 1);
    }
    else
    {
        grainPool.release (g);
        return;
    }

    double rate = std::pow (2.0, static_cast<double> (pitchSt) / 12.0);
    bool thisGrainReverse = (rng.nextFloat() * 100.0f) < reversePct;
    g->playbackRate = thisGrainReverse ? -rate : rate;

    g->durationSamples = static_cast<int> (lengthMs * 0.001 * sampleRate);
    g->currentSample = 0;

    // Overlap compensation: scale amplitude by 1/sqrt(overlapCount)
    // to prevent volume buildup when grain length > grain rate
    float overlapCount = (rateMs > 0.0f) ? (lengthMs / rateMs) : 1.0f;
    if (overlapCount < 1.0f) overlapCount = 1.0f;
    g->amplitude = gainLinear / std::sqrt (overlapCount);
    g->windowShape = shape;
    g->spawnNormPos = pos; // store normalized spawn position for UI
    g->panLeft = 0.707f;
    g->panRight = 0.707f;
}

void GrainHead::process (juce::AudioBuffer<float>& output, int numSamples,
                         const RingBuffer& ringBuffer,
                         const juce::AudioBuffer<float>* sampleBuffer,
                         bool liveMode)
{
    if (!enabled) return;

    // Clear per-head intermediate buffer
    headBuffer.clear (0, numSamples);

    // Schedule new grains
    for (int i = 0; i < numSamples; ++i)
    {
        if (--samplesUntilNextGrain <= 0)
        {
            spawnGrain (ringBuffer, sampleBuffer, liveMode);
            int interval = static_cast<int> (rateMs * 0.001 * sampleRate);
            samplesUntilNextGrain = juce::jmax (1, interval);
        }
    }

    // Synthesise active grains into headBuffer
    for (Grain* g = grainPool.begin(); g != grainPool.end(); ++g)
    {
        if (!g->active) continue;

        for (int i = 0; i < numSamples; ++i)
        {
            if (g->currentSample >= g->durationSamples)
            {
                grainPool.release (g);
                break;
            }

            float phase = static_cast<float> (g->currentSample)
                        / static_cast<float> (g->durationSamples);
            float window = WindowFunctions::getValue (g->windowShape, phase);

            float sampleL = 0.0f, sampleR = 0.0f;

            if (liveMode)
            {
                double rp = g->readPosition;
                int ringSize = ringBuffer.getBufferSize();
                while (rp < 0.0) rp += ringSize;
                while (rp >= ringSize) rp -= ringSize;

                sampleL = ringBuffer.readSample (0, rp);
                sampleR = ringBuffer.readSample (juce::jmin (1, ringBuffer.getNumChannels() - 1), rp);
            }
            else if (sampleBuffer != nullptr)
            {
                int bufLen = sampleBuffer->getNumSamples();
                double rp = g->readPosition;

                if (rp < 0.0) rp = 0.0;
                if (rp >= bufLen - 1) rp = bufLen - 1.001;

                int idx0 = static_cast<int> (rp);
                int idx1 = idx0 + 1;
                float frac = static_cast<float> (rp - idx0);

                idx0 = juce::jlimit (0, bufLen - 1, idx0);
                idx1 = juce::jlimit (0, bufLen - 1, idx1);

                sampleL = sampleBuffer->getSample (0, idx0) * (1.0f - frac)
                        + sampleBuffer->getSample (0, idx1) * frac;
                int rCh = juce::jmin (1, sampleBuffer->getNumChannels() - 1);
                sampleR = sampleBuffer->getSample (rCh, idx0) * (1.0f - frac)
                        + sampleBuffer->getSample (rCh, idx1) * frac;
            }

            float mono = (sampleL + sampleR) * 0.5f * window * g->amplitude;

            // Write to per-head buffer (not shared output)
            headBuffer.addSample (0, i, mono);
            if (headBuffer.getNumChannels() > 1)
                headBuffer.addSample (1, i, mono);

            g->readPosition += g->playbackRate;
            g->currentSample++;
        }
    }

    // Update grain snapshots for UI visualization
    {
        int bufSize = liveMode ? ringBuffer.getBufferSize()
                               : (sampleBuffer ? sampleBuffer->getNumSamples() : 1);
        int count = 0;

        for (Grain* g = grainPool.begin(); g != grainPool.end() && count < kMaxSnapshotGrains; ++g)
        {
            if (!g->active) continue;

            auto& snap = grainSnapshots[count];

            // Use the stored spawn position so grains stay near the head
            // marker regardless of playback direction (forward or reverse)
            snap.normPosition = g->spawnNormPos;
            snap.normLength = static_cast<float> (g->durationSamples) / static_cast<float> (bufSize);
            snap.progress = static_cast<float> (g->currentSample) / static_cast<float> (juce::jmax (1, g->durationSamples));
            snap.active = true;
            count++;
        }

        for (int i = count; i < kMaxSnapshotGrains; ++i)
            grainSnapshots[i].active = false;

        activeGrainCount.store (count, std::memory_order_relaxed);
    }

    // Apply FX chain to per-head buffer
    fxChain.process (headBuffer, numSamples);
    DBG ("Output Channels: " << output.getNumChannels() << " | HeadBuffer Channels: " << headBuffer.getNumChannels());
    // Mix processed head buffer into shared output
    for (int ch = 0; ch < juce::jmin (output.getNumChannels(), headBuffer.getNumChannels()); ++ch)
        output.addFrom (ch, 0, headBuffer, ch, 0, numSamples);
}
