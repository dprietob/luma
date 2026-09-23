// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

#include <QList>
#include <QObject>
#include <QString>

struct AudioDeviceInfo
{
    int index { -1 };
    QString name;
};

class IAudioBackend : public QObject
{
    Q_OBJECT

public:
    static constexpr int k_InvalidVoice = -1;

    explicit IAudioBackend(QObject* parent = nullptr)
        : QObject(parent)
    {}
    ~IAudioBackend() override = default;

    [[nodiscard]] virtual bool initialize() = 0;

    virtual void shutdown() = 0;

    [[nodiscard]] virtual int loadSource(const QString& filePath) = 0;

    virtual void preloadDecode(const QString& filePath) = 0;

    virtual void releaseSource(int voiceId) = 0;

    virtual void playVoice(int voiceId, bool restart) = 0;
    virtual void stopVoice(int voiceId) = 0;
    virtual void pauseVoice(int voiceId) = 0;

    virtual void setVoiceVolume(int voiceId, float volume) = 0;
    virtual void setVoicePan(int voiceId, float pan) = 0;
    virtual void setVoiceLooping(int voiceId, bool looping) = 0;

    virtual void setVoiceAux(int voiceId, bool aux) = 0;

    virtual void setVoiceReverb(int voiceId, bool enabled, float roomSize, float damping,
                                float mix) = 0;
    virtual void setVoiceDelay(int voiceId, bool enabled, float timeMs, float feedback,
                               float mix) = 0;
    virtual void setVoiceDistortion(int voiceId, bool enabled, float drive, float mix) = 0;
    virtual void setVoicePitch(int voiceId, bool enabled, float semitones) = 0;
    virtual void setVoiceSpeed(int voiceId, bool enabled, float tempo) = 0;
    virtual void setVoiceEq(int voiceId, bool enabled, const QList<float>& gainsDb) = 0;

    virtual void setVoiceRegion(int voiceId, double startFrac, double endFrac) = 0;
    virtual void setVoicePlayhead(int voiceId, double seconds) = 0;
    [[nodiscard]] virtual double voiceDurationSeconds(int voiceId) const = 0;
    [[nodiscard]] virtual QList<float> voiceWaveform(int voiceId, int buckets) const = 0;

    virtual void refreshDevices() = 0;
    [[nodiscard]] virtual QList<AudioDeviceInfo> outputDevices() const = 0;
    virtual void setOutputDevices(int mainIndex, int auxIndex) = 0;

    virtual void setChannelMasterVolume(float volume) = 0;

    virtual void panic() = 0;

signals:
    void vuLevelReady(int voiceId, float left, float right);

    void channelBusVuReady(float left, float right);

    void voicePositionReady(int voiceId, float position);

    void voiceFinished(int voiceId);

    void errorOccurred(const QString& message);
};
