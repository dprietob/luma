// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "VoiceEffects.h"

#include "EffectTypes.h"

#include <algorithm>
#include <cmath>

VoiceEffects::VoiceEffects(int sampleRate)
    : m_sampleRate(sampleRate)
{
    m_st.setSampleRate(static_cast<unsigned int>(sampleRate));
    m_st.setChannels(2);

    m_reverb.prepare(sampleRate);
    m_delay.prepare(sampleRate, fx::k_DelayTimeMaxMs);
    m_eq.prepare(sampleRate);
}

void VoiceEffects::setReverb(bool enabled, float roomSize, float damping, float mix)
{
    m_reverbEnabled.store(enabled, std::memory_order_relaxed);
    m_reverbRoom.store(roomSize, std::memory_order_relaxed);
    m_reverbDamp.store(damping, std::memory_order_relaxed);
    m_reverbMix.store(mix, std::memory_order_relaxed);
}

void VoiceEffects::setDelay(bool enabled, float timeMs, float feedback, float mix)
{
    m_delayEnabled.store(enabled, std::memory_order_relaxed);
    m_delayTime.store(timeMs, std::memory_order_relaxed);
    m_delayFeedback.store(feedback, std::memory_order_relaxed);
    m_delayMix.store(mix, std::memory_order_relaxed);
}

void VoiceEffects::setDistortion(bool enabled, float drive, float mix)
{
    m_distortionEnabled.store(enabled, std::memory_order_relaxed);
    m_distortionDrive.store(drive, std::memory_order_relaxed);
    m_distortionMix.store(mix, std::memory_order_relaxed);
}

void VoiceEffects::setPitch(bool enabled, float semitones)
{
    m_pitchEnabled.store(enabled, std::memory_order_relaxed);
    m_pitchSemitones.store(semitones, std::memory_order_relaxed);
}

void VoiceEffects::setSpeed(bool enabled, float tempo)
{
    m_speedEnabled.store(enabled, std::memory_order_relaxed);
    m_speedTempo.store(tempo, std::memory_order_relaxed);
}

void VoiceEffects::setEq(bool enabled, const float* gainsDb, int count)
{
    m_eqEnabled.store(enabled, std::memory_order_relaxed);
    const int n = std::min(count, fx::k_EqBandCount);
    for (int b = 0; b < n; ++b)
        m_eqGains[static_cast<size_t>(b)].store(gainsDb[b], std::memory_order_relaxed);
}

bool VoiceEffects::timeStretchActive() const
{
    const bool pitch = m_pitchEnabled.load(std::memory_order_relaxed) &&
                       std::fabs(m_pitchSemitones.load(std::memory_order_relaxed)) > 0.01f;
    const bool speed = m_speedEnabled.load(std::memory_order_relaxed) &&
                       std::fabs(m_speedTempo.load(std::memory_order_relaxed) - 1.0f) > 0.001f;
    return pitch || speed;
}

void VoiceEffects::syncStretchParams()
{
    const bool pitchOn = m_pitchEnabled.load(std::memory_order_relaxed);
    const bool speedOn = m_speedEnabled.load(std::memory_order_relaxed);
    m_st.setPitchSemiTones(pitchOn ? m_pitchSemitones.load(std::memory_order_relaxed) : 0.0f);
    m_st.setTempo(speedOn ? m_speedTempo.load(std::memory_order_relaxed) : 1.0f);
}

void VoiceEffects::processChain(float* interleavedStereo, int frames)
{
    if (m_eqEnabled.load(std::memory_order_relaxed)) {
        float gains[fx::k_EqBandCount];
        for (int b = 0; b < fx::k_EqBandCount; ++b)
            gains[b] = m_eqGains[static_cast<size_t>(b)].load(std::memory_order_relaxed);
        m_eq.setParams(gains, fx::k_EqBandCount);
        m_eq.process(interleavedStereo, frames);
    }
    if (m_distortionEnabled.load(std::memory_order_relaxed)) {
        m_distortion.setParams(m_distortionDrive.load(std::memory_order_relaxed),
                               m_distortionMix.load(std::memory_order_relaxed));
        m_distortion.process(interleavedStereo, frames);
    }
    if (m_delayEnabled.load(std::memory_order_relaxed)) {
        m_delay.setParams(m_delayTime.load(std::memory_order_relaxed),
                          m_delayFeedback.load(std::memory_order_relaxed),
                          m_delayMix.load(std::memory_order_relaxed));
        m_delay.process(interleavedStereo, frames);
    }
    if (m_reverbEnabled.load(std::memory_order_relaxed)) {
        m_reverb.setParams(m_reverbRoom.load(std::memory_order_relaxed),
                           m_reverbDamp.load(std::memory_order_relaxed),
                           m_reverbMix.load(std::memory_order_relaxed));
        m_reverb.process(interleavedStereo, frames);
    }
}
