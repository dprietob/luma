// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

#include <QColor>
#include <QJsonObject>
#include <QObject>
#include <QString>

#include "ChannelEffects.h"
#include "TrackTimeline.h"
#include "interfaces/IPlayable.h"
#include "interfaces/IPannable.h"

class IAudioBackend;

class AudioChannel final : public QObject, public IPlayable, public IPannable
{
    Q_OBJECT

    Q_PROPERTY(int id READ id CONSTANT FINAL)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged FINAL)
    Q_PROPERTY(QString filePath READ filePath NOTIFY filePathChanged FINAL)
    Q_PROPERTY(bool hasTrack READ hasTrack NOTIFY hasTrackChanged FINAL)
    Q_PROPERTY(float volume READ volume WRITE setVolume NOTIFY volumeChanged FINAL)
    Q_PROPERTY(float pan READ pan WRITE setPan NOTIFY panChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(bool loop READ loop WRITE setLoop NOTIFY loopChanged FINAL)
    Q_PROPERTY(bool aux READ aux WRITE setAux NOTIFY auxChanged FINAL)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged FINAL)
    Q_PROPERTY(bool isPaused READ isPaused NOTIFY isPausedChanged FINAL)
    Q_PROPERTY(int fadeSeconds READ fadeSeconds WRITE setFadeSeconds NOTIFY fadeSecondsChanged FINAL)
    Q_PROPERTY(int fadeMode READ fadeMode WRITE setFadeMode NOTIFY fadeModeChanged FINAL)
    Q_PROPERTY(float fadeMax READ fadeMax WRITE setFadeMax NOTIFY fadeMaxChanged FINAL)
    Q_PROPERTY(float fadeMin READ fadeMin WRITE setFadeMin NOTIFY fadeMinChanged FINAL)
    Q_PROPERTY(float vuLeft READ vuLeft NOTIFY vuLevelChanged FINAL)
    Q_PROPERTY(float vuRight READ vuRight NOTIFY vuLevelChanged FINAL)
    Q_PROPERTY(float progress READ progress NOTIFY progressChanged FINAL)
    Q_PROPERTY(ChannelEffects* effects READ effects CONSTANT FINAL)
    Q_PROPERTY(TrackTimeline* timeline READ timeline CONSTANT FINAL)

public:
    explicit AudioChannel(int id, IAudioBackend* backend, QObject* parent = nullptr);
    ~AudioChannel() override;

    // --- IPlayable ---
    void play() override;
    void stop() override;
    void setVolume(float volume) override;
    [[nodiscard]] bool isPlaying() const override;

    void pause();

    // --- IPannable ---
    void setPan(float pan) override;

    [[nodiscard]] static QString defaultName(int id);
    [[nodiscard]] static QColor defaultColor();

    // --- Getters ---
    [[nodiscard]] int id() const;
    [[nodiscard]] QString name() const;
    [[nodiscard]] QString filePath() const;
    [[nodiscard]] bool hasTrack() const;
    [[nodiscard]] float volume() const;
    [[nodiscard]] float pan() const;
    [[nodiscard]] QColor color() const;
    [[nodiscard]] bool loop() const;
    [[nodiscard]] bool aux() const;
    [[nodiscard]] bool isPaused() const;
    [[nodiscard]] int fadeSeconds() const;
    [[nodiscard]] int fadeMode() const;
    [[nodiscard]] float fadeMax() const;
    [[nodiscard]] float fadeMin() const;
    [[nodiscard]] float vuLeft() const;
    [[nodiscard]] float vuRight() const;
    [[nodiscard]] float progress() const;
    [[nodiscard]] ChannelEffects* effects() const;
    [[nodiscard]] TrackTimeline* timeline() const;

    void setName(const QString& name);
    void setColor(const QColor& color);
    void setFadeSeconds(int seconds);
    void setFadeMode(int mode);
    void setFadeMax(float value);
    void setFadeMin(float value);
    void setLoop(bool loop);
    void setAux(bool aux);

    [[nodiscard]] int voiceId() const;

    [[nodiscard]] bool bindTrack(const QString& filePath);
    void unbindTrack();

    void reset();
    void releaseAudio();

    void setPlayhead(double seconds);
    [[nodiscard]] QJsonObject captureState() const;
    void restoreState(const QJsonObject& state, bool restoreTransport);

    void updateVuLevel(float left, float right);

    void updateProgress(float progress);

    void resetTransport();

signals:
    void nameChanged();
    void filePathChanged();
    void hasTrackChanged();
    void volumeChanged();
    void panChanged();
    void colorChanged();
    void fadeSecondsChanged();
    void fadeModeChanged();
    void fadeMaxChanged();
    void fadeMinChanged();
    void loopChanged();
    void auxChanged();
    void isPlayingChanged();
    void isPausedChanged();
    void vuLevelChanged();
    void progressChanged();
    void errorOccurred(const QString& message);

private:
    void applyEffectiveVolume();
    void setPlaying(bool playing);

    static constexpr float k_MaxVolume = 1.0f;
    static constexpr float k_DefaultVolume = 0.8f;
    static constexpr int k_DefaultFadeSeconds = 2;

    const int m_id;
    IAudioBackend* m_backend;
    ChannelEffects* m_effects { nullptr };
    TrackTimeline* m_timeline { nullptr };
    int m_voiceId { -1 };

    QString m_name;
    QString m_filePath;
    QColor m_color { defaultColor() };
    float m_volume { 0.8f };
    float m_pan { 0.0f };
    bool m_loop { false };
    bool m_aux { false };
    bool m_isPlaying { false };
    bool m_isPaused { false };
    float m_vuLeft { 0.0f };
    float m_vuRight { 0.0f };
    float m_progress { 0.0f };
    int m_fadeSeconds { 2 };
    int m_fadeMode { 0 };
    float m_fadeMax { 1.0f };
    float m_fadeMin { 0.0f };
};
