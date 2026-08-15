// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "ProjectManager.h"

#include "AppSettings.h"
#include "SessionManager.h"
#include "TrackLibrary.h"

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcProject, "luma.project")

namespace {
constexpr char k_Extension[] = "luma";
constexpr char k_TrackDir[] = "tracklist";
constexpr int k_MaxRecent = 12;
}

ProjectManager::ProjectManager(SessionManager* session, TrackLibrary* trackLibrary,
                               AppSettings* settings, QObject* parent)
    : QObject(parent)
    , m_session(session)
    , m_trackLibrary(trackLibrary)
    , m_settings(settings)
{
    Q_ASSERT_X(m_session && m_trackLibrary && m_settings, "ProjectManager",
               "dependencies must be injected");
    emit recentProjectsChanged();
}

bool ProjectManager::hasProject() const { return !m_projectPath.isEmpty(); }
bool ProjectManager::loading() const { return m_loading; }

void ProjectManager::setLoading(bool loading)
{
    if (m_loading == loading) return;
    m_loading = loading;
    emit loadingChanged();
}
QString ProjectManager::projectName() const { return m_projectName; }
QString ProjectManager::projectPath() const { return m_projectPath; }

QVariantList ProjectManager::recentProjects() const
{
    QVariantList list;
    for (const QString& path : m_settings->recentProjects()) {
        QVariantMap entry;
        entry[QStringLiteral("name")] = QFileInfo(path).completeBaseName();
        entry[QStringLiteral("path")] = path;
        list.append(entry);
    }
    return list;
}

void ProjectManager::applyProject(const QString& projectFile)
{
    const QFileInfo info(projectFile);
    m_projectPath = info.absoluteFilePath();
    m_projectDir = info.absolutePath();
    m_projectName = info.completeBaseName();
    m_session->setFilePath(m_projectPath);
    m_session->setProjectDir(m_projectDir);
    m_trackLibrary->setTrackDir(QDir(m_projectDir).absoluteFilePath(QLatin1String(k_TrackDir)));
}

bool ProjectManager::createProject(const QString& location, const QString& name)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty() || location.isEmpty()) {
        emit errorOccurred(tr("A project name and location are required."));
        return false;
    }

    QDir base(location);
    const QString projectDir = base.absoluteFilePath(trimmed);
    const QString projectFile =
        QDir(projectDir)
            .absoluteFilePath(QStringLiteral("%1.%2").arg(trimmed, QLatin1String(k_Extension)));

    if (QFileInfo::exists(projectFile)) {
        emit errorOccurred(tr("A project already exists at: %1").arg(projectDir));
        return false;
    }
    if (!QDir().mkpath(QDir(projectDir).absoluteFilePath(QLatin1String(k_TrackDir)))) {
        emit errorOccurred(tr("Could not create the project directory: %1").arg(projectDir));
        return false;
    }

    applyProject(projectFile);
    emit projectChanged();
    if (!m_session->save()) return false;

    pushRecent(m_projectPath);
    m_settings->setLastProject(m_projectPath);
    qCInfo(lcProject) << "created project" << m_projectPath;
    return true;
}

bool ProjectManager::createProjectFromUrl(const QUrl& locationUrl, const QString& name)
{
    if (!locationUrl.isLocalFile()) {
        emit errorOccurred(tr("Only local locations are supported."));
        return false;
    }
    return createProject(locationUrl.toLocalFile(), name);
}

bool ProjectManager::openProject(const QString& projectFile)
{
    if (!QFileInfo::exists(projectFile)) {
        setLoading(false);
        emit errorOccurred(tr("Project not found: %1").arg(projectFile));
        removeRecent(QFileInfo(projectFile).absoluteFilePath());
        return false;
    }

    setLoading(true);
    applyProject(projectFile);
    if (!m_session->load()) {
        setLoading(false);
        emit errorOccurred(tr("Could not open the project: %1").arg(projectFile));
        return false;
    }

    emit projectChanged();
    pushRecent(m_projectPath);
    m_settings->setLastProject(m_projectPath);
    qCInfo(lcProject) << "opened project" << m_projectPath;
    setLoading(false);
    return true;
}

bool ProjectManager::openProjectFromUrl(const QUrl& url)
{
    if (!url.isLocalFile()) {
        emit errorOccurred(tr("Only local files are supported."));
        return false;
    }
    return openProject(url.toLocalFile());
}

void ProjectManager::closeProject()
{
    if (!hasProject()) return;
    m_session->save();
    m_projectPath.clear();
    m_projectDir.clear();
    m_projectName.clear();
    m_settings->setLastProject(QString());
    emit projectChanged();
}

bool ProjectManager::save()
{
    if (!hasProject()) return false;
    return m_session->save();
}

bool ProjectManager::openLastProject()
{
    const QString last = m_settings->lastProject();
    if (last.isEmpty() || !QFileInfo::exists(last)) return false;
    return openProject(last);
}

void ProjectManager::removeRecent(const QString& projectFile)
{
    const QString abs = QFileInfo(projectFile).absoluteFilePath();
    QStringList recent = m_settings->recentProjects();
    if (recent.removeAll(abs) > 0) {
        m_settings->setRecentProjects(recent);
        emit recentProjectsChanged();
    }
}

void ProjectManager::pushRecent(const QString& projectFile)
{
    const QString abs = QFileInfo(projectFile).absoluteFilePath();
    QStringList recent = m_settings->recentProjects();
    recent.removeAll(abs);
    recent.prepend(abs);
    while (recent.size() > k_MaxRecent) recent.removeLast();
    m_settings->setRecentProjects(recent);
    emit recentProjectsChanged();
}
