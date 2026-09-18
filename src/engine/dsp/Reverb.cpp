// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "Reverb.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr int k_CombTuning[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
constexpr int k_AllpassTuning[4] = { 556, 441, 341, 225 };
constexpr int k_StereoSpread = 23;
constexpr int k_ReferenceRate = 44100;
constexpr float k_FixedGain = 0.015f;
constexpr float k_RoomScale = 0.28f;
constexpr float k_RoomOffset = 0.7f;
constexpr float k_DampScale = 0.4f;

int scaled(int tuning, int sampleRate)
{
    const int size = static_cast<int>(std::lround(static_cast<double>(tuning) * sampleRate /
                                                  static_cast<double>(k_ReferenceRate)));
    return std::max(size, 1);
}
}

void Reverb::prepare(int sampleRate)
{
    for (int i = 0; i < k_NumCombs; ++i) {
        m_combL[static_cast<size_t>(i)].setBuffer(scaled(k_CombTuning[i], sampleRate));
        m_combR[static_cast<size_t>(i)].setBuffer(
            scaled(k_CombTuning[i] + k_StereoSpread, sampleRate));
    }
    for (int i = 0; i < k_NumAllpass; ++i) {
        m_allpassL[static_cast<size_t>(i)].setBuffer(scaled(k_AllpassTuning[i], sampleRate));
        m_allpassR[static_cast<size_t>(i)].setBuffer(
            scaled(k_AllpassTuning[i] + k_StereoSpread, sampleRate));
        m_allpassL[static_cast<size_t>(i)].feedback = 0.5f;
        m_allpassR[static_cast<size_t>(i)].feedback = 0.5f;
    }
    updateInternal();
}

void Reverb::reset()
{
    for (auto& c : m_combL) c.setBuffer(static_cast<int>(c.buffer.size()));
    for (auto& c : m_combR) c.setBuffer(static_cast<int>(c.buffer.size()));
    for (auto& a : m_allpassL) a.setBuffer(static_cast<int>(a.buffer.size()));
    for (auto& a : m_allpassR) a.setBuffer(static_cast<int>(a.buffer.size()));
}

void Reverb::setParams(float roomSize, float damping, float mix)
{
    m_roomSize = std::clamp(roomSize, 0.0f, 1.0f);
    m_damping = std::clamp(damping, 0.0f, 1.0f);
    m_mix = std::clamp(mix, 0.0f, 1.0f);
    updateInternal();
}

void Reverb::updateInternal()
{
    const float feedback = m_roomSize * k_RoomScale + k_RoomOffset;
    const float damp1 = m_damping * k_DampScale;
    const float damp2 = 1.0f - damp1;
    for (auto& c : m_combL) {
        c.feedback = feedback;
        c.damp1 = damp1;
        c.damp2 = damp2;
    }
    for (auto& c : m_combR) {
        c.feedback = feedback;
        c.damp1 = damp1;
        c.damp2 = damp2;
    }
}

void Reverb::process(float* interleavedStereo, int frames)
{
    if (m_mix <= 0.0f) return;

    for (int n = 0; n < frames; ++n) {
        const float inL = interleavedStereo[n * 2];
        const float inR = interleavedStereo[n * 2 + 1];
        const float input = (inL + inR) * k_FixedGain;

        float outL = 0.0f;
        float outR = 0.0f;
        for (int i = 0; i < k_NumCombs; ++i) {
            outL += m_combL[static_cast<size_t>(i)].process(input);
            outR += m_combR[static_cast<size_t>(i)].process(input);
        }
        for (int i = 0; i < k_NumAllpass; ++i) {
            outL = m_allpassL[static_cast<size_t>(i)].process(outL);
            outR = m_allpassR[static_cast<size_t>(i)].process(outR);
        }

        interleavedStereo[n * 2] = inL * (1.0f - m_mix) + outL * m_mix;
        interleavedStereo[n * 2 + 1] = inR * (1.0f - m_mix) + outR * m_mix;
    }
}
