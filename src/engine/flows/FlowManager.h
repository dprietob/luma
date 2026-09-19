// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

class AudioEngine;
class PresetManager;
class IAudioBackend;
class FlowPlayer;

class FlowManager final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList flows READ flows NOTIFY flowsChanged FINAL)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged FINAL)
    Q_PROPERTY(QString runningFlowId READ runningFlowId NOTIFY runningChanged FINAL)
    Q_PROPERTY(QVariantList activeChannels READ activeChannels NOTIFY activeChannelsChanged FINAL)

public:
    FlowManager(AudioEngine* engine, PresetManager* presets, IAudioBackend* backend,
                QObject* parent = nullptr);

    [[nodiscard]] QVariantList flows() const;
    [[nodiscard]] bool running() const;
    [[nodiscard]] QString runningFlowId() const;
    [[nodiscard]] QVariantList activeChannels() const;

    Q_INVOKABLE QString createFlow();
    Q_INVOKABLE void removeFlow(const QString& id);
    Q_INVOKABLE void renameFlow(const QString& id, const QString& name);

    [[nodiscard]] Q_INVOKABLE QVariantMap flowData(const QString& id) const;
    Q_INVOKABLE void updateFlow(const QString& id, const QVariantMap& data);

    Q_INVOKABLE void runFlow(const QString& id);
    Q_INVOKABLE void stopFlow();

    // --- Persistencia (usado por SessionManager) ---
    [[nodiscard]] QJsonArray toJson() const;
    void applyJson(const QJsonArray& flows);

signals:
    void flowsChanged();
    void runningChanged();
    void activeChannelsChanged();

private:
    [[nodiscard]] int indexOf(const QString& id) const;
    [[nodiscard]] static QJsonObject makeFlow(const QString& id, const QString& name);

    void onPlayerRunningChanged();
    void onPanic();

    FlowPlayer* m_player { nullptr };
    QVector<QJsonObject> m_flows;
    int m_nextId { 1 };
    QString m_runningFlowId;
    QVariantList m_activeChannels;
};
