// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

#include "interfaces/IAudioBackend.h"

#include "backend/AudioFileDecoder.h"

#include <QHash>
#include <QList>
#include <QMutex>

#include <portaudio.h>

#include <atomic>
#include <memory>
#include <vector>

class QTimer;
class VoiceEffects;

class PortAudioBackend final : public IAudioBackend
{
    Q_OBJECT

public:
    explicit PortAudioBackend(QObject* parent = nullptr);
    ~PortAudioBackend() override;

    [[nodiscard]] bool initialize() override;
    void shutdown() override;

    [[nodiscard]] int loadSource(const QString& filePath) override;
    void preloadDecode(const QString& filePath) override;
    void releaseSource(int voiceId) override;

    void playVoice(int voiceId, bool restart) override;
    void stopVoice(int voiceId) override;
    void pauseVoice(int voiceId) override;

    void setVoiceVolume(int voiceId, float volume) override;
    void setVoicePan(int voiceId, float pan) override;
    void setVoiceLooping(int voiceId, bool looping) override;
    void setVoiceAux(int voiceId, bool aux) override;

    void setVoiceReverb(int voiceId, bool enabled, float roomSize, float damping,
                        float mix) override;
    void setVoiceDelay(int voiceId, bool enabled, float timeMs, float feedback, float mix) override;
    void setVoiceDistortion(int voiceId, bool enabled, float drive, float mix) override;
    void setVoicePitch(int voiceId, bool enabled, float semitones) override;
    void setVoiceSpeed(int voiceId, bool enabled, float tempo) override;
    void setVoiceEq(int voiceId, bool enabled, const QList<float>& gainsDb) override;

    void setVoiceRegion(int voiceId, double startFrac, double endFrac) override;
    void setVoicePlayhead(int voiceId, double seconds) override;
    [[nodiscard]] double voiceDurationSeconds(int voiceId) const override;
    [[nodiscard]] QList<float> voiceWaveform(int voiceId, int buckets) const override;

    void refreshDevices() override;
    [[nodiscard]] QList<AudioDeviceInfo> outputDevices() const override;
    void setOutputDevices(int mainIndex, int auxIndex) override;

    void setChannelMasterVolume(float volume) override;

    void panic() override;

    void setDecodeCacheBytes(qint64 bytes);

    void renderRoute(float* output, unsigned long frameCount, bool auxRoute);

private slots:
    void onVuTimerTick();

private:
    struct Voice
    {
        int id { k_InvalidVoice };
        QString filePath;
        std::shared_ptr<const DecodedAudio> audio;

        double readPos { 0.0 };

        std::atomic<double> positionFrames { 0.0 };
        std::atomic<bool> playing { false };
        std::atomic<bool> looping { false };
        std::atomic<bool> aux { false };
        std::atomic<bool> rendering { false };
        std::atomic<bool> resetRequested { false };
        std::atomic<bool> finished { false };
        std::atomic<float> volume { 1.0f };
        std::atomic<float> pan { 0.0f };
        std::atomic<double> regionStart { 0.0 };
        std::atomic<double> regionEnd { 1.0 };
        std::atomic<double> seekFrames { -1.0 };
        std::atomic<float> peakLeft { 0.0f };
        std::atomic<float> peakRight { 0.0f };

        std::unique_ptr<VoiceEffects> fx;
        bool stActive { false };
    };

    struct StreamContext
    {
        PortAudioBackend* backend { nullptr };
        bool aux { false };
    };

    static int paCallback(const void* input, void* output, unsigned long frameCount,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags, void* userData);

    [[nodiscard]] std::shared_ptr<Voice> voiceFor(int voiceId) const;
    [[nodiscard]] int nextVoiceId();

    [[nodiscard]] PaDeviceIndex preferredDefaultDevice() const;
    [[nodiscard]] PaDeviceIndex resolveDevice(int requestedIndex) const;
    [[nodiscard]] bool openStreamOn(PaDeviceIndex dev, PaStream** stream, void* userData,
                                    double* outSampleRate);
    void closeStream(PaStream** stream);
    void reconfigureStreams();

    [[nodiscard]] std::shared_ptr<const DecodedAudio> cachedDecode(const QString& filePath);
    void cacheStore(const QString& filePath, const std::shared_ptr<const DecodedAudio>& audio);

    PaStream* m_streamMain { nullptr };
    PaStream* m_streamAux { nullptr };
    StreamContext m_mainCtx { this, false };
    StreamContext m_auxCtx { this, true };
    bool m_initialized { false };
    bool m_paReady { false };
    bool m_auxStreamActive { false };
    int m_reqMainIndex { -1 };
    int m_reqAuxIndex { -1 };
    PaDeviceIndex m_mainDeviceIndex { paNoDevice };
    PaDeviceIndex m_auxDeviceIndex { paNoDevice };
    QTimer* m_vuTimer { nullptr };

    mutable QMutex m_voiceMutex;
    QHash<int, std::shared_ptr<Voice>> m_voices;
    int m_nextVoiceId { 0 };

    QHash<QString, std::shared_ptr<const DecodedAudio>> m_decodeCache;
    QList<QString> m_decodeCacheOrder;
    qint64 m_decodeCacheBytes { 0 };
    qint64 m_decodeCacheMaxBytes { k_DecodeCacheMaxBytes };

    std::atomic<float> m_channelMasterVolume { 0.85f };

    std::atomic<float> m_channelBusPeakLeft { 0.0f };
    std::atomic<float> m_channelBusPeakRight { 0.0f };

    std::vector<float> m_voiceScratch;
    std::vector<float> m_stFeed;
    std::vector<float> m_voiceScratchAux;
    std::vector<float> m_stFeedAux;

    double m_outputSampleRate { static_cast<double>(k_SampleRate) };
    double m_auxSampleRate { static_cast<double>(k_SampleRate) };

    static constexpr qint64 k_DecodeCacheMaxBytes = 512LL * 1024 * 1024;
    static constexpr int k_SampleRate = 48000;
    static constexpr int k_Channels = 2;
    static constexpr int k_FramesPerBuffer = 256;
    static constexpr int k_VuIntervalMs = 33;
    static constexpr int k_MaxBlockFrames = 8192;
};
