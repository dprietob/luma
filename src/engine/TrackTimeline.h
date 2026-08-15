// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QVariantList>

class IAudioBackend;

class TrackTimeline final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool regionEnabled READ regionEnabled WRITE setRegionEnabled NOTIFY regionChanged FINAL)
    Q_PROPERTY(bool hasStart READ hasStart NOTIFY regionChanged FINAL)
    Q_PROPERTY(double start READ start NOTIFY regionChanged FINAL)
    Q_PROPERTY(bool hasEnd READ hasEnd NOTIFY regionChanged FINAL)
    Q_PROPERTY(double end READ end NOTIFY regionChanged FINAL)
    Q_PROPERTY(double durationSeconds READ durationSeconds NOTIFY durationChanged FINAL)
    Q_PROPERTY(double regionSeconds READ regionSeconds NOTIFY regionChanged FINAL)
    Q_PROPERTY(QVariantList waveform READ waveform NOTIFY waveformChanged FINAL)

public:
    explicit TrackTimeline(IAudioBackend* backend, QObject* parent = nullptr);

    [[nodiscard]] bool regionEnabled() const;
    [[nodiscard]] bool hasStart() const;
    [[nodiscard]] double start() const;
    [[nodiscard]] bool hasEnd() const;
    [[nodiscard]] double end() const;
    [[nodiscard]] double durationSeconds() const;
    [[nodiscard]] double regionSeconds() const;
    [[nodiscard]] QVariantList waveform() const;

    void setRegionEnabled(bool enabled);
    Q_INVOKABLE void setStart(double frac);
    Q_INVOKABLE void setEnd(double frac);
    Q_INVOKABLE void clearStart();
    Q_INVOKABLE void clearEnd();
    Q_INVOKABLE void reset();

    void bindVoice(int voiceId);
    void unbindVoice();

    [[nodiscard]] QJsonObject toJson() const;
    void applyJson(const QJsonObject& o);

signals:
    void regionChanged();
    void durationChanged();
    void waveformChanged();

private:
    void pushRegion() const;
    [[nodiscard]] double configuredStart() const;
    [[nodiscard]] double configuredEnd() const;

    static constexpr int k_WaveformBuckets = 512;
    static constexpr double k_MinGap = 0.005;

    IAudioBackend* m_backend;
    int m_voiceId;

    bool m_enabled { true };
    bool m_hasStart { false };
    double m_start { 0.0 };
    bool m_hasEnd { false };
    double m_end { 1.0 };
    double m_durationSeconds { 0.0 };
    QVariantList m_waveform;
};
