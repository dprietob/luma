// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "TrackTimeline.h"

#include "interfaces/IAudioBackend.h"

#include <algorithm>

TrackTimeline::TrackTimeline(IAudioBackend* backend, QObject* parent)
    : QObject(parent)
    , m_backend(backend)
    , m_voiceId(IAudioBackend::k_InvalidVoice)
{}

bool TrackTimeline::regionEnabled() const { return m_enabled; }
bool TrackTimeline::hasStart() const { return m_hasStart; }
double TrackTimeline::start() const { return m_start; }
bool TrackTimeline::hasEnd() const { return m_hasEnd; }
double TrackTimeline::end() const { return m_end; }
double TrackTimeline::durationSeconds() const { return m_durationSeconds; }
QVariantList TrackTimeline::waveform() const { return m_waveform; }

double TrackTimeline::configuredStart() const { return m_hasStart ? m_start : 0.0; }
double TrackTimeline::configuredEnd() const { return m_hasEnd ? m_end : 1.0; }

double TrackTimeline::regionSeconds() const
{
    return (configuredEnd() - configuredStart()) * m_durationSeconds;
}

void TrackTimeline::setRegionEnabled(bool enabled)
{
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    pushRegion();
    emit regionChanged();
}

void TrackTimeline::setStart(double frac)
{
    const double v = std::clamp(frac, 0.0, configuredEnd() - k_MinGap);
    if (m_hasStart && qFuzzyCompare(m_start, v)) return;
    m_hasStart = true;
    m_start = v;
    pushRegion();
    emit regionChanged();
}

void TrackTimeline::setEnd(double frac)
{
    const double v = std::clamp(frac, configuredStart() + k_MinGap, 1.0);
    if (m_hasEnd && qFuzzyCompare(m_end, v)) return;
    m_hasEnd = true;
    m_end = v;
    pushRegion();
    emit regionChanged();
}

void TrackTimeline::clearStart()
{
    if (!m_hasStart) return;
    m_hasStart = false;
    m_start = 0.0;
    pushRegion();
    emit regionChanged();
}

void TrackTimeline::clearEnd()
{
    if (!m_hasEnd) return;
    m_hasEnd = false;
    m_end = 1.0;
    pushRegion();
    emit regionChanged();
}

void TrackTimeline::reset()
{
    if (!m_hasStart && !m_hasEnd) return;
    m_hasStart = false;
    m_start = 0.0;
    m_hasEnd = false;
    m_end = 1.0;
    pushRegion();
    emit regionChanged();
}

void TrackTimeline::bindVoice(int voiceId)
{
    m_voiceId = voiceId;
    m_enabled = true;
    m_hasStart = false;
    m_start = 0.0;
    m_hasEnd = false;
    m_end = 1.0;

    m_durationSeconds = m_backend ? m_backend->voiceDurationSeconds(voiceId) : 0.0;
    m_waveform.clear();
    if (m_backend) {
        const QList<float> peaks = m_backend->voiceWaveform(voiceId, k_WaveformBuckets);
        m_waveform.reserve(peaks.size());
        for (const float p : peaks) m_waveform.append(p);
    }

    pushRegion();
    emit durationChanged();
    emit waveformChanged();
    emit regionChanged();
}

void TrackTimeline::unbindVoice()
{
    m_voiceId = IAudioBackend::k_InvalidVoice;
    m_enabled = true;
    m_hasStart = false;
    m_start = 0.0;
    m_hasEnd = false;
    m_end = 1.0;
    m_durationSeconds = 0.0;
    m_waveform.clear();
    emit durationChanged();
    emit waveformChanged();
    emit regionChanged();
}

QJsonObject TrackTimeline::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("enabled")] = m_enabled;
    o[QStringLiteral("hasStart")] = m_hasStart;
    o[QStringLiteral("start")] = m_start;
    o[QStringLiteral("hasEnd")] = m_hasEnd;
    o[QStringLiteral("end")] = m_end;
    return o;
}

void TrackTimeline::applyJson(const QJsonObject& o)
{
    setRegionEnabled(o.value(QStringLiteral("enabled")).toBool(true));
    if (o.value(QStringLiteral("hasStart")).toBool(false))
        setStart(o.value(QStringLiteral("start")).toDouble(0.0));
    else
        clearStart();
    if (o.value(QStringLiteral("hasEnd")).toBool(false))
        setEnd(o.value(QStringLiteral("end")).toDouble(1.0));
    else
        clearEnd();
}

void TrackTimeline::pushRegion() const
{
    if (!m_backend || m_voiceId == IAudioBackend::k_InvalidVoice) return;
    m_backend->setVoiceRegion(m_voiceId, m_enabled ? configuredStart() : 0.0,
                              m_enabled ? configuredEnd() : 1.0);
}
