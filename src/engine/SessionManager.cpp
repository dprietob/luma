// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "SessionManager.h"

#include "AudioDeviceManager.h"
#include "PresetManager.h"
#include "TrackLibrary.h"
#include "flows/FlowManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLoggingCategory>
#include <QSaveFile>
#include <QStandardPaths>

#include <algorithm>
#include <functional>

Q_LOGGING_CATEGORY(lcSession, "luma.session")

SessionManager::SessionManager(TrackLibrary* trackLibrary, PresetManager* presetManager,
                               QString sessionFilePath, QObject* parent)
    : QObject(parent)
    , m_trackLibrary(trackLibrary)
    , m_presetManager(presetManager)
    , m_filePath(sessionFilePath.isEmpty() ? defaultSessionFilePath() : std::move(sessionFilePath))
{
    Q_ASSERT_X(m_trackLibrary && m_presetManager, "SessionManager",
               "all dependencies must be injected");

    connect(m_presetManager, &PresetManager::loadProgressChanged, this, [this]() {
        if (m_loadingProject) setLoadProgress(0.5 + 0.5 * m_presetManager->loadProgress());
    });
}

qreal SessionManager::loadProgress() const { return m_loadProgress; }

void SessionManager::setLoadProgress(qreal progress)
{
    const qreal clamped = std::clamp(progress, 0.0, 1.0);
    if (qFuzzyCompare(m_loadProgress, clamped)) return;
    m_loadProgress = clamped;
    emit loadProgressChanged();
}

void SessionManager::setAudioDeviceManager(AudioDeviceManager* audioDevices)
{
    m_audioDevices = audioDevices;
}

void SessionManager::setFlowManager(FlowManager* flowManager) { m_flowManager = flowManager; }

void SessionManager::setFilePath(const QString& filePath) { m_filePath = filePath; }

void SessionManager::setProjectDir(const QString& projectDir) { m_projectDir = projectDir; }

QString SessionManager::sessionFilePath() const { return m_filePath; }

namespace {
QString toRelative(const QDir& dir, const QString& path)
{
    if (path.isEmpty()) return path;
    const QString abs = QFileInfo(path).absoluteFilePath();
    const QString base = dir.absolutePath() + QLatin1Char('/');
    return abs.startsWith(base) ? dir.relativeFilePath(abs) : path;
}

QString toAbsolute(const QDir& dir, const QString& path)
{
    if (path.isEmpty() || QFileInfo(path).isAbsolute()) return path;
    return QDir::cleanPath(dir.absoluteFilePath(path));
}

void mapChannelPaths(QJsonObject& root, const std::function<QString(const QString&)>& fn)
{
    QJsonArray tracks = root.value(QStringLiteral("tracks")).toArray();
    for (qsizetype i = 0; i < tracks.size(); ++i) tracks[i] = fn(tracks.at(i).toString());
    root[QStringLiteral("tracks")] = tracks;

    QJsonArray presets = root.value(QStringLiteral("presets")).toArray();
    for (qsizetype p = 0; p < presets.size(); ++p) {
        QJsonObject preset = presets.at(p).toObject();
        QJsonArray channels = preset.value(QStringLiteral("channels")).toArray();
        for (qsizetype c = 0; c < channels.size(); ++c) {
            QJsonObject channel = channels.at(c).toObject();
            if (channel.contains(QStringLiteral("filePath")))
                channel[QStringLiteral("filePath")] =
                    fn(channel.value(QStringLiteral("filePath")).toString());
            channels[c] = channel;
        }
        preset[QStringLiteral("channels")] = channels;
        presets[p] = preset;
    }
    root[QStringLiteral("presets")] = presets;
}
}

void SessionManager::relativizeRoot(QJsonObject& root) const
{
    if (m_projectDir.isEmpty()) return;
    const QDir dir(m_projectDir);
    mapChannelPaths(root, [&dir](const QString& p) { return toRelative(dir, p); });
}

void SessionManager::absolutizeRoot(QJsonObject& root) const
{
    if (m_projectDir.isEmpty()) return;
    const QDir dir(m_projectDir);
    mapChannelPaths(root, [&dir](const QString& p) { return toAbsolute(dir, p); });
}

// --- Serialización ---

QJsonObject SessionManager::toJson() const
{
    QJsonObject root;

    QJsonArray libraryPaths;
    const QStringList paths = m_trackLibrary->libraryPaths();
    for (const QString& p : paths) libraryPaths.append(p);
    root[QStringLiteral("trackLibraryPaths")] = libraryPaths;

    QJsonArray tracks;
    const QStringList trackPaths = m_trackLibrary->trackPaths();
    for (const QString& p : trackPaths) tracks.append(p);
    root[QStringLiteral("tracks")] = tracks;

    const QJsonObject presets = m_presetManager->toJson();
    for (auto it = presets.constBegin(); it != presets.constEnd(); ++it)
        root[it.key()] = it.value();

    if (m_audioDevices) {
        QJsonObject outs;
        outs[QStringLiteral("main")] = m_audioDevices->mainOutput();
        outs[QStringLiteral("aux")] = m_audioDevices->auxOutput();
        root[QStringLiteral("audioOutputs")] = outs;
    }

    if (m_flowManager) root[QStringLiteral("flows")] = m_flowManager->toJson();

    return root;
}

void SessionManager::applyJson(const QJsonObject& root)
{
    m_loadingProject = true;
    setLoadProgress(0.0);

    m_trackLibrary->clear();

    const QJsonArray libraryPaths = root[QStringLiteral("trackLibraryPaths")].toArray();
    for (const QJsonValue& v : libraryPaths) m_trackLibrary->scanDirectory(v.toString());
    const QJsonArray tracks = root[QStringLiteral("tracks")].toArray();
    const int trackTotal = tracks.size();
    int trackDone = 0;
    for (const QJsonValue& v : tracks) {
        m_trackLibrary->addFile(v.toString());
        ++trackDone;
        setLoadProgress(trackTotal > 0 ? 0.5 * trackDone / trackTotal : 0.5);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    if (m_audioDevices) {
        const QJsonObject outs = root.value(QStringLiteral("audioOutputs")).toObject();
        m_audioDevices->applySaved(outs.value(QStringLiteral("main")).toString(),
                                   outs.value(QStringLiteral("aux")).toString());
    }

    if (m_flowManager) m_flowManager->applyJson(root.value(QStringLiteral("flows")).toArray());

    if (root.contains(QStringLiteral("presets"))) {
        m_presetManager->applyJson(root);
    } else {
        QJsonArray presets;
        presets.append(root);
        QJsonObject migrated;
        migrated[QStringLiteral("presets")] = presets;
        migrated[QStringLiteral("activePreset")] = 0;
        m_presetManager->applyJson(migrated);
    }

    setLoadProgress(1.0);
    m_loadingProject = false;
}

// --- E/S en disco ---

bool SessionManager::save()
{
    const QFileInfo info(m_filePath);
    const QDir dir = info.absoluteDir();
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        const QString msg =
            tr("Could not create the configuration directory: %1").arg(dir.absolutePath());
        qCWarning(lcSession).noquote() << msg;
        emit errorOccurred(msg);
        return false;
    }

    QSaveFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        const QString msg = tr("Could not write the session: %1").arg(file.errorString());
        qCWarning(lcSession).noquote() << msg;
        emit errorOccurred(msg);
        return false;
    }

    QJsonObject root = toJson();
    relativizeRoot(root);
    const QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        const QString msg = tr("Could not save the session: %1").arg(file.errorString());
        qCWarning(lcSession).noquote() << msg;
        emit errorOccurred(msg);
        return false;
    }

    qCInfo(lcSession).noquote() << "Session saved to" << m_filePath;
    return true;
}

bool SessionManager::load()
{
    QFile file(m_filePath);
    if (!file.exists()) {
        qCInfo(lcSession).noquote()
            << "No hay sesión previa; se usan los valores por defecto:" << m_filePath;
        return false;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        const QString msg = tr("Could not read the session: %1").arg(file.errorString());
        qCWarning(lcSession).noquote() << msg;
        emit errorOccurred(msg);
        return false;
    }

    QJsonParseError parseError {};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        const QString msg = tr("Corrupt session (%1); ignoring it.").arg(parseError.errorString());
        qCWarning(lcSession).noquote() << msg;
        emit errorOccurred(msg);
        return false;
    }

    QJsonObject root = doc.object();
    absolutizeRoot(root);
    applyJson(root);
    qCInfo(lcSession).noquote() << "Session loaded from" << m_filePath;
    return true;
}

QString SessionManager::defaultSessionFilePath()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    return QDir(base).filePath(QStringLiteral("luma/session.json"));
}
