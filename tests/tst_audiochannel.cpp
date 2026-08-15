// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "MockAudioBackend.h"

#include "AudioChannel.h"

#include <QSignalSpy>
#include <QTemporaryFile>
#include <QtTest>

class TstAudioChannel : public QObject
{
    Q_OBJECT

private slots:
    void should_reportNoTrack_when_created();
    void should_clampVolume_when_valueExceedsMaximum();
    void should_notEmitSignal_when_volumeUnchanged();
    void should_emitVolumeChanged_when_volumeChanges();
    void should_haveTrack_when_bindValidFile();
    void should_failBind_when_fileDoesNotExist();
    void should_clampPan_when_outOfRange();
    void should_forwardLoop_toBackend_when_trackBound();
};

void TstAudioChannel::should_reportNoTrack_when_created()
{
    MockAudioBackend backend;
    AudioChannel ch(0, &backend);
    QVERIFY(!ch.hasTrack());
    QVERIFY(!ch.isPlaying());
    QCOMPARE(ch.name(), QStringLiteral("CH 01"));
}

void TstAudioChannel::should_clampVolume_when_valueExceedsMaximum()
{
    MockAudioBackend backend;
    AudioChannel ch(0, &backend);
    ch.setVolume(3.0f);
    QCOMPARE(ch.volume(), 1.0f);
}

void TstAudioChannel::should_notEmitSignal_when_volumeUnchanged()
{
    MockAudioBackend backend;
    AudioChannel ch(0, &backend);
    ch.setVolume(0.5f);
    QSignalSpy spy(&ch, &AudioChannel::volumeChanged);
    ch.setVolume(0.5f);
    QCOMPARE(spy.count(), 0);
}

void TstAudioChannel::should_emitVolumeChanged_when_volumeChanges()
{
    MockAudioBackend backend;
    AudioChannel ch(0, &backend);
    QSignalSpy spy(&ch, &AudioChannel::volumeChanged);
    ch.setVolume(0.3f);
    QCOMPARE(spy.count(), 1);
}

void TstAudioChannel::should_haveTrack_when_bindValidFile()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    MockAudioBackend backend;
    AudioChannel ch(0, &backend);
    QSignalSpy spy(&ch, &AudioChannel::hasTrackChanged);
    QVERIFY(ch.bindTrack(file.fileName()));
    QVERIFY(ch.hasTrack());
    QCOMPARE(spy.count(), 1);
}

void TstAudioChannel::should_failBind_when_fileDoesNotExist()
{
    MockAudioBackend backend;
    AudioChannel ch(0, &backend);
    QSignalSpy errorSpy(&ch, &AudioChannel::errorOccurred);
    QVERIFY(!ch.bindTrack(QStringLiteral("/no/such/file.wav")));
    QVERIFY(!ch.hasTrack());
    QCOMPARE(errorSpy.count(), 1);
}

void TstAudioChannel::should_clampPan_when_outOfRange()
{
    MockAudioBackend backend;
    AudioChannel ch(0, &backend);
    ch.setPan(5.0f);
    QCOMPARE(ch.pan(), 1.0f);
    ch.setPan(-5.0f);
    QCOMPARE(ch.pan(), -1.0f);
}

void TstAudioChannel::should_forwardLoop_toBackend_when_trackBound()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    MockAudioBackend backend;
    AudioChannel ch(0, &backend);
    QVERIFY(ch.bindTrack(file.fileName()));

    QSignalSpy spy(&ch, &AudioChannel::loopChanged);
    ch.setLoop(true);
    QVERIFY(ch.loop());
    QVERIFY(backend.lastLooping);
    QCOMPARE(spy.count(), 1);

    ch.setLoop(true);
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(TstAudioChannel)
#include "tst_audiochannel.moc"
