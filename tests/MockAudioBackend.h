// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

#include "interfaces/IAudioBackend.h"

#include <QHash>

class MockAudioBackend final : public IAudioBackend
{
    Q_OBJECT

public:
    explicit MockAudioBackend(QObject* parent = nullptr)
        : IAudioBackend(parent)
    {}

    bool initialize() override { return true; }
    void shutdown() override {}

    int loadSource(const QString& filePath) override
    {
        const int id = m_nextVoiceId++;
        m_loaded.insert(id, filePath);
        return id;
    }
    void preloadDecode(const QString& filePath) override { lastPreload = filePath; }
    void releaseSource(int voiceId) override { m_loaded.remove(voiceId); }

    void playVoice(int voiceId, bool restart) override
    {
        lastPlayed = voiceId;
        lastRestart = restart;
    }
    void stopVoice(int voiceId) override { lastStopped = voiceId; }
    void pauseVoice(int voiceId) override { lastPaused = voiceId; }

    void setVoiceVolume(int voiceId, float volume) override
    {
        Q_UNUSED(voiceId) lastVolume = volume;
    }
    void setVoicePan(int voiceId, float pan) override { Q_UNUSED(voiceId) lastPan = pan; }
    void setVoiceLooping(int, bool looping) override { lastLooping = looping; }
    void setVoiceAux(int, bool aux) override { lastAux = aux; }

    void setVoiceReverb(int, bool, float, float, float) override {}
    void setVoiceDelay(int, bool, float, float, float) override {}
    void setVoiceDistortion(int, bool, float, float) override {}
    void setVoicePitch(int, bool, float) override {}
    void setVoiceSpeed(int, bool, float) override {}
    void setVoiceEq(int, bool, const QList<float>&) override {}

    void setVoiceRegion(int, double start, double end) override
    {
        lastRegionStart = start;
        lastRegionEnd = end;
    }
    void setVoicePlayhead(int, double seconds) override { lastPlayhead = seconds; }
    double voiceDurationSeconds(int) const override { return 0.0; }
    QList<float> voiceWaveform(int, int) const override { return {}; }

    void refreshDevices() override {}
    QList<AudioDeviceInfo> outputDevices() const override { return {}; }
    void setOutputDevices(int mainIndex, int auxIndex) override
    {
        lastMainDevice = mainIndex;
        lastAuxDevice = auxIndex;
    }

    void setChannelMasterVolume(float volume) override { channelMasterVolume = volume; }

    void panic() override { panicCount++; }

    // --- Estado observable por los tests ---
    int lastPlayed { k_InvalidVoice };
    bool lastRestart { false };
    int lastStopped { k_InvalidVoice };
    int lastPaused { k_InvalidVoice };
    float lastVolume { -1.0f };
    float lastPan { 0.0f };
    bool lastLooping { false };
    bool lastAux { false };
    int lastMainDevice { -2 };
    int lastAuxDevice { -2 };
    float channelMasterVolume { -1.0f };
    int panicCount { 0 };
    double lastRegionStart { 0.0 };
    double lastRegionEnd { 1.0 };
    double lastPlayhead { -1.0 };
    QString lastPreload;

private:
    int m_nextVoiceId { 0 };
    QHash<int, QString> m_loaded;
};
