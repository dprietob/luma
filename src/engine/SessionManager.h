// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

#include <QObject>
#include <QString>

class AudioDeviceManager;
class FlowManager;
class PresetManager;
class TrackLibrary;
class QJsonObject;

class SessionManager final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(qreal loadProgress READ loadProgress NOTIFY loadProgressChanged FINAL)

public:
    explicit SessionManager(TrackLibrary* trackLibrary, PresetManager* presetManager,
                            QString sessionFilePath = {}, QObject* parent = nullptr);
    ~SessionManager() override = default;

    [[nodiscard]] qreal loadProgress() const;

    void setAudioDeviceManager(AudioDeviceManager* audioDevices);
    void setFlowManager(FlowManager* flowManager);

    void setFilePath(const QString& filePath);
    void setProjectDir(const QString& projectDir);

    Q_INVOKABLE bool save();

    Q_INVOKABLE bool load();

    [[nodiscard]] QString sessionFilePath() const;

signals:
    void errorOccurred(const QString& message);
    void loadProgressChanged();

private:
    [[nodiscard]] QJsonObject toJson() const;
    void applyJson(const QJsonObject& root);

    void setLoadProgress(qreal progress);

    void relativizeRoot(QJsonObject& root) const;
    void absolutizeRoot(QJsonObject& root) const;

    [[nodiscard]] static QString defaultSessionFilePath();

    TrackLibrary* m_trackLibrary;
    PresetManager* m_presetManager;
    AudioDeviceManager* m_audioDevices { nullptr };
    FlowManager* m_flowManager { nullptr };
    QString m_filePath;
    QString m_projectDir;
    qreal m_loadProgress { 0.0 };
    bool m_loadingProject { false };
};
