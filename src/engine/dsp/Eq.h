// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

#include "EffectTypes.h"

#include <array>

class Eq
{
public:
    void prepare(int sampleRate);
    void setParams(const float* gainsDb, int count);
    void process(float* interleavedStereo, int frames);
    void reset();

private:
    struct Biquad
    {
        float b0 { 1.0f };
        float b1 { 0.0f };
        float b2 { 0.0f };
        float a1 { 0.0f };
        float a2 { 0.0f };
        float z1[2] { 0.0f, 0.0f };
        float z2[2] { 0.0f, 0.0f };

        inline float process(float in, int ch)
        {
            const float out = b0 * in + z1[ch];
            z1[ch] = b1 * in - a1 * out + z2[ch];
            z2[ch] = b2 * in - a2 * out;
            return out;
        }
    };

    void makePeaking(Biquad& bq, float freq, float q, float gainDb) const;

    int m_sampleRate { 48000 };
    std::array<float, fx::k_EqBandCount> m_gainsDb {};
    std::array<Biquad, fx::k_EqBandCount> m_bands {};
};
