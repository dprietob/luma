// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "backend/AudioFileDecoder.h"

#include <QDir>
#include <QTemporaryDir>
#include <QtTest>

#include <sndfile.h>

#include <vector>

class TstDecoder : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;

    QString writeWav(const QString& name, int sampleRate, int channels, int frames)
    {
        const QString path = m_dir.filePath(name);
        SF_INFO info {};
        info.samplerate = sampleRate;
        info.channels = channels;
        info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        SNDFILE* snd = sf_open(path.toLocal8Bit().constData(), SFM_WRITE, &info);
        Q_ASSERT(snd);
        std::vector<float> buf(static_cast<size_t>(frames) * channels);
        for (int f = 0; f < frames; ++f) {
            for (int c = 0; c < channels; ++c) {
                buf[static_cast<size_t>(f) * channels + c] = (c == 0 ? 0.5f : -0.5f);
            }
        }
        sf_writef_float(snd, buf.data(), frames);
        sf_close(snd);
        return path;
    }

private slots:
    void should_decodeStereoWav_preservingChannels();
    void should_upmixMonoWav_toStereo();
    void should_decodeMp3_withNormalizedSamples();
    void should_probeDuration_forWavAndMp3();
    void should_fail_when_fileMissing();
};

void TstDecoder::should_decodeStereoWav_preservingChannels()
{
    const QString path = writeWav(QStringLiteral("stereo.wav"), 44100, 2, 500);
    QString err;
    const auto audio = AudioFileDecoder::decode(path, &err);
    QVERIFY2(audio.has_value(), qPrintable(err));
    QCOMPARE(audio->sampleRate, 44100);
    QCOMPARE(audio->frameCount(), static_cast<std::int64_t>(500));
    QVERIFY(qAbs(audio->samples[0] - 0.5f) < 0.01f);
    QVERIFY(qAbs(audio->samples[1] + 0.5f) < 0.01f);
}

void TstDecoder::should_upmixMonoWav_toStereo()
{
    const QString path = writeWav(QStringLiteral("mono.wav"), 22050, 1, 200);
    const auto audio = AudioFileDecoder::decode(path);
    QVERIFY(audio.has_value());
    QCOMPARE(audio->sampleRate, 22050);
    QCOMPARE(audio->frameCount(), static_cast<std::int64_t>(200));
    QCOMPARE(audio->samples[0], audio->samples[1]);
}

void TstDecoder::should_decodeMp3_withNormalizedSamples()
{
    const QString path = QStringLiteral(FIXTURES_DIR "/sine440.mp3");
    QString err;
    const auto audio = AudioFileDecoder::decode(path, &err);
    QVERIFY2(audio.has_value(), qPrintable(err));
    QCOMPARE(audio->sampleRate, 44100);

    const double seconds = static_cast<double>(audio->frameCount()) / audio->sampleRate;
    QVERIFY2(seconds > 2.0 && seconds < 4.0, qPrintable(QStringLiteral("dur=%1s").arg(seconds)));

    float peak = 0.0f;
    for (float s : audio->samples) peak = std::max(peak, std::fabs(s));
    QVERIFY2(peak <= 1.0f, qPrintable(QStringLiteral("peak=%1 (fuera de rango)").arg(peak)));
    QVERIFY2(peak > 0.3f && peak < 0.7f,
             qPrintable(QStringLiteral("peak=%1 (esperado ~0.5)").arg(peak)));
}

void TstDecoder::should_probeDuration_forWavAndMp3()
{
    const QString wav = writeWav(QStringLiteral("onesec.wav"), 44100, 2, 44100);
    QCOMPARE(AudioFileDecoder::durationMs(wav), 1000);

    const int mp3Ms = AudioFileDecoder::durationMs(QStringLiteral(FIXTURES_DIR "/sine440.mp3"));
    QVERIFY2(mp3Ms > 2000 && mp3Ms < 4000, qPrintable(QStringLiteral("mp3Ms=%1").arg(mp3Ms)));

    QCOMPARE(AudioFileDecoder::durationMs(m_dir.filePath("nope.wav")), 0);
}

void TstDecoder::should_fail_when_fileMissing()
{
    QString err;
    const auto audio = AudioFileDecoder::decode(m_dir.filePath("nope.wav"), &err);
    QVERIFY(!audio.has_value());
    QVERIFY(!err.isEmpty());
}

QTEST_MAIN(TstDecoder)
#include "tst_decoder.moc"
