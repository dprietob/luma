// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>

class TrackLibrary final : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)

public:
    enum Roles {
        FilePathRole = Qt::UserRole + 1,
        FileNameRole,
        DurationMsRole,
        FormatRole,
    };
    Q_ENUM(Roles)

    explicit TrackLibrary(QObject* parent = nullptr);
    ~TrackLibrary() override = default;

    // --- QAbstractListModel ---
    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] int count() const;

    void setTrackDir(const QString& trackDir);

    void clear();

    Q_INVOKABLE void scanDirectory(const QString& directoryPath);

    Q_INVOKABLE bool addFile(const QString& filePath);

    Q_INVOKABLE bool addFileFromUrl(const QUrl& url);

    Q_INVOKABLE bool removeTrack(const QString& filePath);

    [[nodiscard]] QStringList libraryPaths() const;

    [[nodiscard]] QStringList trackPaths() const;

signals:
    void countChanged();
    void errorOccurred(const QString& message);

private:
    struct Track
    {
        QString filePath;
        QString fileName;
        int durationMs { 0 };
        QString format;
    };

    [[nodiscard]] static bool isSupportedFormat(const QString& suffix);
    [[nodiscard]] static int probeDurationMs(const QString& filePath);
    [[nodiscard]] int indexOfPath(const QString& filePath) const;
    [[nodiscard]] int sortedInsertIndex(const QString& fileName) const;
    [[nodiscard]] QString importIntoTrackDir(const QString& filePath);
    void deleteFromTrackDir(const QString& filePath);

    QVector<Track> m_tracks;
    QStringList m_libraryPaths;
    QString m_trackDir;
};
