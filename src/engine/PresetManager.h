// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

#include <QJsonObject>
#include <QObject>
#include <QVector>

class AudioEngine;
class MasterBus;

class PresetManager final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int count READ count CONSTANT FINAL)
    Q_PROPERTY(int activePreset READ activePreset NOTIFY activePresetChanged FINAL)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged FINAL)
    Q_PROPERTY(qreal loadProgress READ loadProgress NOTIFY loadProgressChanged FINAL)
    Q_PROPERTY(int bindingsRevision READ bindingsRevision NOTIFY bindingsChanged FINAL)

public:
    static constexpr int k_PresetCount = 10;

    explicit PresetManager(AudioEngine* engine, MasterBus* masterBus, QObject* parent = nullptr);
    ~PresetManager() override = default;

    [[nodiscard]] int count() const;
    [[nodiscard]] int activePreset() const;
    [[nodiscard]] int bindingsRevision() const;
    [[nodiscard]] bool loading() const;
    [[nodiscard]] qreal loadProgress() const;

    Q_INVOKABLE void selectPreset(int index);

    [[nodiscard]] Q_INVOKABLE QString bindingsFor(const QString& filePath) const;

    Q_INVOKABLE void unbindTrackEverywhere(const QString& filePath);

    // --- Persistencia (usado por SessionManager) ---

    [[nodiscard]] QJsonObject toJson() const;

    void applyJson(const QJsonObject& root);

signals:
    void activePresetChanged();
    void bindingsChanged();
    void loadingChanged();
    void loadProgressChanged();

private:
    [[nodiscard]] QJsonObject captureScene() const;

    void applyScene(const QJsonObject& scene);

    void bumpBindings();

    void setLoading(bool loading);
    void setLoadProgress(qreal progress);

    void onChannelCountChanged();
    void wireChannelBindings();

    AudioEngine* m_engine;
    MasterBus* m_masterBus;

    QVector<QJsonObject> m_scenes;
    int m_activeIndex { 0 };
    int m_bindingsRevision { 0 };
    bool m_loading { false };
    qreal m_loadProgress { 0.0 };
};
