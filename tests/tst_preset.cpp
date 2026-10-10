// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "AudioChannel.h"
#include "AudioEngine.h"
#include "ChannelEffects.h"
#include "MasterBus.h"
#include "PresetManager.h"
#include "TrackLibrary.h"
#include "TrackTimeline.h"

#include "MockAudioBackend.h"

#include <QColor>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class TstPreset : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;

    QString makeAudioFile(const QString& name)
    {
        const QString path = m_dir.filePath(name);
        QFile f(path);
        [[maybe_unused]] const bool ok = f.open(QIODevice::WriteOnly);
        Q_ASSERT(ok);
        f.close();
        return path;
    }

private slots:
    void should_exposeTenPresets_byDefault();
    void should_restoreIndependentChannelConfig_when_switchingBackAndForth();
    void should_recallMasterVolumes_perPreset();
    void should_recallEffects_perPreset();
    void should_recallRegion_perPreset();
    void should_persistChannelOrder_acrossJson();
    void should_reapplyConfigWithoutRebind_when_sameTrackAcrossPresets();
    void should_stopPlayback_when_switchingPreset();
    void should_resetMeters_when_switchingPreset();
    void should_resetToDefaults_when_selectingUnusedPreset();
    void should_reportBindingsAcrossPresets();
    void should_unbindTrackFromAllPresets_when_removed();
    void should_ignore_invalidOrSameIndex();
};

void TstPreset::should_exposeTenPresets_byDefault()
{
    MockAudioBackend backend;
    MasterBus master(&backend);
    TrackLibrary lib;
    AudioEngine engine(&backend, &master, &lib);
    PresetManager presets(&engine, &master);

    QCOMPARE(presets.count(), 10);
    QCOMPARE(presets.activePreset(), 0);
}

void TstPreset::should_restoreIndependentChannelConfig_when_switchingBackAndForth()
{
    MockAudioBackend backend;
    MasterBus master(&backend);
    TrackLibrary lib;
    AudioEngine engine(&backend, &master, &lib);
    PresetManager presets(&engine, &master);

    engine.channelAt(0)->setName(QStringLiteral("Bajo"));
    engine.setVolume(0, 0.6f);

    QSignalSpy spy(&presets, &PresetManager::activePresetChanged);

    presets.selectPreset(1);
    QCOMPARE(presets.activePreset(), 1);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(engine.channelAt(0)->name(), AudioChannel::defaultName(0));
    QVERIFY(qAbs(engine.channelAt(0)->volume() - 0.8f) < 1e-4f);

    engine.channelAt(0)->setName(QStringLiteral("Voz"));
    engine.setVolume(0, 0.3f);

    presets.selectPreset(0);
    QCOMPARE(engine.channelAt(0)->name(), QStringLiteral("Bajo"));
    QVERIFY(qAbs(engine.channelAt(0)->volume() - 0.6f) < 1e-4f);

    presets.selectPreset(1);
    QCOMPARE(engine.channelAt(0)->name(), QStringLiteral("Voz"));
    QVERIFY(qAbs(engine.channelAt(0)->volume() - 0.3f) < 1e-4f);
}

void TstPreset::should_recallMasterVolumes_perPreset()
{
    MockAudioBackend backend;
    MasterBus master(&backend);
    TrackLibrary lib;
    AudioEngine engine(&backend, &master, &lib);
    PresetManager presets(&engine, &master);

    master.setChannelMasterVolume(0.42f);

    presets.selectPreset(1);
    QVERIFY(qAbs(master.channelMasterVolume() - 0.85f) < 1e-4f);

    master.setChannelMasterVolume(0.10f);

    presets.selectPreset(0);
    QVERIFY(qAbs(master.channelMasterVolume() - 0.42f) < 1e-4f);
}

void TstPreset::should_recallEffects_perPreset()
{
    MockAudioBackend backend;
    MasterBus master(&backend);
    TrackLibrary lib;
    AudioEngine engine(&backend, &master, &lib);
    PresetManager presets(&engine, &master);

    ChannelEffects* fx = engine.channelAt(0)->effects();
    fx->setReverbEnabled(true);
    fx->setReverbMix(0.6f);
    fx->setPitchEnabled(true);
    fx->setPitchSemitones(5.0f);
    fx->setEqEnabled(true);
    fx->setEqBandGain(0, 4.0f);
    fx->setEqBandGain(9, -3.0f);

    presets.selectPreset(1);
    QVERIFY(!fx->reverbEnabled());
    QVERIFY(qAbs(fx->pitchSemitones()) < 1e-4f);
    QVERIFY(!fx->eqEnabled());
    QVERIFY(qAbs(fx->eqBands().at(0).toFloat()) < 1e-4f);

    presets.selectPreset(0);
    QVERIFY(fx->reverbEnabled());
    QVERIFY(qAbs(fx->reverbMix() - 0.6f) < 1e-4f);
    QVERIFY(fx->pitchEnabled());
    QVERIFY(qAbs(fx->pitchSemitones() - 5.0f) < 1e-4f);
    QVERIFY(fx->eqEnabled());
    QVERIFY(qAbs(fx->eqBands().at(0).toFloat() - 4.0f) < 1e-4f);
    QVERIFY(qAbs(fx->eqBands().at(9).toFloat() + 3.0f) < 1e-4f);
}

void TstPreset::should_recallRegion_perPreset()
{
    MockAudioBackend backend;
    MasterBus master(&backend);
    TrackLibrary lib;
    AudioEngine engine(&backend, &master, &lib);
    PresetManager presets(&engine, &master);

    TrackTimeline* tl = engine.channelAt(0)->timeline();
    tl->setStart(0.2);
    tl->setEnd(0.8);
    tl->setRegionEnabled(false);

    presets.selectPreset(1);
    QVERIFY(!tl->hasStart());
    QVERIFY(!tl->hasEnd());
    QVERIFY(tl->regionEnabled());

    presets.selectPreset(0);
    QVERIFY(tl->hasStart());
    QVERIFY(tl->hasEnd());
    QVERIFY(qAbs(tl->start() - 0.2) < 1e-4);
    QVERIFY(qAbs(tl->end() - 0.8) < 1e-4);
    QVERIFY(!tl->regionEnabled());
}

void TstPreset::should_persistChannelOrder_acrossJson()
{
    MockAudioBackend backend;
    MasterBus master(&backend);
    TrackLibrary lib;

    AudioEngine engine(&backend, &master, &lib);
    PresetManager presets(&engine, &master);

    QVariantList reversed;
    for (int i = engine.channelCount() - 1; i >= 0; --i) reversed.append(i);
    engine.setChannelOrder(reversed);
    engine.setGridMode(true);

    const QJsonObject root = presets.toJson();

    AudioEngine engine2(&backend, &master, &lib);
    PresetManager presets2(&engine2, &master);
    presets2.applyJson(root);

    QCOMPARE(engine2.channelOrder(), reversed);
    QVERIFY(engine2.gridMode());
}

void TstPreset::should_reapplyConfigWithoutRebind_when_sameTrackAcrossPresets()
{
    MockAudioBackend backend;
    MasterBus master(&backend);
    TrackLibrary lib;
    AudioEngine engine(&backend, &master, &lib);
    PresetManager presets(&engine, &master);

    const QString wav = makeAudioFile(QStringLiteral("shared.wav"));

    QVERIFY(engine.bindTrack(0, wav));
    engine.channelAt(0)->effects()->setReverbEnabled(true);

    presets.selectPreset(1);
    QVERIFY(engine.bindTrack(0, wav));
    engine.channelAt(0)->effects()->setReverbEnabled(false);
    const int voiceBefore = engine.channelAt(0)->voiceId();

    presets.selectPreset(0);
    QCOMPARE(engine.channelAt(0)->filePath(), wav);
    QCOMPARE(engine.channelAt(0)->voiceId(), voiceBefore);
    QVERIFY(engine.channelAt(0)->effects()->reverbEnabled());
}

void TstPreset::should_stopPlayback_when_switchingPreset()
{
    MockAudioBackend backend;
    MasterBus master(&backend);
    TrackLibrary lib;
    AudioEngine engine(&backend, &master, &lib);
    PresetManager presets(&engine, &master);

    const QString wav = makeAudioFile(QStringLiteral("track.wav"));
    QVERIFY(engine.bindTrack(0, wav));
    engine.playChannel(0);
    QVERIFY(engine.channelAt(0)->isPlaying());

    presets.selectPreset(1);

    QVERIFY(!engine.channelAt(0)->isPlaying());
}

void TstPreset::should_resetMeters_when_switchingPreset()
{
    MockAudioBackend backend;
    MasterBus master(&backend);
    TrackLibrary lib;
    AudioEngine engine(&backend, &master, &lib);
    PresetManager presets(&engine, &master);

    engine.channelAt(0)->updateVuLevel(0.8f, 0.7f);
    master.updateMasterVu(0.6f, 0.5f);

    presets.selectPreset(1);

    QCOMPARE(engine.channelAt(0)->vuLeft(), 0.0f);
    QCOMPARE(engine.channelAt(0)->vuRight(), 0.0f);
    QCOMPARE(master.masterVuLeft(), 0.0f);
    QCOMPARE(master.masterVuRight(), 0.0f);
}

void TstPreset::should_resetToDefaults_when_selectingUnusedPreset()
{
    MockAudioBackend backend;
    MasterBus master(&backend);
    TrackLibrary lib;
    AudioEngine engine(&backend, &master, &lib);
    PresetManager presets(&engine, &master);

    const QString wav = makeAudioFile(QStringLiteral("track.wav"));
    QVERIFY(engine.bindTrack(2, wav));
    engine.channelAt(2)->setColor(QColor(QStringLiteral("#3399ff")));
    engine.channelAt(2)->setLoop(true);
    QVERIFY(engine.channelAt(2)->hasTrack());

    presets.selectPreset(5);

    const AudioChannel* ch = engine.channelAt(2);
    QVERIFY(!ch->hasTrack());
    QVERIFY(!ch->loop());
    QCOMPARE(ch->color(), QColor(0xE8, 0x60, 0x1C));
}

void TstPreset::should_reportBindingsAcrossPresets()
{
    MockAudioBackend backend;
    MasterBus master(&backend);
    TrackLibrary lib;
    AudioEngine engine(&backend, &master, &lib);
    PresetManager presets(&engine, &master);

    const QString wav = makeAudioFile(QStringLiteral("t.wav"));

    QVERIFY(engine.bindTrack(2, wav));
    presets.selectPreset(1);
    QVERIFY(engine.bindTrack(4, wav));

    QCOMPARE(presets.bindingsFor(wav), QStringLiteral("P1·C3, P2·C5"));

    QSignalSpy spy(&presets, &PresetManager::bindingsChanged);
    QVERIFY(engine.bindTrack(0, wav));
    QVERIFY(spy.count() >= 1);
    QCOMPARE(presets.bindingsFor(wav), QStringLiteral("P1·C3, P2·C1, P2·C5"));
}

void TstPreset::should_unbindTrackFromAllPresets_when_removed()
{
    MockAudioBackend backend;
    MasterBus master(&backend);
    TrackLibrary lib;
    AudioEngine engine(&backend, &master, &lib);
    PresetManager presets(&engine, &master);

    const QString wav = makeAudioFile(QStringLiteral("shared.wav"));

    QVERIFY(engine.bindTrack(2, wav));
    presets.selectPreset(1);
    QVERIFY(engine.bindTrack(4, wav));
    QCOMPARE(presets.bindingsFor(wav), QStringLiteral("P1·C3, P2·C5"));

    QSignalSpy spy(&presets, &PresetManager::bindingsChanged);
    presets.unbindTrackEverywhere(wav);

    QCOMPARE(presets.bindingsFor(wav), QString());
    QVERIFY(!engine.channelAt(4)->hasTrack());
    QVERIFY(spy.count() >= 1);

    presets.selectPreset(0);
    QVERIFY(!engine.channelAt(2)->hasTrack());
}

void TstPreset::should_ignore_invalidOrSameIndex()
{
    MockAudioBackend backend;
    MasterBus master(&backend);
    TrackLibrary lib;
    AudioEngine engine(&backend, &master, &lib);
    PresetManager presets(&engine, &master);

    QSignalSpy spy(&presets, &PresetManager::activePresetChanged);
    presets.selectPreset(0);
    presets.selectPreset(-1);
    presets.selectPreset(99);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(presets.activePreset(), 0);
}

QTEST_MAIN(TstPreset)
#include "tst_preset.moc"
