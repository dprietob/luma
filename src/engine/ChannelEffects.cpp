// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "ChannelEffects.h"

#include "dsp/EffectTypes.h"
#include "interfaces/IAudioBackend.h"

#include <QJsonArray>
#include <QVariant>

#include <algorithm>

ChannelEffects::ChannelEffects(IAudioBackend* backend, QObject* parent)
    : QObject(parent)
    , m_backend(backend)
    , m_voiceId(IAudioBackend::k_InvalidVoice)
    , m_reverbEnabled(fx::k_ReverbEnabled)
    , m_reverbRoomSize(fx::k_ReverbRoomSize)
    , m_reverbDamping(fx::k_ReverbDamping)
    , m_reverbMix(fx::k_ReverbMix)
    , m_delayEnabled(fx::k_DelayEnabled)
    , m_delayTime(fx::k_DelayTimeMs)
    , m_delayFeedback(fx::k_DelayFeedback)
    , m_delayMix(fx::k_DelayMix)
    , m_distortionEnabled(fx::k_DistortionEnabled)
    , m_distortionDrive(fx::k_DistortionDrive)
    , m_distortionMix(fx::k_DistortionMix)
    , m_pitchEnabled(fx::k_PitchEnabled)
    , m_pitchSemitones(fx::k_PitchSemitones)
    , m_speedEnabled(fx::k_SpeedEnabled)
    , m_speedTempo(fx::k_SpeedTempo)
    , m_eqEnabled(fx::k_EqEnabled)
    , m_eqGains(fx::k_EqBandCount, fx::k_EqBandGain)
{}

bool ChannelEffects::reverbEnabled() const { return m_reverbEnabled; }
float ChannelEffects::reverbRoomSize() const { return m_reverbRoomSize; }
float ChannelEffects::reverbDamping() const { return m_reverbDamping; }
float ChannelEffects::reverbMix() const { return m_reverbMix; }
bool ChannelEffects::delayEnabled() const { return m_delayEnabled; }
float ChannelEffects::delayTime() const { return m_delayTime; }
float ChannelEffects::delayFeedback() const { return m_delayFeedback; }
float ChannelEffects::delayMix() const { return m_delayMix; }
bool ChannelEffects::distortionEnabled() const { return m_distortionEnabled; }
float ChannelEffects::distortionDrive() const { return m_distortionDrive; }
float ChannelEffects::distortionMix() const { return m_distortionMix; }
bool ChannelEffects::pitchEnabled() const { return m_pitchEnabled; }
float ChannelEffects::pitchSemitones() const { return m_pitchSemitones; }
bool ChannelEffects::speedEnabled() const { return m_speedEnabled; }
float ChannelEffects::speedTempo() const { return m_speedTempo; }
bool ChannelEffects::eqEnabled() const { return m_eqEnabled; }
float ChannelEffects::eqGainMax() const { return fx::k_EqGainMaxDb; }

QVariantList ChannelEffects::eqBands() const
{
    QVariantList list;
    list.reserve(m_eqGains.size());
    for (const float g : m_eqGains) list.append(g);
    return list;
}

QStringList ChannelEffects::eqBandLabels() const
{
    QStringList labels;
    labels.reserve(fx::k_EqBandCount);
    for (int b = 0; b < fx::k_EqBandCount; ++b) {
        const float f = fx::k_EqBandFreq[b];
        labels.append(f >= 1000.0f ? QStringLiteral("%1 kHz").arg(f / 1000.0)
                                   : QStringLiteral("%1 Hz").arg(static_cast<int>(f)));
    }
    return labels;
}

bool ChannelEffects::anyEnabled() const
{
    return m_reverbEnabled || m_delayEnabled || m_distortionEnabled || m_pitchEnabled ||
           m_speedEnabled;
}

// --- Reverb ---

void ChannelEffects::setReverbEnabled(bool enabled)
{
    if (m_reverbEnabled == enabled) return;
    m_reverbEnabled = enabled;
    pushReverb();
    emit reverbChanged();
    emit anyEnabledChanged();
}

void ChannelEffects::setReverbRoomSize(float value)
{
    const float v = std::clamp(value, 0.0f, 1.0f);
    if (qFuzzyCompare(m_reverbRoomSize, v)) return;
    m_reverbRoomSize = v;
    pushReverb();
    emit reverbChanged();
}

void ChannelEffects::setReverbDamping(float value)
{
    const float v = std::clamp(value, 0.0f, 1.0f);
    if (qFuzzyCompare(m_reverbDamping, v)) return;
    m_reverbDamping = v;
    pushReverb();
    emit reverbChanged();
}

void ChannelEffects::setReverbMix(float value)
{
    const float v = std::clamp(value, 0.0f, 1.0f);
    if (qFuzzyCompare(m_reverbMix, v)) return;
    m_reverbMix = v;
    pushReverb();
    emit reverbChanged();
}

// --- Delay ---

void ChannelEffects::setDelayEnabled(bool enabled)
{
    if (m_delayEnabled == enabled) return;
    m_delayEnabled = enabled;
    pushDelay();
    emit delayChanged();
    emit anyEnabledChanged();
}

void ChannelEffects::setDelayTime(float value)
{
    const float v = std::clamp(value, 0.0f, fx::k_DelayTimeMaxMs);
    if (qFuzzyCompare(m_delayTime, v)) return;
    m_delayTime = v;
    pushDelay();
    emit delayChanged();
}

void ChannelEffects::setDelayFeedback(float value)
{
    const float v = std::clamp(value, 0.0f, fx::k_DelayFeedbackMax);
    if (qFuzzyCompare(m_delayFeedback, v)) return;
    m_delayFeedback = v;
    pushDelay();
    emit delayChanged();
}

void ChannelEffects::setDelayMix(float value)
{
    const float v = std::clamp(value, 0.0f, 1.0f);
    if (qFuzzyCompare(m_delayMix, v)) return;
    m_delayMix = v;
    pushDelay();
    emit delayChanged();
}

// --- Distortion ---

void ChannelEffects::setDistortionEnabled(bool enabled)
{
    if (m_distortionEnabled == enabled) return;
    m_distortionEnabled = enabled;
    pushDistortion();
    emit distortionChanged();
    emit anyEnabledChanged();
}

void ChannelEffects::setDistortionDrive(float value)
{
    const float v = std::clamp(value, fx::k_DistortionDriveMin, fx::k_DistortionDriveMax);
    if (qFuzzyCompare(m_distortionDrive, v)) return;
    m_distortionDrive = v;
    pushDistortion();
    emit distortionChanged();
}

void ChannelEffects::setDistortionMix(float value)
{
    const float v = std::clamp(value, 0.0f, 1.0f);
    if (qFuzzyCompare(m_distortionMix, v)) return;
    m_distortionMix = v;
    pushDistortion();
    emit distortionChanged();
}

// --- Pitch ---

void ChannelEffects::setPitchEnabled(bool enabled)
{
    if (m_pitchEnabled == enabled) return;
    m_pitchEnabled = enabled;
    pushPitch();
    emit pitchChanged();
    emit anyEnabledChanged();
}

void ChannelEffects::setPitchSemitones(float value)
{
    const float v = std::clamp(value, -fx::k_PitchSemitonesMax, fx::k_PitchSemitonesMax);
    if (qFuzzyCompare(m_pitchSemitones, v)) return;
    m_pitchSemitones = v;
    pushPitch();
    emit pitchChanged();
}

// --- Speed ---

void ChannelEffects::setSpeedEnabled(bool enabled)
{
    if (m_speedEnabled == enabled) return;
    m_speedEnabled = enabled;
    pushSpeed();
    emit speedChanged();
    emit anyEnabledChanged();
}

void ChannelEffects::setSpeedTempo(float value)
{
    const float v = std::clamp(value, fx::k_SpeedTempoMin, fx::k_SpeedTempoMax);
    if (qFuzzyCompare(m_speedTempo, v)) return;
    m_speedTempo = v;
    pushSpeed();
    emit speedChanged();
}

// --- EQ ---

void ChannelEffects::setEqEnabled(bool enabled)
{
    if (m_eqEnabled == enabled) return;
    m_eqEnabled = enabled;
    pushEq();
    emit eqChanged();
}

void ChannelEffects::setEqBandGain(int band, float db)
{
    if (band < 0 || band >= m_eqGains.size()) return;
    const float v = std::clamp(db, -fx::k_EqGainMaxDb, fx::k_EqGainMaxDb);
    if (qFuzzyCompare(m_eqGains[band], v)) return;
    m_eqGains[band] = v;
    pushEq();
    emit eqChanged();
}

// --- Reset a valores por defecto ---

void ChannelEffects::resetReverb()
{
    setReverbEnabled(fx::k_ReverbEnabled);
    setReverbRoomSize(fx::k_ReverbRoomSize);
    setReverbDamping(fx::k_ReverbDamping);
    setReverbMix(fx::k_ReverbMix);
}

void ChannelEffects::resetDelay()
{
    setDelayEnabled(fx::k_DelayEnabled);
    setDelayTime(fx::k_DelayTimeMs);
    setDelayFeedback(fx::k_DelayFeedback);
    setDelayMix(fx::k_DelayMix);
}

void ChannelEffects::resetDistortion()
{
    setDistortionEnabled(fx::k_DistortionEnabled);
    setDistortionDrive(fx::k_DistortionDrive);
    setDistortionMix(fx::k_DistortionMix);
}

void ChannelEffects::resetPitch()
{
    setPitchEnabled(fx::k_PitchEnabled);
    setPitchSemitones(fx::k_PitchSemitones);
}

void ChannelEffects::resetSpeed()
{
    setSpeedEnabled(fx::k_SpeedEnabled);
    setSpeedTempo(fx::k_SpeedTempo);
}

void ChannelEffects::resetEq()
{
    setEqEnabled(fx::k_EqEnabled);
    for (int b = 0; b < m_eqGains.size(); ++b) setEqBandGain(b, fx::k_EqBandGain);
}

void ChannelEffects::resetAll()
{
    resetReverb();
    resetDelay();
    resetDistortion();
    resetPitch();
    resetSpeed();
    resetEq();
}

// --- Enrutado al backend ---

void ChannelEffects::setVoiceId(int voiceId)
{
    m_voiceId = voiceId;
    reapply();
}

void ChannelEffects::reapply() const
{
    pushReverb();
    pushDelay();
    pushDistortion();
    pushPitch();
    pushSpeed();
    pushEq();
}

QJsonObject ChannelEffects::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("reverbEnabled")] = m_reverbEnabled;
    o[QStringLiteral("reverbRoomSize")] = m_reverbRoomSize;
    o[QStringLiteral("reverbDamping")] = m_reverbDamping;
    o[QStringLiteral("reverbMix")] = m_reverbMix;
    o[QStringLiteral("delayEnabled")] = m_delayEnabled;
    o[QStringLiteral("delayTime")] = m_delayTime;
    o[QStringLiteral("delayFeedback")] = m_delayFeedback;
    o[QStringLiteral("delayMix")] = m_delayMix;
    o[QStringLiteral("distortionEnabled")] = m_distortionEnabled;
    o[QStringLiteral("distortionDrive")] = m_distortionDrive;
    o[QStringLiteral("distortionMix")] = m_distortionMix;
    o[QStringLiteral("pitchEnabled")] = m_pitchEnabled;
    o[QStringLiteral("pitchSemitones")] = m_pitchSemitones;
    o[QStringLiteral("speedEnabled")] = m_speedEnabled;
    o[QStringLiteral("speedTempo")] = m_speedTempo;
    o[QStringLiteral("eqEnabled")] = m_eqEnabled;
    QJsonArray eqBands;
    for (const float g : m_eqGains) eqBands.append(static_cast<double>(g));
    o[QStringLiteral("eqBands")] = eqBands;
    return o;
}

void ChannelEffects::applyJson(const QJsonObject& o)
{
    setReverbEnabled(o.value(QStringLiteral("reverbEnabled")).toBool(fx::k_ReverbEnabled));
    setReverbRoomSize(static_cast<float>(
        o.value(QStringLiteral("reverbRoomSize")).toDouble(fx::k_ReverbRoomSize)));
    setReverbDamping(
        static_cast<float>(o.value(QStringLiteral("reverbDamping")).toDouble(fx::k_ReverbDamping)));
    setReverbMix(
        static_cast<float>(o.value(QStringLiteral("reverbMix")).toDouble(fx::k_ReverbMix)));
    setDelayEnabled(o.value(QStringLiteral("delayEnabled")).toBool(fx::k_DelayEnabled));
    setDelayTime(
        static_cast<float>(o.value(QStringLiteral("delayTime")).toDouble(fx::k_DelayTimeMs)));
    setDelayFeedback(
        static_cast<float>(o.value(QStringLiteral("delayFeedback")).toDouble(fx::k_DelayFeedback)));
    setDelayMix(static_cast<float>(o.value(QStringLiteral("delayMix")).toDouble(fx::k_DelayMix)));
    setDistortionEnabled(
        o.value(QStringLiteral("distortionEnabled")).toBool(fx::k_DistortionEnabled));
    setDistortionDrive(static_cast<float>(
        o.value(QStringLiteral("distortionDrive")).toDouble(fx::k_DistortionDrive)));
    setDistortionMix(
        static_cast<float>(o.value(QStringLiteral("distortionMix")).toDouble(fx::k_DistortionMix)));
    setPitchEnabled(o.value(QStringLiteral("pitchEnabled")).toBool(fx::k_PitchEnabled));
    setPitchSemitones(static_cast<float>(
        o.value(QStringLiteral("pitchSemitones")).toDouble(fx::k_PitchSemitones)));
    setSpeedEnabled(o.value(QStringLiteral("speedEnabled")).toBool(fx::k_SpeedEnabled));
    setSpeedTempo(
        static_cast<float>(o.value(QStringLiteral("speedTempo")).toDouble(fx::k_SpeedTempo)));
    setEqEnabled(o.value(QStringLiteral("eqEnabled")).toBool(fx::k_EqEnabled));
    const QJsonArray eqBands = o.value(QStringLiteral("eqBands")).toArray();
    for (int b = 0; b < fx::k_EqBandCount; ++b)
        setEqBandGain(b, static_cast<float>(b < eqBands.size()
                                                ? eqBands.at(b).toDouble(fx::k_EqBandGain)
                                                : fx::k_EqBandGain));
}

void ChannelEffects::pushReverb() const
{
    if (!m_backend || m_voiceId == IAudioBackend::k_InvalidVoice) return;
    m_backend->setVoiceReverb(m_voiceId, m_reverbEnabled, m_reverbRoomSize, m_reverbDamping,
                              m_reverbMix);
}

void ChannelEffects::pushDelay() const
{
    if (!m_backend || m_voiceId == IAudioBackend::k_InvalidVoice) return;
    m_backend->setVoiceDelay(m_voiceId, m_delayEnabled, m_delayTime, m_delayFeedback, m_delayMix);
}

void ChannelEffects::pushDistortion() const
{
    if (!m_backend || m_voiceId == IAudioBackend::k_InvalidVoice) return;
    m_backend->setVoiceDistortion(m_voiceId, m_distortionEnabled, m_distortionDrive,
                                  m_distortionMix);
}

void ChannelEffects::pushPitch() const
{
    if (!m_backend || m_voiceId == IAudioBackend::k_InvalidVoice) return;
    m_backend->setVoicePitch(m_voiceId, m_pitchEnabled, m_pitchSemitones);
}

void ChannelEffects::pushSpeed() const
{
    if (!m_backend || m_voiceId == IAudioBackend::k_InvalidVoice) return;
    m_backend->setVoiceSpeed(m_voiceId, m_speedEnabled, m_speedTempo);
}

void ChannelEffects::pushEq() const
{
    if (!m_backend || m_voiceId == IAudioBackend::k_InvalidVoice) return;
    m_backend->setVoiceEq(m_voiceId, m_eqEnabled, m_eqGains);
}
