#pragma once
#include <JuceHeader.h>
#include <vector>
#include <mutex>
#include <algorithm>
#include "LFO.h"
#include "StepSequencer.h"

class ModulationEngine
{
public:
    // Source indices
    static constexpr int kLFO = 0;
    static constexpr int kStepSeq = 1;
    static constexpr int kNumSources = 2;

    struct Connection
    {
        int sourceIndex;
        juce::String destParamId;
        float amount; // -1..1
    };

    ModulationEngine() = default;

    void prepare (double sr)
    {
        lfo.prepare (sr);
        stepSeq.prepare (sr);
    }

    // Called once per audio block. Advances all sources.
    void tick (int numSamples)
    {
        lfo.advance (numSamples);
        stepSeq.advance (numSamples);
    }

    float getSourceOutput (int sourceIndex) const
    {
        switch (sourceIndex)
        {
            case kLFO:     return lfo.getOutput();
            case kStepSeq: return stepSeq.getOutput();
            default:       return 0.0f;
        }
    }

    // Apply modulation on top of a base parameter value.
    // range = paramMax - paramMin. Result clamped to [paramMin, paramMax].
    float applyMod (const juce::String& paramId, float baseValue,
                    float paramMin, float paramMax) const
    {
        std::lock_guard<std::mutex> lock (connMutex);
        if (connections.empty())
            return juce::jlimit (paramMin, paramMax, baseValue);

        float modSum = 0.0f;
        for (const auto& c : connections)
        {
            if (c.destParamId == paramId)
                modSum += getSourceOutput (c.sourceIndex) * c.amount;
        }

        if (modSum == 0.0f)
            return juce::jlimit (paramMin, paramMax, baseValue);

        float range = paramMax - paramMin;
        return juce::jlimit (paramMin, paramMax, baseValue + modSum * range);
    }

    // -------- UI/message thread operations --------

    void addOrUpdateConnection (int sourceIndex, const juce::String& destParamId, float amount)
    {
        std::lock_guard<std::mutex> lock (connMutex);
        for (auto& c : connections)
        {
            if (c.sourceIndex == sourceIndex && c.destParamId == destParamId)
            {
                c.amount = amount;
                return;
            }
        }
        connections.push_back ({ sourceIndex, destParamId, amount });
    }

    void removeConnection (int sourceIndex, const juce::String& destParamId)
    {
        std::lock_guard<std::mutex> lock (connMutex);
        connections.erase (
            std::remove_if (connections.begin(), connections.end(),
                [&] (const Connection& c) {
                    return c.sourceIndex == sourceIndex && c.destParamId == destParamId;
                }),
            connections.end());
    }

    float getConnectionAmount (int sourceIndex, const juce::String& destParamId) const
    {
        std::lock_guard<std::mutex> lock (connMutex);
        for (const auto& c : connections)
            if (c.sourceIndex == sourceIndex && c.destParamId == destParamId)
                return c.amount;
        return 0.0f;
    }

    bool hasConnection (int sourceIndex, const juce::String& destParamId) const
    {
        std::lock_guard<std::mutex> lock (connMutex);
        for (const auto& c : connections)
            if (c.sourceIndex == sourceIndex && c.destParamId == destParamId)
                return true;
        return false;
    }

    // Returns all connections that target paramId (for drawing aggregated rings)
    std::vector<Connection> getConnectionsForParam (const juce::String& paramId) const
    {
        std::vector<Connection> result;
        std::lock_guard<std::mutex> lock (connMutex);
        for (const auto& c : connections)
            if (c.destParamId == paramId)
                result.push_back (c);
        return result;
    }

    std::vector<Connection> getAllConnections() const
    {
        std::lock_guard<std::mutex> lock (connMutex);
        return connections;
    }

    void clearConnections()
    {
        std::lock_guard<std::mutex> lock (connMutex);
        connections.clear();
    }

    // Public for direct access from audio thread setters
    LFO lfo;
    StepSequencer stepSeq;

private:
    mutable std::mutex connMutex;
    std::vector<Connection> connections;
};
