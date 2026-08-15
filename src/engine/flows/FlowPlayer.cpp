// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "FlowPlayer.h"

#include "AudioChannel.h"
#include "AudioEngine.h"
#include "ChannelEffects.h"
#include "PresetManager.h"
#include "TrackTimeline.h"
#include "interfaces/IAudioBackend.h"

#include <QEasingCurve>
#include <QJsonArray>
#include <QLoggingCategory>
#include <QPointF>
#include <QTimer>
#include <QVariantAnimation>

#include <algorithm>

Q_LOGGING_CATEGORY(lcFlowPlayer, "luma.flows.player")

namespace {
QEasingCurve curveFor(int mode)
{
    switch (mode) {
    case 1: return QEasingCurve(QEasingCurve::InOutQuad);
    case 2: return QEasingCurve(QEasingCurve::InQuad);
    case 3: return QEasingCurve(QEasingCurve::OutQuad);
    case 4: return QEasingCurve(QEasingCurve::InOutCubic);
    case 5: {
        QEasingCurve curve(QEasingCurve::BezierSpline);
        curve.addCubicBezierSegment(QPointF(0.68, -0.55), QPointF(0.27, 1.55), QPointF(1.0, 1.0));
        return curve;
    }
    default: return QEasingCurve(QEasingCurve::Linear);
    }
}
}

FlowPlayer::FlowPlayer(AudioEngine* engine, PresetManager* presets, IAudioBackend* backend,
                       QObject* parent)
    : QObject(parent)
    , m_engine(engine)
    , m_presets(presets)
    , m_backend(backend)
{
    Q_ASSERT_X(m_engine && m_presets && m_backend, "FlowPlayer", "dependencies must be injected");
    connect(m_backend, &IAudioBackend::voiceFinished, this, &FlowPlayer::onVoiceFinished, Qt::QueuedConnection);
}

bool FlowPlayer::running() const { return m_running; }
QList<int> FlowPlayer::affectedChannels() const { return m_affected; }

void FlowPlayer::start(const QJsonObject& flow)
{
    if (m_running) teardown(false);

    m_presets->selectPreset(flow.value(QStringLiteral("preset")).toInt(0));

    m_nodes.clear();
    m_edgesFrom.clear();
    m_channelCurrentNode.clear();
    m_affected.clear();
    m_snapshot.clear();

    const QJsonArray nodes = flow.value(QStringLiteral("nodes")).toArray();
    for (const QJsonValue& v : nodes) {
        const QJsonObject n = v.toObject();
        NodeRt rt;
        rt.channelId = n.value(QStringLiteral("channelId")).toInt(-1);
        rt.config = n.value(QStringLiteral("config")).toObject();
        m_nodes.insert(n.value(QStringLiteral("id")).toString(), rt);
        if (rt.channelId >= 0 && !m_affected.contains(rt.channelId))
            m_affected.append(rt.channelId);
    }

    const QJsonArray edges = flow.value(QStringLiteral("edges")).toArray();
    for (const QJsonValue& v : edges) {
        const QJsonObject e = v.toObject();
        Edge edge;
        edge.from = e.value(QStringLiteral("from")).toString();
        edge.to = e.value(QStringLiteral("to")).toString();
        const QJsonObject trigger = e.value(QStringLiteral("trigger")).toObject();
        edge.triggerType =
            trigger.value(QStringLiteral("type")).toString(QStringLiteral("immediate"));
        edge.triggerSeconds = trigger.value(QStringLiteral("seconds")).toInt(0);
        edge.originAction =
            e.value(QStringLiteral("originAction")).toString(QStringLiteral("none"));
        edge.targetAction =
            e.value(QStringLiteral("targetAction")).toString(QStringLiteral("play"));
        m_edgesFrom[edge.from].append(edge);
    }

    for (const int channelId : std::as_const(m_affected)) {
        if (AudioChannel* ch = m_engine->channelAt(channelId))
            m_snapshot.insert(channelId, ch->captureState());
    }

    m_running = true;
    ++m_gen;
    m_edgesFired = 0;
    qCInfo(lcFlowPlayer) << "flow started; affected channels:" << m_affected;
    emit runningChanged();

    fireEdgesFrom(QStringLiteral("start"), false);
    checkCompletion();
}

void FlowPlayer::stop()
{
    if (!m_running) return;
    m_running = false;
    teardown(true);
    emit runningChanged();
}

void FlowPlayer::stopSilent()
{
    if (!m_running) return;
    m_running = false;
    teardown(false);
    emit runningChanged();
}

void FlowPlayer::finishNaturally()
{
    if (!m_running) return;
    m_running = false;
    teardown(true);
    qCInfo(lcFlowPlayer) << "flow completed";
    emit runningChanged();
}

void FlowPlayer::fireEdgesFrom(const QString& nodeId, bool finishPhase)
{
    const QList<Edge> edges = m_edgesFrom.value(nodeId);
    for (const Edge& edge : edges) {
        if (finishPhase) {
            if (edge.triggerType == QLatin1String("finish")) scheduleAction(edge, 0);
        } else if (edge.triggerType == QLatin1String("immediate")) {
            scheduleAction(edge, 0);
        } else if (edge.triggerType == QLatin1String("elapsed")) {
            scheduleAction(edge, std::max(0, edge.triggerSeconds) * 1000);
        }
    }
}

void FlowPlayer::scheduleAction(const Edge& edge, int delayMs)
{
    auto* timer = new QTimer(this);
    timer->setSingleShot(true);
    const int gen = m_gen;
    ++m_pending;
    connect(timer, &QTimer::timeout, this, [this, edge, gen, timer]() {
        m_timers.removeOne(timer);
        timer->deleteLater();
        --m_pending;
        if (gen != m_gen) return;
        if (++m_edgesFired > k_MaxEdgeFires) {
            qCWarning(lcFlowPlayer)
                << "flow exceeded" << k_MaxEdgeFires << "actions; stopping (possible cycle)";
            stop();
            return;
        }
        executeEdge(edge);
        checkCompletion();
    });
    m_timers.append(timer);
    timer->start(delayMs);
}

void FlowPlayer::executeEdge(const Edge& edge)
{
    applyTargetAction(edge.to, edge.targetAction);
    if (edge.from != QLatin1String("start")) applyOriginAction(edge.from, edge.originAction);
}

void FlowPlayer::applyOriginAction(const QString& nodeId, const QString& action)
{
    if (action == QLatin1String("none")) return;
    if (!m_nodes.contains(nodeId)) return;
    const NodeRt node = m_nodes.value(nodeId);
    AudioChannel* ch = m_engine->channelAt(node.channelId);

    if (action == QLatin1String("stop")) {
        if (ch) ch->stop();
        nodeFinished(nodeId, false);
        return;
    }

    if (!ch || !ch->hasTrack()) {
        nodeFinished(nodeId, false);
        return;
    }
    const float target =
        static_cast<float>(node.config.value(QStringLiteral("fadeMin")).toDouble(0.0));
    const int seconds = node.config.value(QStringLiteral("fadeSeconds")).toInt(2);
    const int mode = node.config.value(QStringLiteral("fadeMode")).toInt(0);
    const int channelId = node.channelId;
    startFade(channelId, ch->volume(), target, seconds, mode, [this, channelId, nodeId]() {
        if (AudioChannel* c = m_engine->channelAt(channelId)) c->pause();
        nodeFinished(nodeId, false);
    });
}

void FlowPlayer::applyTargetAction(const QString& nodeId, const QString& action)
{
    if (!m_nodes.contains(nodeId)) return;
    const NodeRt node = m_nodes.value(nodeId);
    AudioChannel* ch = m_engine->channelAt(node.channelId);
    if (ch && ch->hasTrack()) {
        applyConfig(node.channelId, node.config);
        ch->play();
        m_channelCurrentNode.insert(node.channelId, nodeId);
        if (action == QLatin1String("fadeIn")) {
            const float initial = static_cast<float>(
                node.config.value(QStringLiteral("initialVolume")).toDouble(0.8));
            const float target =
                static_cast<float>(node.config.value(QStringLiteral("fadeMax")).toDouble(1.0));
            const int seconds = node.config.value(QStringLiteral("fadeSeconds")).toInt(2);
            const int mode = node.config.value(QStringLiteral("fadeMode")).toInt(0);
            startFade(node.channelId, initial, target, seconds, mode, nullptr);
        }
    }
    nodeStarted(nodeId);
}

void FlowPlayer::nodeStarted(const QString& nodeId) { fireEdgesFrom(nodeId, false); }

void FlowPlayer::nodeFinished(const QString& nodeId, bool propagateFinishEdges)
{
    if (m_nodes.contains(nodeId)) {
        const int channelId = m_nodes.value(nodeId).channelId;
        if (m_channelCurrentNode.value(channelId) == nodeId) m_channelCurrentNode.remove(channelId);
    }
    if (propagateFinishEdges) fireEdgesFrom(nodeId, true);
    checkCompletion();
}

void FlowPlayer::applyConfig(int channelId, const QJsonObject& config)
{
    AudioChannel* ch = m_engine->channelAt(channelId);
    if (!ch) return;
    ch->setPan(static_cast<float>(config.value(QStringLiteral("pan")).toDouble(0.0)));
    ch->setLoop(config.value(QStringLiteral("loop")).toBool(false));
    ch->setFadeSeconds(config.value(QStringLiteral("fadeSeconds")).toInt(2));
    ch->setFadeMode(config.value(QStringLiteral("fadeMode")).toInt(0));
    ch->setFadeMax(static_cast<float>(config.value(QStringLiteral("fadeMax")).toDouble(1.0)));
    ch->setFadeMin(static_cast<float>(config.value(QStringLiteral("fadeMin")).toDouble(0.0)));
    ch->effects()->applyJson(config.value(QStringLiteral("effects")).toObject());
    ch->timeline()->applyJson(config.value(QStringLiteral("region")).toObject());
    ch->setVolume(static_cast<float>(config.value(QStringLiteral("initialVolume")).toDouble(0.8)));
    ch->setPlayhead(config.value(QStringLiteral("startSeconds")).toDouble(0.0));
}

void FlowPlayer::startFade(int channelId, float from, float to, int seconds, int mode,
                           std::function<void()> onDone)
{
    if (QVariantAnimation* old = m_fades.value(channelId, nullptr)) {
        old->stop();
        old->deleteLater();
        m_fades.remove(channelId);
    }

    auto* anim = new QVariantAnimation(this);
    anim->setStartValue(from);
    anim->setEndValue(to);
    anim->setDuration(std::max(1, seconds * 1000));
    anim->setEasingCurve(curveFor(mode));
    const int gen = m_gen;
    connect(anim, &QVariantAnimation::valueChanged, this,
            [this, channelId, gen](const QVariant& v) {
                if (gen != m_gen) return;
                if (AudioChannel* ch = m_engine->channelAt(channelId)) ch->setVolume(v.toFloat());
            });
    connect(anim, &QVariantAnimation::finished, this, [this, channelId, gen, anim, onDone]() {
        m_fades.remove(channelId);
        anim->deleteLater();
        if (gen != m_gen) return;
        if (onDone) onDone();
        checkCompletion();
    });
    m_fades.insert(channelId, anim);
    anim->start();
}

void FlowPlayer::onVoiceFinished(int voiceId)
{
    if (!m_running) return;
    for (const int channelId : std::as_const(m_affected)) {
        AudioChannel* ch = m_engine->channelAt(channelId);
        if (ch && ch->voiceId() == voiceId) {
            const QString node = m_channelCurrentNode.value(channelId);
            if (!node.isEmpty())
                nodeFinished(node, true);
            else
                checkCompletion();
            break;
        }
    }
}

void FlowPlayer::checkCompletion()
{
    if (!m_running) return;
    if (m_pending > 0) return;
    for (QVariantAnimation* anim : std::as_const(m_fades))
        if (anim && anim->state() == QAbstractAnimation::Running) return;
    for (const int channelId : std::as_const(m_affected)) {
        const AudioChannel* ch = m_engine->channelAt(channelId);
        if (ch && ch->isPlaying()) return;
    }
    finishNaturally();
}

void FlowPlayer::teardown(bool restoreTransport)
{
    ++m_gen;

    for (QTimer* timer : std::as_const(m_timers)) {
        timer->stop();
        timer->deleteLater();
    }
    m_timers.clear();
    m_pending = 0;

    for (QVariantAnimation* anim : std::as_const(m_fades)) {
        if (anim) {
            anim->stop();
            anim->deleteLater();
        }
    }
    m_fades.clear();

    for (const int channelId : std::as_const(m_affected)) {
        AudioChannel* ch = m_engine->channelAt(channelId);
        if (ch && m_snapshot.contains(channelId))
            ch->restoreState(m_snapshot.value(channelId), restoreTransport);
    }

    m_channelCurrentNode.clear();
    m_nodes.clear();
    m_edgesFrom.clear();
    m_affected.clear();
    m_snapshot.clear();
}
