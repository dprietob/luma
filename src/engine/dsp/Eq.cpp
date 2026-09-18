// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "Eq.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float k_Pi = 3.14159265358979323846f;
}

void Eq::prepare(int sampleRate)
{
    m_sampleRate = sampleRate > 0 ? sampleRate : 48000;
    m_gainsDb.fill(fx::k_EqBandGain);
    for (int b = 0; b < fx::k_EqBandCount; ++b)
        makePeaking(m_bands[static_cast<size_t>(b)], fx::k_EqBandFreq[b], fx::k_EqBandQ,
                    m_gainsDb[static_cast<size_t>(b)]);
    reset();
}

void Eq::setParams(const float* gainsDb, int count)
{
    const int n = std::min(count, fx::k_EqBandCount);
    for (int b = 0; b < n; ++b) {
        if (gainsDb[b] == m_gainsDb[static_cast<size_t>(b)]) continue;
        m_gainsDb[static_cast<size_t>(b)] = gainsDb[b];
        makePeaking(m_bands[static_cast<size_t>(b)], fx::k_EqBandFreq[b], fx::k_EqBandQ,
                    gainsDb[b]);
    }
}

void Eq::makePeaking(Biquad& bq, float freq, float q, float gainDb) const
{
    const float a = std::pow(10.0f, gainDb / 40.0f);
    const float w0 = 2.0f * k_Pi * freq / static_cast<float>(m_sampleRate);
    const float cosw = std::cos(w0);
    const float alpha = std::sin(w0) / (2.0f * q);

    const float a0 = 1.0f + alpha / a;
    bq.b0 = (1.0f + alpha * a) / a0;
    bq.b1 = -2.0f * cosw / a0;
    bq.b2 = (1.0f - alpha * a) / a0;
    bq.a1 = -2.0f * cosw / a0;
    bq.a2 = (1.0f - alpha / a) / a0;
}

void Eq::process(float* interleavedStereo, int frames)
{
    for (int i = 0; i < frames; ++i) {
        for (int ch = 0; ch < 2; ++ch) {
            float s = interleavedStereo[i * 2 + ch];
            for (auto& band : m_bands) s = band.process(s, ch);
            interleavedStereo[i * 2 + ch] = s;
        }
    }
}

void Eq::reset()
{
    for (auto& band : m_bands) {
        band.z1[0] = band.z1[1] = 0.0f;
        band.z2[0] = band.z2[1] = 0.0f;
    }
}
