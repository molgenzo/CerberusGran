#pragma once
#include <cmath>

class Bitcrusher
{
public:
    void setBits (float bits) { bitDepth = bits; }
    void setRateReduction (float rate) { rateReduction = rate; }

    void processStereo (float& left, float& right)
    {
        // Bit depth reduction
        float levels = std::pow (2.0f, bitDepth);
        left  = std::round (left * levels) / levels;
        right = std::round (right * levels) / levels;

        // Sample rate reduction via sample-and-hold
        sampleCount++;
        int holdSamples = static_cast<int> (rateReduction);
        if (holdSamples < 1) holdSamples = 1;

        if (sampleCount >= holdSamples)
        {
            sampleCount = 0;
            heldLeft = left;
            heldRight = right;
        }

        left = heldLeft;
        right = heldRight;
    }

    void reset()
    {
        sampleCount = 0;
        heldLeft = 0.0f;
        heldRight = 0.0f;
    }

private:
    float bitDepth = 16.0f;
    float rateReduction = 1.0f;
    int sampleCount = 0;
    float heldLeft = 0.0f;
    float heldRight = 0.0f;
};
