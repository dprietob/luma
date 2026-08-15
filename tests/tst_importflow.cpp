// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "MockAudioBackend.h"

#include "AudioChannel.h"
#include "AudioEngine.h"
#include "MasterBus.h"
#include "PresetManager.h"
#include "TrackLibrary.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QUrl>
#include <QtTest>

class TstImportFlow : public QObject
{
    Q_OBJECT

private:
    static QString makeTempAudio(QTemporaryFile& holder, const QString& suffix)
    {
        holder.setFileTemplate(QDir::tempPath() + "/lmtest_XXXXXX." + suffix);
        const bool opened = holder.open();
        Q_ASSERT(opened);
        Q_UNUSED(opened)
        holder.write("data");
        holder.flush();
        return holder.fileName();
    }

    static QString writeAudio(const QString& path)
    {
        QFile f(path);
        [[maybe_unused]] const bool ok = f.open(QIODevice::WriteOnly);
        Q_ASSERT(ok);
        f.write("data");
        f.close();
        return path;
    }

private slots:
    void should_addTrack_when_importingSupportedFile();
    void should_addTrack_when_importingFromUrl();
    void should_rejectFile_when_formatUnsupported();
    void should_bindTrackToChannel_and_reportBindingInPreset();
    void should_addTrackToLibrary_when_bindingFromUrlToChannel();
    void should_sortTracksAlphabetically_when_added();
    void should_deleteFile_when_removingTrackInsideTrackDir();
    void should_keepFile_when_removingTrackWithoutTrackDir();
};

void TstImportFlow::should_sortTracksAlphabetically_when_added()
{
    QTemporaryDir src;
    QTemporaryDir proj;
    TrackLibrary lib;
    lib.setTrackDir(QDir(proj.path()).filePath(QStringLiteral("tracklist")));

    QVERIFY(lib.addFile(writeAudio(QDir(src.path()).filePath(QStringLiteral("c.wav")))));
    QVERIFY(lib.addFile(writeAudio(QDir(src.path()).filePath(QStringLiteral("a.wav")))));
    QVERIFY(lib.addFile(writeAudio(QDir(src.path()).filePath(QStringLiteral("b.wav")))));

    const QStringList paths = lib.trackPaths();
    QCOMPARE(paths.size(), 3);
    QCOMPARE(QFileInfo(paths.at(0)).fileName(), QStringLiteral("a.wav"));
    QCOMPARE(QFileInfo(paths.at(1)).fileName(), QStringLiteral("b.wav"));
    QCOMPARE(QFileInfo(paths.at(2)).fileName(), QStringLiteral("c.wav"));
}

void TstImportFlow::should_deleteFile_when_removingTrackInsideTrackDir()
{
    QTemporaryDir src;
    QTemporaryDir proj;
    TrackLibrary lib;
    lib.setTrackDir(QDir(proj.path()).filePath(QStringLiteral("tracklist")));

    QVERIFY(lib.addFile(writeAudio(QDir(src.path()).filePath(QStringLiteral("song.wav")))));
    const QString stored = lib.trackPaths().first();
    QVERIFY(QFileInfo::exists(stored));

    QVERIFY(lib.removeTrack(stored));
    QCOMPARE(lib.count(), 0);
    QVERIFY(!QFileInfo::exists(stored));
}

void TstImportFlow::should_keepFile_when_removingTrackWithoutTrackDir()
{
    QTemporaryDir src;
    TrackLibrary lib;
    const QString path = writeAudio(QDir(src.path()).filePath(QStringLiteral("keep.wav")));

    QVERIFY(lib.addFile(path));
    const QString stored = lib.trackPaths().first();
    QCOMPARE(stored, path);

    QVERIFY(lib.removeTrack(stored));
    QVERIFY(QFileInfo::exists(path));
}

void TstImportFlow::should_addTrack_when_importingSupportedFile()
{
    TrackLibrary lib;
    QTemporaryFile f;
    const QString path = makeTempAudio(f, "wav");
    QSignalSpy spy(&lib, &TrackLibrary::countChanged);

    QVERIFY(lib.addFile(path));
    QCOMPARE(lib.count(), 1);
    QCOMPARE(spy.count(), 1);
    QVERIFY(!lib.addFile(path));
    QCOMPARE(lib.count(), 1);
}

void TstImportFlow::should_addTrack_when_importingFromUrl()
{
    TrackLibrary lib;
    QTemporaryFile f;
    const QString path = makeTempAudio(f, "flac");
    QVERIFY(lib.addFileFromUrl(QUrl::fromLocalFile(path)));
    QCOMPARE(lib.count(), 1);
}

void TstImportFlow::should_rejectFile_when_formatUnsupported()
{
    TrackLibrary lib;
    QTemporaryFile f;
    const QString path = makeTempAudio(f, "txt");
    QVERIFY(!lib.addFile(path));
    QCOMPARE(lib.count(), 0);
}

void TstImportFlow::should_bindTrackToChannel_and_reportBindingInPreset()
{
    MockAudioBackend backend;
    MasterBus master(&backend);
    TrackLibrary lib;
    AudioEngine engine(&backend, &master, &lib);
    PresetManager presets(&engine, &master);

    QTemporaryFile f;
    const QString path = makeTempAudio(f, "ogg");
    QVERIFY(lib.addFile(path));

    QVERIFY(engine.bindTrack(2, path));
    AudioChannel* ch = engine.channelAt(2);
    QVERIFY(ch != nullptr);
    QVERIFY(ch->hasTrack());
    QCOMPARE(ch->filePath(), path);

    QCOMPARE(presets.bindingsFor(path), QStringLiteral("P1·C3"));
    QVERIFY(presets.bindingsFor(QStringLiteral("/no/existe.wav")).isEmpty());
}

void TstImportFlow::should_addTrackToLibrary_when_bindingFromUrlToChannel()
{
    MockAudioBackend backend;
    MasterBus master(&backend);
    TrackLibrary lib;
    AudioEngine engine(&backend, &master, &lib);

    QTemporaryFile f;
    const QString path = makeTempAudio(f, "wav");
    QCOMPARE(lib.count(), 0);

    QVERIFY(engine.bindTrackFromUrl(3, QUrl::fromLocalFile(path)));
    QVERIFY(engine.channelAt(3)->hasTrack());
    QCOMPARE(lib.count(), 1);
    QCOMPARE(lib.trackPaths().first(), path);

    QVERIFY(engine.bindTrackFromUrl(5, QUrl::fromLocalFile(path)));
    QCOMPARE(lib.count(), 1);
}

QTEST_MAIN(TstImportFlow)
#include "tst_importflow.moc"
