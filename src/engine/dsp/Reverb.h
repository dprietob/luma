// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

#include <array>
#include <vector>

class Reverb
{
public:
    void prepare(int sampleRate);
    void setParams(float roomSize, float damping, float mix);
    void process(float* interleavedStereo, int frames);
    void reset();

private:
    struct Comb
    {
        std::vector<float> buffer;
        int index { 0 };
        float filterStore { 0.0f };
        float feedback { 0.0f };
        float damp1 { 0.0f };
        float damp2 { 1.0f };

        void setBuffer(int size)
        {
            buffer.assign(static_cast<size_t>(size), 0.0f);
            index = 0;
            filterStore = 0.0f;
        }

        inline float process(float input)
        {
            const float output = buffer[static_cast<size_t>(index)];
            filterStore = output * damp2 + filterStore * damp1;
            buffer[static_cast<size_t>(index)] = input + filterStore * feedback;
            if (++index >= static_cast<int>(buffer.size())) index = 0;
            return output;
        }
    };

    struct Allpass
    {
        std::vector<float> buffer;
        int index { 0 };
        float feedback { 0.5f };

        void setBuffer(int size)
        {
            buffer.assign(static_cast<size_t>(size), 0.0f);
            index = 0;
        }

        inline float process(float input)
        {
            const float bufout = buffer[static_cast<size_t>(index)];
            const float output = -input + bufout;
            buffer[static_cast<size_t>(index)] = input + bufout * feedback;
            if (++index >= static_cast<int>(buffer.size())) index = 0;
            return output;
        }
    };

    static constexpr int k_NumCombs = 8;
    static constexpr int k_NumAllpass = 4;

    void updateInternal();

    std::array<Comb, k_NumCombs> m_combL {};
    std::array<Comb, k_NumCombs> m_combR {};
    std::array<Allpass, k_NumAllpass> m_allpassL {};
    std::array<Allpass, k_NumAllpass> m_allpassR {};

    float m_roomSize { 0.5f };
    float m_damping { 0.5f };
    float m_mix { 0.3f };
};
