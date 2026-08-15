// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "AppSettings.h"

#include <QCoreApplication>
#include <QKeySequence>
#include <QPair>
#include <QVariantMap>
#include <QVector>

namespace {
struct ShortcutCommand
{
    QString id;
    QString label;
    QString def;
};

QVector<ShortcutCommand> shortcutCommands()
{
    QVector<ShortcutCommand> commands {
        { QStringLiteral("newProject"), QCoreApplication::translate("AppSettings", "New project"),
          QStringLiteral("Ctrl+N") },
        { QStringLiteral("openProject"), QCoreApplication::translate("AppSettings", "Open project"),
          QStringLiteral("Ctrl+O") },
        { QStringLiteral("addTrack"),
          QCoreApplication::translate("AppSettings", "Add track to tracklist"),
          QStringLiteral("Ctrl+T") },
        { QStringLiteral("newFlow"), QCoreApplication::translate("AppSettings", "New flow"),
          QStringLiteral("Ctrl+F") },
        { QStringLiteral("audioOutput"),
          QCoreApplication::translate("AppSettings", "Audio output devices"),
          QStringLiteral("Ctrl+D") },
        { QStringLiteral("channelCount"),
          QCoreApplication::translate("AppSettings", "Channel count"), QStringLiteral("Ctrl+H") },
        { QStringLiteral("reorder"), QCoreApplication::translate("AppSettings", "Reorder channels"),
          QStringLiteral("Ctrl+R") },
        { QStringLiteral("grid"), QCoreApplication::translate("AppSettings", "Grid mode"),
          QStringLiteral("Ctrl+G") },
        { QStringLiteral("masterMin"),
          QCoreApplication::translate("AppSettings", "Master to minimum"),
          QStringLiteral("Ctrl+M") },
        { QStringLiteral("panic"), QCoreApplication::translate("AppSettings", "Panic"),
          QStringLiteral("Ctrl+K") },
        { QStringLiteral("preferences"), QCoreApplication::translate("AppSettings", "Preferences"),
          QStringLiteral("Ctrl+P") },
    };
    for (int i = 1; i <= 10; ++i)
        commands.append({ QStringLiteral("preset%1").arg(i),
                          QCoreApplication::translate("AppSettings", "Select preset %1").arg(i),
                          QStringLiteral("F%1").arg(i) });
    return commands;
}
}

namespace {
constexpr char k_WindowWidth[] = "window/width";
constexpr char k_WindowHeight[] = "window/height";
constexpr char k_WindowMaximized[] = "window/maximized";
constexpr char k_AudioCacheMB[] = "audio/cacheMB";
constexpr char k_OpenLastOnStartup[] = "startup/openLast";
constexpr char k_Language[] = "ui/language";
constexpr char k_LastProject[] = "project/last";
constexpr char k_RecentProjects[] = "project/recent";

constexpr int k_DefaultWidth = 1600;
constexpr int k_DefaultHeight = 900;
constexpr int k_DefaultCacheMB = 512;
}

AppSettings::AppSettings(QObject* parent)
    : QObject(parent)
    , m_settings(QStringLiteral("Luma"), QStringLiteral("Luma"))
{}

int AppSettings::windowWidth() const
{
    return m_settings.value(QLatin1String(k_WindowWidth), k_DefaultWidth).toInt();
}

int AppSettings::windowHeight() const
{
    return m_settings.value(QLatin1String(k_WindowHeight), k_DefaultHeight).toInt();
}

bool AppSettings::windowMaximized() const
{
    return m_settings.value(QLatin1String(k_WindowMaximized), false).toBool();
}

int AppSettings::audioCacheMB() const
{
    return m_settings.value(QLatin1String(k_AudioCacheMB), k_DefaultCacheMB).toInt();
}

bool AppSettings::openLastOnStartup() const
{
    return m_settings.value(QLatin1String(k_OpenLastOnStartup), true).toBool();
}

QString AppSettings::language() const
{
    return m_settings.value(QLatin1String(k_Language), QStringLiteral("system")).toString();
}

QVariantList AppSettings::availableLanguages() const
{
    static const QVector<QPair<QString, QString>> langs {
        { QStringLiteral("system"), QStringLiteral("System") },
        { QStringLiteral("en"), QStringLiteral("English") },
        { QStringLiteral("es"), QStringLiteral("Español") },
        { QStringLiteral("fr"), QStringLiteral("Français") },
        { QStringLiteral("de"), QStringLiteral("Deutsch") },
        { QStringLiteral("it"), QStringLiteral("Italiano") },
        { QStringLiteral("pt"), QStringLiteral("Português") },
        { QStringLiteral("nl"), QStringLiteral("Nederlands") },
    };
    QVariantList list;
    for (const auto& [code, name] : langs) {
        QVariantMap entry;
        entry[QStringLiteral("code")] = code;
        entry[QStringLiteral("name")] = name;
        list.append(entry);
    }
    return list;
}

void AppSettings::setWindowWidth(int value)
{
    if (value <= 0 || value == windowWidth()) return;
    m_settings.setValue(QLatin1String(k_WindowWidth), value);
    emit windowWidthChanged();
}

void AppSettings::setWindowHeight(int value)
{
    if (value <= 0 || value == windowHeight()) return;
    m_settings.setValue(QLatin1String(k_WindowHeight), value);
    emit windowHeightChanged();
}

void AppSettings::setWindowMaximized(bool value)
{
    if (value == windowMaximized()) return;
    m_settings.setValue(QLatin1String(k_WindowMaximized), value);
    emit windowMaximizedChanged();
}

void AppSettings::setAudioCacheMB(int value)
{
    const int clamped = value < 0 ? 0 : value;
    if (clamped == audioCacheMB()) return;
    m_settings.setValue(QLatin1String(k_AudioCacheMB), clamped);
    emit audioCacheMBChanged();
}

void AppSettings::setOpenLastOnStartup(bool value)
{
    if (value == openLastOnStartup()) return;
    m_settings.setValue(QLatin1String(k_OpenLastOnStartup), value);
    emit openLastOnStartupChanged();
}

void AppSettings::setLanguage(const QString& value)
{
    if (value == language()) return;
    m_settings.setValue(QLatin1String(k_Language), value);
    emit languageChanged();
}

QString AppSettings::lastProject() const
{
    return m_settings.value(QLatin1String(k_LastProject)).toString();
}

void AppSettings::setLastProject(const QString& path)
{
    m_settings.setValue(QLatin1String(k_LastProject), path);
}

QStringList AppSettings::recentProjects() const
{
    return m_settings.value(QLatin1String(k_RecentProjects)).toStringList();
}

void AppSettings::setRecentProjects(const QStringList& paths)
{
    m_settings.setValue(QLatin1String(k_RecentProjects), paths);
}

int AppSettings::shortcutsRevision() const { return m_shortcutsRevision; }

QString AppSettings::defaultShortcut(const QString& id) const
{
    for (const ShortcutCommand& cmd : shortcutCommands())
        if (cmd.id == id) return cmd.def;
    return {};
}

QString AppSettings::shortcut(const QString& id) const
{
    return m_settings.value(QStringLiteral("shortcuts/") + id, defaultShortcut(id)).toString();
}

void AppSettings::setShortcut(const QString& id, const QString& sequence)
{
    const QString normalized =
        QKeySequence(sequence, QKeySequence::PortableText).toString(QKeySequence::PortableText);
    m_settings.setValue(QStringLiteral("shortcuts/") + id,
                        normalized.isEmpty() ? sequence.trimmed() : normalized);
    ++m_shortcutsRevision;
    emit shortcutsChanged();
}

void AppSettings::resetShortcut(const QString& id)
{
    m_settings.remove(QStringLiteral("shortcuts/") + id);
    ++m_shortcutsRevision;
    emit shortcutsChanged();
}

QVariantList AppSettings::shortcutList() const
{
    QVariantList list;
    for (const ShortcutCommand& cmd : shortcutCommands()) {
        QVariantMap entry;
        entry[QStringLiteral("id")] = cmd.id;
        entry[QStringLiteral("label")] = cmd.label;
        entry[QStringLiteral("sequence")] = shortcut(cmd.id);
        entry[QStringLiteral("isDefault")] = shortcut(cmd.id) == cmd.def;
        list.append(entry);
    }
    return list;
}

bool AppSettings::capturing() const { return m_capturing; }

void AppSettings::setCapturing(bool value)
{
    if (m_capturing == value) return;
    m_capturing = value;
    emit capturingChanged();
}

QString AppSettings::sequenceFromEvent(int key, int modifiers) const
{
    if (key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt ||
        key == Qt::Key_Meta || key == 0)
        return {};
    return QKeySequence(key | modifiers).toString(QKeySequence::PortableText);
}
