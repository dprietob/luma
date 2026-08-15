// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "Distortion.h"

#include <algorithm>
#include <cmath>

void Distortion::setParams(float drive, float mix)
{
    m_drive = std::max(1.0f, drive);
    m_mix = std::clamp(mix, 0.0f, 1.0f);
}

void Distortion::process(float* interleavedStereo, int frames)
{
    if (m_mix <= 0.0f) return;

    const float norm = 1.0f / std::tanh(m_drive);
    for (int i = 0; i < frames * 2; ++i) {
        const float in = interleavedStereo[i];
        const float wet = std::tanh(m_drive * in) * norm;
        interleavedStereo[i] = in * (1.0f - m_mix) + wet * m_mix;
    }
}
