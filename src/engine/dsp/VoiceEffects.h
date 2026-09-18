// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

#include "Delay.h"
#include "Distortion.h"
#include "EffectTypes.h"
#include "Eq.h"
#include "Reverb.h"

#include <soundtouch/SoundTouch.h>

#include <array>
#include <atomic>

class VoiceEffects
{
public:
    explicit VoiceEffects(int sampleRate);

    void setReverb(bool enabled, float roomSize, float damping, float mix);
    void setDelay(bool enabled, float timeMs, float feedback, float mix);
    void setDistortion(bool enabled, float drive, float mix);
    void setPitch(bool enabled, float semitones);
    void setSpeed(bool enabled, float tempo);
    void setEq(bool enabled, const float* gainsDb, int count);

    // --- Hilo de audio ---
    [[nodiscard]] bool timeStretchActive() const;
    [[nodiscard]] soundtouch::SoundTouch& stretcher() { return m_st; }
    void syncStretchParams();
    void processChain(float* interleavedStereo, int frames);

private:
    int m_sampleRate;

    soundtouch::SoundTouch m_st;
    Reverb m_reverb;
    Delay m_delay;
    Distortion m_distortion;
    Eq m_eq;

    std::atomic<bool> m_reverbEnabled { false };
    std::atomic<float> m_reverbRoom { 0.5f };
    std::atomic<float> m_reverbDamp { 0.5f };
    std::atomic<float> m_reverbMix { 0.3f };

    std::atomic<bool> m_delayEnabled { false };
    std::atomic<float> m_delayTime { 300.0f };
    std::atomic<float> m_delayFeedback { 0.35f };
    std::atomic<float> m_delayMix { 0.3f };

    std::atomic<bool> m_distortionEnabled { false };
    std::atomic<float> m_distortionDrive { 5.0f };
    std::atomic<float> m_distortionMix { 0.5f };

    std::atomic<bool> m_pitchEnabled { false };
    std::atomic<float> m_pitchSemitones { 0.0f };

    std::atomic<bool> m_speedEnabled { false };
    std::atomic<float> m_speedTempo { 1.0f };

    std::atomic<bool> m_eqEnabled { false };
    std::array<std::atomic<float>, fx::k_EqBandCount> m_eqGains {};
};
