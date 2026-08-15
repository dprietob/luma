// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariantList>

class SessionManager;
class TrackLibrary;
class AppSettings;

class ProjectManager final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool hasProject READ hasProject NOTIFY projectChanged FINAL)
    Q_PROPERTY(QString projectName READ projectName NOTIFY projectChanged FINAL)
    Q_PROPERTY(QString projectPath READ projectPath NOTIFY projectChanged FINAL)
    Q_PROPERTY(QVariantList recentProjects READ recentProjects NOTIFY recentProjectsChanged FINAL)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged FINAL)

public:
    ProjectManager(SessionManager* session, TrackLibrary* trackLibrary, AppSettings* settings,
                   QObject* parent = nullptr);

    [[nodiscard]] bool hasProject() const;
    [[nodiscard]] QString projectName() const;
    [[nodiscard]] QString projectPath() const;
    [[nodiscard]] QVariantList recentProjects() const;
    [[nodiscard]] bool loading() const;

    void setLoading(bool loading);

    Q_INVOKABLE bool createProject(const QString& location, const QString& name);
    Q_INVOKABLE bool createProjectFromUrl(const QUrl& locationUrl, const QString& name);
    Q_INVOKABLE bool openProject(const QString& projectFile);
    Q_INVOKABLE bool openProjectFromUrl(const QUrl& url);
    Q_INVOKABLE void closeProject();
    Q_INVOKABLE bool save();
    Q_INVOKABLE void removeRecent(const QString& projectFile);

    bool openLastProject();

signals:
    void projectChanged();
    void recentProjectsChanged();
    void loadingChanged();
    void errorOccurred(const QString& message);

private:
    void applyProject(const QString& projectFile);
    void pushRecent(const QString& projectFile);

    SessionManager* m_session;
    TrackLibrary* m_trackLibrary;
    AppSettings* m_settings;

    QString m_projectPath;
    QString m_projectDir;
    QString m_projectName;
    bool m_loading { false };
};
