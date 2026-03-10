#pragma once
#include <cmath>

class Bitcrusher
{
public:
    void prepare (double sampleRate)
    {
        this->sampleRate = sampleRate;
        holdCounter = 0;
        holdSampleL = 0.0f;
        holdSampleR = 0.0f;
    }

    void setBitDepth (float bits)    { bitDepth = bits; }
    void setRateReduction (float r)  { rateReduction = r; }

    void processStereo (float& left, float& right)
    {
        // Sample rate reduction
        holdCounter++;
        int holdLength = static_cast<int> (sampleRate / (sampleRate / rateReduction));
        if (holdLength < 1) holdLength = 1;

        if (holdCounter >= holdLength)
        {
            holdCounter = 0;

            // Bit depth quantization
            float levels = std::pow (2.0f, bitDepth);
            holdSampleL = std::round (left * levels) / levels;
            holdSampleR = std::round (right * levels) / levels;
        }

        left = holdSampleL;
        right = holdSampleR;
    }

private:
    double sampleRate = 44100.0;
    float bitDepth = 16.0f;
    float rateReduction = 1.0f;

    int holdCounter = 0;
    float holdSampleL = 0.0f;
    float holdSampleR = 0.0f;
};
