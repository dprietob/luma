// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "FlowNodeConfig.h"

#include "ChannelEffects.h"
#include "TrackTimeline.h"

#include <QJsonObject>

#include <algorithm>

FlowNodeConfig::FlowNodeConfig(IAudioBackend* backend, QObject* parent)
    : QObject(parent)
    , m_effects(new ChannelEffects(backend, this))
    , m_timeline(new TrackTimeline(backend, this))
{}

float FlowNodeConfig::pan() const { return m_pan; }
float FlowNodeConfig::initialVolume() const { return m_initialVolume; }
float FlowNodeConfig::fadeMax() const { return m_fadeMax; }
float FlowNodeConfig::fadeMin() const { return m_fadeMin; }
int FlowNodeConfig::fadeSeconds() const { return m_fadeSeconds; }
int FlowNodeConfig::fadeMode() const { return m_fadeMode; }
bool FlowNodeConfig::loop() const { return m_loop; }
double FlowNodeConfig::startSeconds() const { return m_startSeconds; }
ChannelEffects* FlowNodeConfig::effects() const { return m_effects; }
TrackTimeline* FlowNodeConfig::timeline() const { return m_timeline; }

void FlowNodeConfig::setPan(float value)
{
    const float clamped = std::clamp(value, -1.0f, 1.0f);
    if (qFuzzyCompare(m_pan, clamped)) return;
    m_pan = clamped;
    emit panChanged();
}

void FlowNodeConfig::setInitialVolume(float value)
{
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    if (qFuzzyCompare(m_initialVolume, clamped)) return;
    m_initialVolume = clamped;
    emit initialVolumeChanged();
}

void FlowNodeConfig::setFadeMax(float value)
{
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    if (qFuzzyCompare(m_fadeMax, clamped)) return;
    m_fadeMax = clamped;
    emit fadeMaxChanged();
}

void FlowNodeConfig::setFadeMin(float value)
{
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    if (qFuzzyCompare(m_fadeMin, clamped)) return;
    m_fadeMin = clamped;
    emit fadeMinChanged();
}

void FlowNodeConfig::setFadeSeconds(int value)
{
    const int clamped = std::max(0, value);
    if (m_fadeSeconds == clamped) return;
    m_fadeSeconds = clamped;
    emit fadeSecondsChanged();
}

void FlowNodeConfig::setFadeMode(int value)
{
    const int clamped = std::max(0, value);
    if (m_fadeMode == clamped) return;
    m_fadeMode = clamped;
    emit fadeModeChanged();
}

void FlowNodeConfig::setLoop(bool value)
{
    if (m_loop == value) return;
    m_loop = value;
    emit loopChanged();
}

void FlowNodeConfig::setStartSeconds(double value)
{
    const double clamped = std::max(0.0, value);
    if (qFuzzyCompare(m_startSeconds, clamped)) return;
    m_startSeconds = clamped;
    emit startSecondsChanged();
}

void FlowNodeConfig::loadFromMap(const QVariantMap& config)
{
    setPan(static_cast<float>(config.value(QStringLiteral("pan"), 0.0).toDouble()));
    setInitialVolume(
        static_cast<float>(config.value(QStringLiteral("initialVolume"), 0.8).toDouble()));
    setFadeMax(static_cast<float>(config.value(QStringLiteral("fadeMax"), 1.0).toDouble()));
    setFadeMin(static_cast<float>(config.value(QStringLiteral("fadeMin"), 0.0).toDouble()));
    setFadeSeconds(config.value(QStringLiteral("fadeSeconds"), 2).toInt());
    setFadeMode(config.value(QStringLiteral("fadeMode"), 0).toInt());
    setLoop(config.value(QStringLiteral("loop"), false).toBool());
    setStartSeconds(config.value(QStringLiteral("startSeconds"), 0.0).toDouble());

    m_effects->applyJson(
        QJsonObject::fromVariantMap(config.value(QStringLiteral("effects")).toMap()));
    m_timeline->applyJson(
        QJsonObject::fromVariantMap(config.value(QStringLiteral("region")).toMap()));
}

QVariantMap FlowNodeConfig::toMap() const
{
    QVariantMap config;
    config[QStringLiteral("pan")] = m_pan;
    config[QStringLiteral("initialVolume")] = m_initialVolume;
    config[QStringLiteral("fadeMax")] = m_fadeMax;
    config[QStringLiteral("fadeMin")] = m_fadeMin;
    config[QStringLiteral("fadeSeconds")] = m_fadeSeconds;
    config[QStringLiteral("fadeMode")] = m_fadeMode;
    config[QStringLiteral("loop")] = m_loop;
    config[QStringLiteral("startSeconds")] = m_startSeconds;
    config[QStringLiteral("effects")] = m_effects->toJson().toVariantMap();
    config[QStringLiteral("region")] = m_timeline->toJson().toVariantMap();
    return config;
}
