// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "backend/PortAudioBackend.h"

#include "AudioEngine.h"
#include "MasterBus.h"
#include "TrackLibrary.h"

#include <QTemporaryDir>
#include <QtTest>

#include <sndfile.h>

#include <vector>

class TstMixer : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;

    QString writeConstMonoWav(int frames)
    {
        const QString path = m_dir.filePath(QStringLiteral("const.wav"));
        SF_INFO info {};
        info.samplerate = 48000;
        info.channels = 1;
        info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        SNDFILE* snd = sf_open(path.toLocal8Bit().constData(), SFM_WRITE, &info);
        Q_ASSERT(snd);
        std::vector<float> buf(static_cast<size_t>(frames), 0.5f);
        sf_writef_float(snd, buf.data(), frames);
        sf_close(snd);
        return path;
    }

    static constexpr unsigned long kFrames = 128;

private slots:
    void should_outputSilence_when_nothingPlaying();
    void should_mixCenteredVoice_atMasterGain();
    void should_applyVolume();
    void should_panHardLeft();
    void should_silence_afterStop();
    void should_produceAudio_throughFullEngineStack();
    void should_resampleToCorrectDuration();
    void should_reportPlaybackPosition();
    void should_keepPlaying_when_looping();
};

void TstMixer::should_outputSilence_when_nothingPlaying()
{
    PortAudioBackend backend;
    std::vector<float> out(kFrames * 2, 1.0f);
    backend.renderRoute(out.data(), kFrames, false);
    for (float s : out) QCOMPARE(s, 0.0f);
}

void TstMixer::should_mixCenteredVoice_atMasterGain()
{
    PortAudioBackend backend;
    backend.setChannelMasterVolume(1.0f);
    const int id = backend.loadSource(writeConstMonoWav(4096));
    QVERIFY(id >= 0);
    backend.setVoiceVolume(id, 1.0f);
    backend.setVoicePan(id, 0.0f);
    backend.playVoice(id, true);

    std::vector<float> out(kFrames * 2, 0.0f);
    backend.renderRoute(out.data(), kFrames, false);
    QVERIFY(qAbs(out[0] - 0.5f) < 0.02f);
    QVERIFY(qAbs(out[1] - 0.5f) < 0.02f);
}

void TstMixer::should_applyVolume()
{
    PortAudioBackend backend;
    backend.setChannelMasterVolume(1.0f);
    const int id = backend.loadSource(writeConstMonoWav(4096));
    backend.setVoiceVolume(id, 0.5f);
    backend.playVoice(id, true);

    std::vector<float> out(kFrames * 2, 0.0f);
    backend.renderRoute(out.data(), kFrames, false);
    QVERIFY(qAbs(out[0] - 0.25f) < 0.02f);
}

void TstMixer::should_panHardLeft()
{
    PortAudioBackend backend;
    backend.setChannelMasterVolume(1.0f);
    const int id = backend.loadSource(writeConstMonoWav(4096));
    backend.setVoiceVolume(id, 1.0f);
    backend.setVoicePan(id, -1.0f);
    backend.playVoice(id, true);

    std::vector<float> out(kFrames * 2, 0.0f);
    backend.renderRoute(out.data(), kFrames, false);
    QVERIFY(qAbs(out[0] - 0.5f) < 0.02f);
    QCOMPARE(out[1], 0.0f);
}

void TstMixer::should_silence_afterStop()
{
    PortAudioBackend backend;
    backend.setChannelMasterVolume(1.0f);
    const int id = backend.loadSource(writeConstMonoWav(4096));
    backend.playVoice(id, true);
    backend.stopVoice(id);

    std::vector<float> out(kFrames * 2, 0.0f);
    backend.renderRoute(out.data(), kFrames, false);
    for (float s : out) QCOMPARE(s, 0.0f);
}

void TstMixer::should_produceAudio_throughFullEngineStack()
{
    PortAudioBackend backend;
    MasterBus master(&backend);
    TrackLibrary lib;
    AudioEngine engine(&backend, &master, &lib);
    master.setChannelMasterVolume(1.0f);

    const QString wav = writeConstMonoWav(4096);
    QVERIFY(engine.bindTrack(0, wav));
    engine.setVolume(0, 1.0f);
    engine.playChannel(0);

    std::vector<float> out(kFrames * 2, 0.0f);
    backend.renderRoute(out.data(), kFrames, false);
    QVERIFY(qAbs(out[0] - 0.5f) < 0.02f);
    QVERIFY(qAbs(out[1] - 0.5f) < 0.02f);

    engine.stopChannel(0);
    std::fill(out.begin(), out.end(), 0.0f);
    backend.renderRoute(out.data(), kFrames, false);
    for (float s : out) QCOMPARE(s, 0.0f);
}

void TstMixer::should_resampleToCorrectDuration()
{
    const QString path = m_dir.filePath(QStringLiteral("half.wav"));
    {
        SF_INFO info {};
        info.samplerate = 24000;
        info.channels = 1;
        info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        SNDFILE* snd = sf_open(path.toLocal8Bit().constData(), SFM_WRITE, &info);
        QVERIFY(snd);
        std::vector<float> buf(24000, 0.5f);
        sf_writef_float(snd, buf.data(), 24000);
        sf_close(snd);
    }

    PortAudioBackend backend;
    const int id = backend.loadSource(path);
    QVERIFY(id >= 0);
    backend.setVoiceVolume(id, 1.0f);
    backend.playVoice(id, true);

    const unsigned long block = 512;
    std::vector<float> out(block * 2, 0.0f);
    int audibleFrames = 0;
    for (int i = 0; i < 300; ++i) {
        std::fill(out.begin(), out.end(), 0.0f);
        backend.renderRoute(out.data(), block, false);
        for (unsigned long f = 0; f < block; ++f) {
            if (std::fabs(out[f * 2]) > 1e-4f) ++audibleFrames;
        }
    }
    QVERIFY2(audibleFrames > 44000 && audibleFrames < 52000,
             qPrintable(QStringLiteral("audibleFrames=%1 (esperado ~48000)").arg(audibleFrames)));
}

void TstMixer::should_reportPlaybackPosition()
{
    PortAudioBackend backend;
    const int id = backend.loadSource(writeConstMonoWav(4800));
    QVERIFY(id >= 0);
    backend.playVoice(id, true);

    std::vector<float> out(2400 * 2, 0.0f);
    backend.renderRoute(out.data(), 2400, false);

    QSignalSpy spy(&backend, &IAudioBackend::voicePositionReady);
    QVERIFY(QMetaObject::invokeMethod(&backend, "onVuTimerTick"));
    QVERIFY(spy.count() >= 1);

    float pos = -1.0f;
    for (const QList<QVariant>& args : spy) {
        if (args.at(0).toInt() == id) pos = args.at(1).toFloat();
    }
    QVERIFY2(pos > 0.45f && pos < 0.55f,
             qPrintable(QStringLiteral("progress=%1 (esperado ~0.5)").arg(pos)));

    backend.stopVoice(id);
    QSignalSpy spy2(&backend, &IAudioBackend::voicePositionReady);
    QVERIFY(QMetaObject::invokeMethod(&backend, "onVuTimerTick"));
    float posAfterStop = -1.0f;
    for (const QList<QVariant>& args : spy2) {
        if (args.at(0).toInt() == id) posAfterStop = args.at(1).toFloat();
    }
    QCOMPARE(posAfterStop, 0.0f);
}

void TstMixer::should_keepPlaying_when_looping()
{
    PortAudioBackend backend;
    const int id = backend.loadSource(writeConstMonoWav(256));
    QVERIFY(id >= 0);
    backend.setVoiceVolume(id, 1.0f);
    backend.setVoiceLooping(id, true);
    backend.playVoice(id, true);

    std::vector<float> out(256 * 2, 0.0f);
    bool audibleAtEnd = false;
    for (int b = 0; b < 10; ++b) {
        std::fill(out.begin(), out.end(), 0.0f);
        backend.renderRoute(out.data(), 256, false);
        audibleAtEnd = std::fabs(out[0]) > 1e-4f;
    }
    QVERIFY2(audibleAtEnd, "la voz en bucle debe seguir sonando tras varias repeticiones");

    backend.setVoiceLooping(id, false);
    backend.playVoice(id, true);
    for (int b = 0; b < 10; ++b) {
        std::fill(out.begin(), out.end(), 0.0f);
        backend.renderRoute(out.data(), 256, false);
    }
    QCOMPARE(out[0], 0.0f);
}

QTEST_MAIN(TstMixer)
#include "tst_mixer.moc"
