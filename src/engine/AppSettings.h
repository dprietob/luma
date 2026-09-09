// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariantList>

class AppSettings final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int windowWidth READ windowWidth WRITE setWindowWidth NOTIFY windowWidthChanged FINAL)
    Q_PROPERTY(
        int windowHeight READ windowHeight WRITE setWindowHeight NOTIFY windowHeightChanged FINAL)
    Q_PROPERTY(bool windowMaximized READ windowMaximized WRITE setWindowMaximized NOTIFY
                   windowMaximizedChanged FINAL)
    Q_PROPERTY(int audioCacheMB READ audioCacheMB WRITE setAudioCacheMB NOTIFY audioCacheMBChanged
                   FINAL)
    Q_PROPERTY(bool openLastOnStartup READ openLastOnStartup WRITE setOpenLastOnStartup NOTIFY
                   openLastOnStartupChanged FINAL)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged FINAL)
    Q_PROPERTY(int shortcutsRevision READ shortcutsRevision NOTIFY shortcutsChanged FINAL)
    Q_PROPERTY(bool capturing READ capturing WRITE setCapturing NOTIFY capturingChanged FINAL)

public:
    explicit AppSettings(QObject* parent = nullptr);

    [[nodiscard]] int windowWidth() const;
    [[nodiscard]] int windowHeight() const;
    [[nodiscard]] bool windowMaximized() const;
    [[nodiscard]] int audioCacheMB() const;
    [[nodiscard]] bool openLastOnStartup() const;
    [[nodiscard]] QString language() const;
    [[nodiscard]] Q_INVOKABLE QVariantList availableLanguages() const;

    void setWindowWidth(int value);
    void setWindowHeight(int value);
    void setWindowMaximized(bool value);
    void setAudioCacheMB(int value);
    void setOpenLastOnStartup(bool value);
    void setLanguage(const QString& value);

    [[nodiscard]] QString lastProject() const;
    void setLastProject(const QString& path);

    [[nodiscard]] QStringList recentProjects() const;
    void setRecentProjects(const QStringList& paths);

    [[nodiscard]] int shortcutsRevision() const;
    [[nodiscard]] Q_INVOKABLE QString shortcut(const QString& id) const;
    [[nodiscard]] Q_INVOKABLE QString defaultShortcut(const QString& id) const;
    Q_INVOKABLE void setShortcut(const QString& id, const QString& sequence);
    Q_INVOKABLE void resetShortcut(const QString& id);
    [[nodiscard]] Q_INVOKABLE QVariantList shortcutList() const;
    [[nodiscard]] Q_INVOKABLE QString sequenceFromEvent(int key, int modifiers) const;

    [[nodiscard]] bool capturing() const;
    void setCapturing(bool value);

signals:
    void windowWidthChanged();
    void windowHeightChanged();
    void windowMaximizedChanged();
    void audioCacheMBChanged();
    void openLastOnStartupChanged();
    void languageChanged();
    void shortcutsChanged();
    void capturingChanged();

private:
    QSettings m_settings;
    int m_shortcutsRevision { 0 };
    bool m_capturing { false };
};
