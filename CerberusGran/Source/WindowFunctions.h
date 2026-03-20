#pragma once
#include <cmath>
#include <array>

// Window types: 0=Square, 1=Trapezoid, 2=Sine(Hamming), 3=Triangle, 4=SawDown
// Ordered from sharpest to smoothest for continuous morphing
static constexpr int kNumWindowTypes = 5;

class WindowFunctions
{
public:
    static constexpr int kTableSize = 1024;

    // Discrete window lookup (used internally)
    static float getValueDiscrete (int windowType, float phase)
    {
        static const auto& tables = getTables();

        if (windowType < 0 || windowType >= kNumWindowTypes)
            windowType = 0;

        float pos = phase * static_cast<float> (kTableSize - 1);
        int idx0 = static_cast<int> (pos);
        float frac = pos - static_cast<float> (idx0);

        if (idx0 < 0) idx0 = 0;
        if (idx0 >= kTableSize - 1) return tables[windowType][kTableSize - 1];

        return tables[windowType][idx0] * (1.0f - frac)
             + tables[windowType][idx0 + 1] * frac;
    }

    // Continuous morph: shape 0.0 = Square, 0.25 = Trapezoid, 0.5 = Sine,
    //                   0.75 = Triangle, 1.0 = SawDown
    static float getValue (float shape, float phase)
    {
        float scaled = shape * static_cast<float> (kNumWindowTypes - 1);
        int lo = static_cast<int> (scaled);
        int hi = lo + 1;
        float blend = scaled - static_cast<float> (lo);

        if (lo < 0) lo = 0;
        if (hi >= kNumWindowTypes) hi = kNumWindowTypes - 1;
        if (lo >= kNumWindowTypes) lo = kNumWindowTypes - 1;

        float a = getValueDiscrete (lo, phase);
        float b = getValueDiscrete (hi, phase);
        return a + (b - a) * blend;
    }

    // Legacy int overload for compatibility
    static float getValue (int windowType, float phase)
    {
        return getValueDiscrete (windowType, phase);
    }

private:
    using Table = std::array<float, kTableSize>;
    using Tables = std::array<Table, kNumWindowTypes>;

    static const Tables& getTables()
    {
        static Tables tables = buildTables();
        return tables;
    }

    static Tables buildTables()
    {
        Tables t {};
        constexpr float pi = 3.14159265358979323846f;

        for (int i = 0; i < kTableSize; ++i)
        {
            float p = static_cast<float> (i) / static_cast<float> (kTableSize - 1);

            // 0: Square — with short 1% fade in/out to avoid clicks
            {
                constexpr float fade = 0.01f;
                if (p < fade)
                    t[0][i] = p / fade;
                else if (p > 1.0f - fade)
                    t[0][i] = (1.0f - p) / fade;
                else
                    t[0][i] = 1.0f;
            }

            // 1: Trapezoid — ramp up 5%, flat 90%, ramp down 5%
            {
                constexpr float ramp = 0.05f;
                if (p < ramp)
                    t[1][i] = p / ramp;
                else if (p > 1.0f - ramp)
                    t[1][i] = (1.0f - p) / ramp;
                else
                    t[1][i] = 1.0f;
            }

            // 2: Sine/Hamming — half-sine bell shape
            t[2][i] = std::sin (pi * p);

            // 3: Triangle — rises to peak at center, falls back
            t[3][i] = (p < 0.5f) ? (2.0f * p) : (2.0f * (1.0f - p));

            // 4: Saw Down — fade in 2%, then linear fall to 0, fade out last 1%
            {
                constexpr float fadeIn = 0.02f;
                constexpr float fadeOut = 0.01f;
                float raw = 1.0f - p;
                if (p < fadeIn)
                    t[4][i] = (p / fadeIn) * raw;
                else if (p > 1.0f - fadeOut)
                    t[4][i] = raw * ((1.0f - p) / fadeOut);
                else
                    t[4][i] = raw;
            }
        }

        return t;
    }
};
