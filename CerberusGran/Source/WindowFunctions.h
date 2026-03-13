#pragma once
#include <cmath>
#include <array>

class WindowFunctions
{
public:
    static constexpr int kTableSize = 1024;

    // Window types: 0=Hann, 1=Gaussian, 2=Tukey, 3=Triangle
    static float getValue (int windowType, float phase)
    {
        static const auto& tables = getTables();

        windowType = (windowType < 0 || windowType > 3) ? 0 : windowType;

        float pos = phase * static_cast<float> (kTableSize - 1);
        int idx0 = static_cast<int> (pos);
        float frac = pos - static_cast<float> (idx0);

        if (idx0 < 0) idx0 = 0;
        if (idx0 >= kTableSize - 1) return tables[windowType][kTableSize - 1];

        return tables[windowType][idx0] * (1.0f - frac)
             + tables[windowType][idx0 + 1] * frac;
    }

private:
    using Table = std::array<float, kTableSize>;
    using Tables = std::array<Table, 4>;

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

            // Hann
            t[0][i] = 0.5f * (1.0f - std::cos (2.0f * pi * p));

            // Gaussian (sigma = 0.4)
            float g = (p - 0.5f) / 0.4f;
            t[1][i] = std::exp (-0.5f * g * g);

            // Tukey (alpha = 0.5)
            {
                constexpr float alpha = 0.5f;
                if (p < alpha * 0.5f)
                    t[2][i] = 0.5f * (1.0f + std::cos (pi * (2.0f * p / alpha - 1.0f)));
                else if (p > 1.0f - alpha * 0.5f)
                    t[2][i] = 0.5f * (1.0f + std::cos (pi * (2.0f * p / alpha - 2.0f / alpha + 1.0f)));
                else
                    t[2][i] = 1.0f;
            }

            // Triangle
            t[3][i] = (p < 0.5f) ? (2.0f * p) : (2.0f * (1.0f - p));
        }

        return t;
    }
};
