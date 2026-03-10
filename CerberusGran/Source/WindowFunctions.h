#pragma once
#include <cmath>
#include <array>

class WindowFunctions
{
public:
    enum Shape { Hann = 0, Gaussian, Tukey, Triangle, NumShapes };

    static constexpr int kTableSize = 1024;

    static void initialise()
    {
        if (initialised) return;

        for (int i = 0; i < kTableSize; ++i)
        {
            double phase = static_cast<double> (i) / static_cast<double> (kTableSize - 1);

            // Hann
            tables[Hann][i] = static_cast<float> (0.5 * (1.0 - std::cos (2.0 * M_PI * phase)));

            // Gaussian (sigma = 0.4)
            double g = (phase - 0.5) / 0.4;
            tables[Gaussian][i] = static_cast<float> (std::exp (-0.5 * g * g));

            // Tukey (alpha = 0.5)
            if (phase < 0.25)
                tables[Tukey][i] = static_cast<float> (0.5 * (1.0 + std::cos (2.0 * M_PI * (phase / 0.5 - 1.0))));
            else if (phase > 0.75)
                tables[Tukey][i] = static_cast<float> (0.5 * (1.0 + std::cos (2.0 * M_PI * ((phase - 0.75) / 0.5))));
            else
                tables[Tukey][i] = 1.0f;

            // Triangle
            tables[Triangle][i] = static_cast<float> (phase < 0.5 ? 2.0 * phase : 2.0 * (1.0 - phase));
        }

        initialised = true;
    }

    static float lookup (int shape, float phase)
    {
        if (shape < 0 || shape >= NumShapes) shape = Hann;

        float pos = phase * static_cast<float> (kTableSize - 1);
        int idx0 = static_cast<int> (pos);
        if (idx0 < 0) idx0 = 0;
        if (idx0 >= kTableSize - 1) return tables[shape][kTableSize - 1];

        float frac = pos - static_cast<float> (idx0);
        return tables[shape][idx0] + frac * (tables[shape][idx0 + 1] - tables[shape][idx0]);
    }

private:
    static inline bool initialised = false;
    static inline std::array<std::array<float, kTableSize>, NumShapes> tables {};
};
