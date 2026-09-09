// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

#include <QList>
#include <QObject>
#include <QQmlListProperty>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariant>
#include <QVector>

#include "AudioChannel.h"
#include "MasterBus.h"
#include "TrackLibrary.h"

class IAudioBackend;
class QTimer;

class AudioEngine final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQmlListProperty<AudioChannel> channels READ channels NOTIFY channelsChanged FINAL)
    Q_PROPERTY(bool warming READ warming NOTIFY warmingChanged FINAL)
    Q_PROPERTY(int channelCount READ channelCount WRITE setChannelCount NOTIFY channelCountChanged FINAL)
    Q_PROPERTY(bool gridMode READ gridMode WRITE setGridMode NOTIFY gridModeChanged FINAL)
    Q_PROPERTY(MasterBus* masterBus READ masterBus CONSTANT FINAL)
    Q_PROPERTY(TrackLibrary* trackLibrary READ trackLibrary CONSTANT FINAL)

public:
    static constexpr int k_DefaultChannelCount = 12;
    static constexpr int k_MinChannelCount = 1;
    static constexpr int k_MaxChannelCount = 40;

    AudioEngine(IAudioBackend* backend, MasterBus* masterBus, TrackLibrary* trackLibrary,
                QObject* parent = nullptr);
    ~AudioEngine() override;

    [[nodiscard]] QQmlListProperty<AudioChannel> channels();
    [[nodiscard]] MasterBus* masterBus() const;
    [[nodiscard]] TrackLibrary* trackLibrary() const;

    [[nodiscard]] Q_INVOKABLE AudioChannel* channelAt(int id) const;

    [[nodiscard]] int channelCount() const;
    void setChannelCount(int count);

    // --- Control de canales (PROJECT.md §3.2) ---
    Q_INVOKABLE void playChannel(int id);
    Q_INVOKABLE void stopChannel(int id);
    Q_INVOKABLE void pauseChannel(int id);
    Q_INVOKABLE void setVolume(int id, float volume);
    Q_INVOKABLE void setPan(int id, float pan);
    Q_INVOKABLE bool bindTrack(int channelId, const QString& filePath);
    Q_INVOKABLE bool bindTrackFromUrl(int channelId, const QUrl& url);
    Q_INVOKABLE void unbindChannel(int id);
    Q_INVOKABLE void resetChannelColor(int id);

    [[nodiscard]] Q_INVOKABLE QVariantList channelOrder() const;
    Q_INVOKABLE void setChannelOrder(const QVariantList& order);

    [[nodiscard]] bool gridMode() const;
    void setGridMode(bool gridMode);

    // --- Control global ---
    Q_INVOKABLE void panic();
    Q_INVOKABLE void stopAll();
    Q_INVOKABLE void warmTrackCache();

    [[nodiscard]] bool warming() const;

signals:
    void vuLevelChanged(int id, float left, float right);
    void gridModeChanged();
    void channelsChanged();
    void channelCountChanged();
    void panicRequested();
    void warmingChanged();
    void errorOccurred(const QString& message);

private slots:
    void onBackendVuLevel(int voiceId, float left, float right);
    void onBackendVoicePosition(int voiceId, float position);
    void onBackendVoiceFinished(int voiceId);
    void onWarmTick();

private:
    [[nodiscard]] bool isValidChannel(int id) const;
    void normalizeChannelOrder();

    static qsizetype channelListCount(QQmlListProperty<AudioChannel>* list);
    static AudioChannel* channelAtIndex(QQmlListProperty<AudioChannel>* list, qsizetype i);

    IAudioBackend* m_backend;
    MasterBus* m_masterBus;
    TrackLibrary* m_trackLibrary;

    QVector<AudioChannel*> m_channels;
    QList<int> m_channelOrder;
    int m_channelCount { k_DefaultChannelCount };
    bool m_gridMode { false };

    QTimer* m_warmTimer { nullptr };
    QStringList m_warmQueue;
    bool m_warming { false };
};
