// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>

#include <functional>

class AudioEngine;
class PresetManager;
class IAudioBackend;
class QTimer;
class QVariantAnimation;

class FlowPlayer final : public QObject
{
    Q_OBJECT

public:
    FlowPlayer(AudioEngine* engine, PresetManager* presets, IAudioBackend* backend,
               QObject* parent = nullptr);

    [[nodiscard]] bool running() const;
    [[nodiscard]] QList<int> affectedChannels() const;

    void start(const QJsonObject& flow);
    void stop();
    void stopSilent();

signals:
    void runningChanged();

private slots:
    void onVoiceFinished(int voiceId);

private:
    struct Edge
    {
        QString from;
        QString to;
        QString triggerType;
        int triggerSeconds { 0 };
        QString originAction;
        QString targetAction;
    };

    struct NodeRt
    {
        int channelId { -1 };
        QJsonObject config;
    };

    void executeEdge(const Edge& edge);
    void applyOriginAction(const QString& nodeId, const QString& action);
    void applyTargetAction(const QString& nodeId, const QString& action);
    void nodeStarted(const QString& nodeId);
    void nodeFinished(const QString& nodeId, bool propagateFinishEdges);
    void fireEdgesFrom(const QString& nodeId, bool finishPhase);
    void scheduleAction(const Edge& edge, int delayMs);
    void applyConfig(int channelId, const QJsonObject& config);
    void startFade(int channelId, float from, float to, int seconds, int mode,
                   std::function<void()> onDone);
    void checkCompletion();
    void teardown(bool restoreTransport);
    void finishNaturally();

    AudioEngine* m_engine;
    PresetManager* m_presets;
    IAudioBackend* m_backend;

    static constexpr int k_MaxEdgeFires = 100000;

    bool m_running { false };
    int m_gen { 0 };
    int m_pending { 0 };
    int m_edgesFired { 0 };

    QHash<QString, NodeRt> m_nodes;
    QHash<QString, QList<Edge>> m_edgesFrom;
    QHash<int, QString> m_channelCurrentNode;
    QList<int> m_affected;
    QHash<int, QJsonObject> m_snapshot;

    QList<QTimer*> m_timers;
    QHash<int, QVariantAnimation*> m_fades;
};
