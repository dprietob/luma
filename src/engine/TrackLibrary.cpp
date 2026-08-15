// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "TrackLibrary.h"

#include "backend/AudioFileDecoder.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcTrackLibrary, "luma.tracklibrary")

namespace {
const QStringList k_SupportedSuffixes { QStringLiteral("wav"), QStringLiteral("mp3"),
                                        QStringLiteral("ogg"), QStringLiteral("flac") };
}

TrackLibrary::TrackLibrary(QObject* parent)
    : QAbstractListModel(parent)
{}

int TrackLibrary::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return m_tracks.size();
}

QVariant TrackLibrary::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= m_tracks.size()) return {};
    const Track& t = m_tracks.at(index.row());
    switch (role) {
    case FilePathRole: return t.filePath;
    case FileNameRole: return t.fileName;
    case DurationMsRole: return t.durationMs;
    case FormatRole: return t.format;
    default: return {};
    }
}

QHash<int, QByteArray> TrackLibrary::roleNames() const
{
    return {
        { FilePathRole, "filePath" },
        { FileNameRole, "fileName" },
        { DurationMsRole, "durationMs" },
        { FormatRole, "format" },
    };
}

int TrackLibrary::count() const { return m_tracks.size(); }

void TrackLibrary::setTrackDir(const QString& trackDir) { m_trackDir = trackDir; }

void TrackLibrary::clear()
{
    if (m_tracks.isEmpty() && m_libraryPaths.isEmpty()) return;
    beginResetModel();
    m_tracks.clear();
    m_libraryPaths.clear();
    endResetModel();
    emit countChanged();
}

QString TrackLibrary::importIntoTrackDir(const QString& filePath)
{
    const QFileInfo source(filePath);
    QDir dir(m_trackDir);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        qCWarning(lcTrackLibrary) << "could not create track dir:" << m_trackDir;
        emit errorOccurred(tr("Could not create the tracklist directory: %1").arg(m_trackDir));
        return {};
    }

    const QString base = source.completeBaseName();
    const QString suffix = source.suffix();
    QString dest = dir.absoluteFilePath(source.fileName());
    int n = 1;
    while (QFileInfo::exists(dest) &&
           QFileInfo(dest).absoluteFilePath() != source.absoluteFilePath()) {
        dest = dir.absoluteFilePath(QStringLiteral("%1-%2.%3").arg(base).arg(n++).arg(suffix));
    }

    if (QFileInfo(dest).absoluteFilePath() == source.absoluteFilePath()) return dest;
    if (!QFile::copy(source.absoluteFilePath(), dest)) {
        qCWarning(lcTrackLibrary) << "could not copy track to:" << dest;
        emit errorOccurred(
            tr("Could not copy the track into the project: %1").arg(source.fileName()));
        return {};
    }
    return dest;
}

QStringList TrackLibrary::libraryPaths() const { return m_libraryPaths; }

QStringList TrackLibrary::trackPaths() const
{
    QStringList paths;
    paths.reserve(m_tracks.size());
    for (const Track& t : m_tracks) paths.append(t.filePath);
    return paths;
}

void TrackLibrary::scanDirectory(const QString& directoryPath)
{
    QDir dir(directoryPath);
    if (!dir.exists()) {
        qCWarning(lcTrackLibrary) << "directory not found:" << directoryPath;
        emit errorOccurred(tr("Directory not found: %1").arg(directoryPath));
        return;
    }
    if (!m_libraryPaths.contains(directoryPath)) {
        m_libraryPaths.append(directoryPath);
    }

    const QFileInfoList entries = dir.entryInfoList(QDir::Files, QDir::Name);
    for (const QFileInfo& info : entries) {
        addFile(info.absoluteFilePath());
    }
}

bool TrackLibrary::addFile(const QString& filePath)
{
    QFileInfo info(filePath);
    if (!info.exists() || !isSupportedFormat(info.suffix())) return false;

    if (!m_trackDir.isEmpty()) {
        const QString base = QDir(m_trackDir).absolutePath() + QLatin1Char('/');
        if (!info.absoluteFilePath().startsWith(base)) {
            const QString copied = importIntoTrackDir(info.absoluteFilePath());
            if (copied.isEmpty()) return false;
            info = QFileInfo(copied);
        }
    }

    if (indexOfPath(info.absoluteFilePath()) != -1) return false;

    Track t;
    t.filePath = info.absoluteFilePath();
    t.fileName = info.fileName();
    t.format = info.suffix().toUpper();
    t.durationMs = probeDurationMs(t.filePath);

    const int row = sortedInsertIndex(t.fileName);
    beginInsertRows({}, row, row);
    m_tracks.insert(row, std::move(t));
    endInsertRows();
    emit countChanged();
    return true;
}

bool TrackLibrary::addFileFromUrl(const QUrl& url)
{
    if (!url.isLocalFile()) {
        qCWarning(lcTrackLibrary) << "not a local file url:" << url;
        emit errorOccurred(tr("Only local files are supported: %1").arg(url.toString()));
        return false;
    }
    return addFile(url.toLocalFile());
}

bool TrackLibrary::removeTrack(const QString& filePath)
{
    const int row = indexOfPath(filePath);
    if (row == -1) return false;

    beginRemoveRows({}, row, row);
    m_tracks.remove(row);
    endRemoveRows();
    emit countChanged();

    deleteFromTrackDir(filePath);
    return true;
}

void TrackLibrary::deleteFromTrackDir(const QString& filePath)
{
    if (m_trackDir.isEmpty()) return;
    const QString base = QDir(m_trackDir).absolutePath() + QLatin1Char('/');
    const QString abs = QFileInfo(filePath).absoluteFilePath();
    if (!abs.startsWith(base)) return;
    if (QFileInfo::exists(abs) && !QFile::remove(abs)) {
        qCWarning(lcTrackLibrary) << "could not delete track file:" << abs;
        emit errorOccurred(tr("Could not delete the track file: %1").arg(abs));
    }
}

bool TrackLibrary::isSupportedFormat(const QString& suffix)
{
    return k_SupportedSuffixes.contains(suffix.toLower());
}

int TrackLibrary::probeDurationMs(const QString& filePath)
{
    return AudioFileDecoder::durationMs(filePath);
}

int TrackLibrary::indexOfPath(const QString& filePath) const
{
    for (int i = 0; i < m_tracks.size(); ++i) {
        if (m_tracks.at(i).filePath == filePath) return i;
    }
    return -1;
}

int TrackLibrary::sortedInsertIndex(const QString& fileName) const
{
    int row = 0;
    while (row < m_tracks.size() &&
           QString::compare(m_tracks.at(row).fileName, fileName, Qt::CaseInsensitive) < 0) {
        ++row;
    }
    return row;
}
