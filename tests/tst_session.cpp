// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "AudioChannel.h"
#include "AudioEngine.h"
#include "MasterBus.h"
#include "PresetManager.h"
#include "SessionManager.h"
#include "TrackLibrary.h"

#include "MockAudioBackend.h"

#include <QColor>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class TstSession : public QObject
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
    void should_roundTripFullSessionState();
    void should_persistIndividuallyAddedTracks();
    void should_returnFalse_when_noSessionFile();
    void should_emitError_when_sessionCorrupt();
    void should_writeExpectedTopLevelKeys();
};

void TstSession::should_roundTripFullSessionState()
{
    const QString sessionPath = m_dir.filePath(QStringLiteral("roundtrip.json"));
    const QString wav = makeAudioFile(QStringLiteral("track.wav"));
    const QString libDir = m_dir.path();

    // --- Grafo de origen: fija estado y guarda ---
    {
        MockAudioBackend backend;
        MasterBus master(&backend);
        TrackLibrary lib;
        AudioEngine engine(&backend, &master, &lib);
        PresetManager presets(&engine, &master);
        SessionManager session(&lib, &presets, sessionPath);

        master.setChannelMasterVolume(0.42f);

        engine.channelAt(0)->setName(QStringLiteral("Bajo"));
        QVERIFY(engine.bindTrack(0, wav));
        engine.setVolume(0, 0.6f);
        engine.setPan(0, -0.3f);
        engine.channelAt(0)->setColor(QColor(QStringLiteral("#3399ff")));
        engine.channelAt(0)->setFadeSeconds(5);
        engine.channelAt(0)->setFadeMode(4);
        engine.channelAt(0)->setLoop(true);

        lib.scanDirectory(libDir);

        QVERIFY(session.save());
        QVERIFY(QFile::exists(sessionPath));
    }

    // --- Grafo destino: carga y verifica ---
    {
        MockAudioBackend backend;
        MasterBus master(&backend);
        TrackLibrary lib;
        AudioEngine engine(&backend, &master, &lib);
        PresetManager presets(&engine, &master);
        SessionManager session(&lib, &presets, sessionPath);

        QVERIFY(session.load());

        QVERIFY(qAbs(master.channelMasterVolume() - 0.42f) < 1e-4f);

        const AudioChannel* ch = engine.channelAt(0);
        QCOMPARE(ch->name(), QStringLiteral("Bajo"));
        QVERIFY(ch->hasTrack());
        QCOMPARE(ch->filePath(), wav);
        QVERIFY(qAbs(ch->volume() - 0.6f) < 1e-4f);
        QVERIFY(qAbs(ch->pan() - (-0.3f)) < 1e-4f);
        QCOMPARE(presets.bindingsFor(wav), QStringLiteral("P1·C1"));
        QCOMPARE(ch->color(), QColor(QStringLiteral("#3399ff")));
        QCOMPARE(ch->fadeSeconds(), 5);
        QCOMPARE(ch->fadeMode(), 4);
        QVERIFY(ch->loop());

        QVERIFY(lib.libraryPaths().contains(libDir));
    }
}

void TstSession::should_persistIndividuallyAddedTracks()
{
    const QString sessionPath = m_dir.filePath(QStringLiteral("tracks.json"));
    const QString solo = makeAudioFile(QStringLiteral("solo.wav"));

    {
        MockAudioBackend backend;
        MasterBus master(&backend);
        TrackLibrary lib;
        AudioEngine engine(&backend, &master, &lib);
        PresetManager presets(&engine, &master);
        SessionManager session(&lib, &presets, sessionPath);

        QVERIFY(lib.addFile(solo));
        QCOMPARE(lib.count(), 1);
        QVERIFY(session.save());
    }

    {
        MockAudioBackend backend;
        MasterBus master(&backend);
        TrackLibrary lib;
        AudioEngine engine(&backend, &master, &lib);
        PresetManager presets(&engine, &master);
        SessionManager session(&lib, &presets, sessionPath);

        QVERIFY(session.load());
        QCOMPARE(lib.count(), 1);
        QCOMPARE(lib.trackPaths().first(), solo);
    }
}

void TstSession::should_returnFalse_when_noSessionFile()
{
    MockAudioBackend backend;
    MasterBus master(&backend);
    TrackLibrary lib;
    AudioEngine engine(&backend, &master, &lib);
    PresetManager presets(&engine, &master);
    const QString missing = m_dir.filePath(QStringLiteral("does-not-exist.json"));
    SessionManager session(&lib, &presets, missing);

    QSignalSpy spy(&session, &SessionManager::errorOccurred);
    QCOMPARE(session.load(), false);
    QCOMPARE(spy.count(), 0);
}

void TstSession::should_emitError_when_sessionCorrupt()
{
    const QString path = m_dir.filePath(QStringLiteral("corrupt.json"));
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("{ esto no es json valido ");
        f.close();
    }

    MockAudioBackend backend;
    MasterBus master(&backend);
    TrackLibrary lib;
    AudioEngine engine(&backend, &master, &lib);
    PresetManager presets(&engine, &master);
    SessionManager session(&lib, &presets, path);

    QSignalSpy spy(&session, &SessionManager::errorOccurred);
    QCOMPARE(session.load(), false);
    QCOMPARE(spy.count(), 1);
}

void TstSession::should_writeExpectedTopLevelKeys()
{
    const QString path = m_dir.filePath(QStringLiteral("keys.json"));
    MockAudioBackend backend;
    MasterBus master(&backend);
    TrackLibrary lib;
    AudioEngine engine(&backend, &master, &lib);
    PresetManager presets(&engine, &master);
    SessionManager session(&lib, &presets, path);

    QVERIFY(session.save());

    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    f.close();

    QVERIFY(root.contains(QStringLiteral("trackLibraryPaths")));
    QVERIFY(root.contains(QStringLiteral("activePreset")));
    QVERIFY(root.contains(QStringLiteral("presets")));
    const QJsonArray presetArray = root[QStringLiteral("presets")].toArray();
    QCOMPARE(presetArray.size(), PresetManager::k_PresetCount);

    const QJsonObject scene0 = presetArray.at(0).toObject();
    QVERIFY(scene0.contains(QStringLiteral("channelMasterVolume")));
    QCOMPARE(scene0[QStringLiteral("channels")].toArray().size(),
             AudioEngine::k_DefaultChannelCount);
}

QTEST_MAIN(TstSession)
#include "tst_session.moc"
