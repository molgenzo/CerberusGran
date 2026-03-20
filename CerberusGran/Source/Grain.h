#pragma once
#include <array>

struct Grain
{
    bool active = false;
    double readPosition = 0.0;
    double playbackRate = 1.0;
    int durationSamples = 0;
    int currentSample = 0;
    float amplitude = 1.0f;
    float panLeft = 0.707f;
    float panRight = 0.707f;
    float windowShape = 0.0f;  // 0–1 continuous morph

    void reset()
    {
        active = false;
        readPosition = 0.0;
        playbackRate = 1.0;
        durationSamples = 0;
        currentSample = 0;
        amplitude = 1.0f;
        panLeft = 0.707f;
        panRight = 0.707f;
        windowShape = 0.0f;
    }
};

static constexpr int kMaxGrainsPerHead = 32;

class GrainPool
{
public:
    Grain* acquire()
    {
        for (auto& g : grains)
            if (!g.active) { g.active = true; return &g; }
        return nullptr;
    }

    void release (Grain* g)
    {
        if (g) g->reset();
    }

    Grain* begin() { return grains.data(); }
    Grain* end()   { return grains.data() + grains.size(); }

private:
    std::array<Grain, kMaxGrainsPerHead> grains {};
};
