// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

#include <QObject>
#include <QVariantMap>

class ChannelEffects;
class TrackTimeline;
class IAudioBackend;

class FlowNodeConfig final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(float pan READ pan WRITE setPan NOTIFY panChanged FINAL)
    Q_PROPERTY(float initialVolume READ initialVolume WRITE setInitialVolume NOTIFY initialVolumeChanged FINAL)
    Q_PROPERTY(float fadeMax READ fadeMax WRITE setFadeMax NOTIFY fadeMaxChanged FINAL)
    Q_PROPERTY(float fadeMin READ fadeMin WRITE setFadeMin NOTIFY fadeMinChanged FINAL)
    Q_PROPERTY(int fadeSeconds READ fadeSeconds WRITE setFadeSeconds NOTIFY fadeSecondsChanged FINAL)
    Q_PROPERTY(int fadeMode READ fadeMode WRITE setFadeMode NOTIFY fadeModeChanged FINAL)
    Q_PROPERTY(bool loop READ loop WRITE setLoop NOTIFY loopChanged FINAL)
    Q_PROPERTY(double startSeconds READ startSeconds WRITE setStartSeconds NOTIFY startSecondsChanged FINAL)
    Q_PROPERTY(ChannelEffects* effects READ effects CONSTANT FINAL)
    Q_PROPERTY(TrackTimeline* timeline READ timeline CONSTANT FINAL)

public:
    explicit FlowNodeConfig(IAudioBackend* backend, QObject* parent = nullptr);

    [[nodiscard]] float pan() const;
    [[nodiscard]] float initialVolume() const;
    [[nodiscard]] float fadeMax() const;
    [[nodiscard]] float fadeMin() const;
    [[nodiscard]] int fadeSeconds() const;
    [[nodiscard]] int fadeMode() const;
    [[nodiscard]] bool loop() const;
    [[nodiscard]] double startSeconds() const;
    [[nodiscard]] ChannelEffects* effects() const;
    [[nodiscard]] TrackTimeline* timeline() const;

    void setPan(float value);
    void setInitialVolume(float value);
    void setFadeMax(float value);
    void setFadeMin(float value);
    void setFadeSeconds(int value);
    void setFadeMode(int value);
    void setLoop(bool value);
    void setStartSeconds(double value);

    Q_INVOKABLE void loadFromMap(const QVariantMap& config);
    [[nodiscard]] Q_INVOKABLE QVariantMap toMap() const;

signals:
    void panChanged();
    void initialVolumeChanged();
    void fadeMaxChanged();
    void fadeMinChanged();
    void fadeSecondsChanged();
    void fadeModeChanged();
    void loopChanged();
    void startSecondsChanged();

private:
    ChannelEffects* m_effects { nullptr };
    TrackTimeline* m_timeline { nullptr };

    float m_pan { 0.0f };
    float m_initialVolume { 0.8f };
    float m_fadeMax { 1.0f };
    float m_fadeMin { 0.0f };
    int m_fadeSeconds { 2 };
    int m_fadeMode { 0 };
    bool m_loop { false };
    double m_startSeconds { 0.0 };
};
