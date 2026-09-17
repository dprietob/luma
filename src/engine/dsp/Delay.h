// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

#include <vector>

class Delay
{
public:
    void prepare(int sampleRate, float maxMs);
    void setParams(float timeMs, float feedback, float mix);
    void process(float* interleavedStereo, int frames);
    void reset();

private:
    std::vector<float> m_bufL;
    std::vector<float> m_bufR;
    int m_size { 0 };
    int m_write { 0 };
    int m_sampleRate { 48000 };

    int m_delaySamples { 1 };
    float m_feedback { 0.0f };
    float m_mix { 0.0f };
};
