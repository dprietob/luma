// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "Delay.h"

#include <algorithm>
#include <cmath>

void Delay::prepare(int sampleRate, float maxMs)
{
    m_sampleRate = sampleRate;
    m_size = std::max(1, static_cast<int>(std::lround(maxMs / 1000.0f * sampleRate)));
    m_bufL.assign(static_cast<size_t>(m_size), 0.0f);
    m_bufR.assign(static_cast<size_t>(m_size), 0.0f);
    m_write = 0;
}

void Delay::reset()
{
    std::fill(m_bufL.begin(), m_bufL.end(), 0.0f);
    std::fill(m_bufR.begin(), m_bufR.end(), 0.0f);
    m_write = 0;
}

void Delay::setParams(float timeMs, float feedback, float mix)
{
    const int samples = static_cast<int>(std::lround(timeMs / 1000.0f * m_sampleRate));
    m_delaySamples = std::clamp(samples, 1, m_size - 1);
    m_feedback = std::clamp(feedback, 0.0f, 0.95f);
    m_mix = std::clamp(mix, 0.0f, 1.0f);
}

void Delay::process(float* interleavedStereo, int frames)
{
    if (m_size <= 1) return;

    for (int n = 0; n < frames; ++n) {
        const int readIdx = (m_write - m_delaySamples + m_size) % m_size;
        const float dL = m_bufL[static_cast<size_t>(readIdx)];
        const float dR = m_bufR[static_cast<size_t>(readIdx)];

        const float inL = interleavedStereo[n * 2];
        const float inR = interleavedStereo[n * 2 + 1];

        m_bufL[static_cast<size_t>(m_write)] = inL + dL * m_feedback;
        m_bufR[static_cast<size_t>(m_write)] = inR + dR * m_feedback;

        interleavedStereo[n * 2] = inL + dL * m_mix;
        interleavedStereo[n * 2 + 1] = inR + dR * m_mix;

        if (++m_write >= m_size) m_write = 0;
    }
}
