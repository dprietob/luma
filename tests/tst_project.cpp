// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "AppSettings.h"
#include "AudioEngine.h"
#include "MasterBus.h"
#include "PresetManager.h"
#include "ProjectManager.h"
#include "SessionManager.h"
#include "TrackLibrary.h"

#include "MockAudioBackend.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>

class TstProject : public QObject
{
    Q_OBJECT

private:
    QString makeWav(const QString& path)
    {
        QFile f(path);
        [[maybe_unused]] const bool ok = f.open(QIODevice::WriteOnly);
        Q_ASSERT(ok);
        f.write("RIFF");
        f.close();
        return path;
    }

private slots:
    void initTestCase() { QStandardPaths::setTestModeEnabled(true); }

    void should_createProjectDirsAndFile();
    void should_copyTrackIntoTracklistWithRelativePath();
    void should_reopenProjectAfterOriginalDeleted();
    void should_listRecentProjects();
};

void TstProject::should_createProjectDirsAndFile()
{
    QTemporaryDir dir;
    MockAudioBackend backend;
    MasterBus master(&backend);
    TrackLibrary lib;
    AudioEngine engine(&backend, &master, &lib);
    PresetManager presets(&engine, &master);
    SessionManager session(&lib, &presets);
    AppSettings settings;
    ProjectManager pm(&session, &lib, &settings);

    QVERIFY(pm.createProject(dir.path(), QStringLiteral("MyProj")));
    QVERIFY(pm.hasProject());
    QCOMPARE(pm.projectName(), QStringLiteral("MyProj"));

    const QString projDir = QDir(dir.path()).filePath(QStringLiteral("MyProj"));
    QVERIFY(QFileInfo::exists(QDir(projDir).filePath(QStringLiteral("MyProj.luma"))));
    QVERIFY(QFileInfo(QDir(projDir).filePath(QStringLiteral("tracklist"))).isDir());
}

void TstProject::should_copyTrackIntoTracklistWithRelativePath()
{
    QTemporaryDir dir;
    QTemporaryDir external;
    const QString src = makeWav(external.filePath(QStringLiteral("song.wav")));

    MockAudioBackend backend;
    MasterBus master(&backend);
    TrackLibrary lib;
    AudioEngine engine(&backend, &master, &lib);
    PresetManager presets(&engine, &master);
    SessionManager session(&lib, &presets);
    AppSettings settings;
    ProjectManager pm(&session, &lib, &settings);

    QVERIFY(pm.createProject(dir.path(), QStringLiteral("Proj")));
    const QString projDir = QDir(dir.path()).filePath(QStringLiteral("Proj"));
    const QString trackDir = QDir(projDir).filePath(QStringLiteral("tracklist"));

    QVERIFY(lib.addFile(src));
    QCOMPARE(lib.count(), 1);
    const QString stored = lib.trackPaths().first();
    QVERIFY(stored.startsWith(trackDir));
    QVERIFY(QFileInfo::exists(stored));

    QVERIFY(pm.save());

    QFile projFile(QDir(projDir).filePath(QStringLiteral("Proj.luma")));
    QVERIFY(projFile.open(QIODevice::ReadOnly));
    const QJsonObject root = QJsonDocument::fromJson(projFile.readAll()).object();
    projFile.close();
    const QJsonArray tracks = root.value(QStringLiteral("tracks")).toArray();
    QCOMPARE(tracks.size(), 1);
    QCOMPARE(tracks.at(0).toString(), QStringLiteral("tracklist/song.wav"));
}

void TstProject::should_reopenProjectAfterOriginalDeleted()
{
    QTemporaryDir dir;
    QTemporaryDir external;
    const QString src = makeWav(external.filePath(QStringLiteral("loop.wav")));
    const QString projDir = QDir(dir.path()).filePath(QStringLiteral("P"));
    const QString projectFile = QDir(projDir).filePath(QStringLiteral("P.luma"));

    {
        MockAudioBackend backend;
        MasterBus master(&backend);
        TrackLibrary lib;
        AudioEngine engine(&backend, &master, &lib);
        PresetManager presets(&engine, &master);
        SessionManager session(&lib, &presets);
        AppSettings settings;
        ProjectManager pm(&session, &lib, &settings);

        QVERIFY(pm.createProject(dir.path(), QStringLiteral("P")));
        QVERIFY(lib.addFile(src));
        QVERIFY(pm.save());
    }

    QVERIFY(QFile::remove(src));

    {
        MockAudioBackend backend;
        MasterBus master(&backend);
        TrackLibrary lib;
        AudioEngine engine(&backend, &master, &lib);
        PresetManager presets(&engine, &master);
        SessionManager session(&lib, &presets);
        AppSettings settings;
        ProjectManager pm(&session, &lib, &settings);

        QVERIFY(pm.openProject(projectFile));
        QCOMPARE(lib.count(), 1);
        const QString restored = lib.trackPaths().first();
        QVERIFY(restored.startsWith(QDir(projDir).filePath(QStringLiteral("tracklist"))));
        QVERIFY(QFileInfo::exists(restored));
    }
}

void TstProject::should_listRecentProjects()
{
    QTemporaryDir dir;
    MockAudioBackend backend;
    MasterBus master(&backend);
    TrackLibrary lib;
    AudioEngine engine(&backend, &master, &lib);
    PresetManager presets(&engine, &master);
    SessionManager session(&lib, &presets);
    AppSettings settings;
    ProjectManager pm(&session, &lib, &settings);

    QVERIFY(pm.createProject(dir.path(), QStringLiteral("Recent1")));
    const QVariantList recent = pm.recentProjects();
    QVERIFY(!recent.isEmpty());
    QCOMPARE(recent.first().toMap().value(QStringLiteral("name")).toString(),
             QStringLiteral("Recent1"));
}

QTEST_MAIN(TstProject)
#include "tst_project.moc"
