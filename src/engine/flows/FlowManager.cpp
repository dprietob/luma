// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "FlowManager.h"

#include "AudioEngine.h"
#include "FlowPlayer.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcFlows, "luma.flows")

namespace {
constexpr char k_KeyId[] = "id";
constexpr char k_KeyName[] = "name";
constexpr char k_KeyPreset[] = "preset";
constexpr char k_KeyNodes[] = "nodes";
constexpr char k_KeyEdges[] = "edges";
}

FlowManager::FlowManager(AudioEngine* engine, PresetManager* presets, IAudioBackend* backend,
                         QObject* parent)
    : QObject(parent)
    , m_player(new FlowPlayer(engine, presets, backend, this))
{
    connect(m_player, &FlowPlayer::runningChanged, this, &FlowManager::onPlayerRunningChanged);
    connect(engine, &AudioEngine::panicRequested, this, &FlowManager::onPanic);
}

QJsonObject FlowManager::makeFlow(const QString& id, const QString& name)
{
    QJsonObject flow;
    flow[QLatin1String(k_KeyId)] = id;
    flow[QLatin1String(k_KeyName)] = name;
    flow[QLatin1String(k_KeyPreset)] = 0;
    flow[QLatin1String(k_KeyNodes)] = QJsonArray();
    flow[QLatin1String(k_KeyEdges)] = QJsonArray();
    return flow;
}

int FlowManager::indexOf(const QString& id) const
{
    for (int i = 0; i < m_flows.size(); ++i) {
        if (m_flows.at(i).value(QLatin1String(k_KeyId)).toString() == id) return i;
    }
    return -1;
}

QVariantList FlowManager::flows() const
{
    QVariantList list;
    list.reserve(m_flows.size());
    for (const QJsonObject& flow : m_flows) {
        QVariantMap entry;
        entry[QLatin1String(k_KeyId)] = flow.value(QLatin1String(k_KeyId)).toString();
        entry[QLatin1String(k_KeyName)] = flow.value(QLatin1String(k_KeyName)).toString();
        entry[QLatin1String(k_KeyPreset)] = flow.value(QLatin1String(k_KeyPreset)).toInt();
        list.append(entry);
    }
    return list;
}

bool FlowManager::running() const { return m_player->running(); }
QString FlowManager::runningFlowId() const { return m_runningFlowId; }
QVariantList FlowManager::activeChannels() const { return m_activeChannels; }

QString FlowManager::createFlow()
{
    const int n = m_nextId++;
    const QString id = QStringLiteral("flow-%1").arg(n);
    const QString name = QStringLiteral("Flow %1").arg(n);
    m_flows.append(makeFlow(id, name));
    qCInfo(lcFlows) << "created flow" << id;
    emit flowsChanged();
    return id;
}

void FlowManager::removeFlow(const QString& id)
{
    const int index = indexOf(id);
    if (index < 0) return;
    if (id == m_runningFlowId) stopFlow();
    m_flows.removeAt(index);
    qCInfo(lcFlows) << "removed flow" << id;
    emit flowsChanged();
}

void FlowManager::onPlayerRunningChanged()
{
    if (m_player->running()) {
        m_activeChannels.clear();
        const QList<int> affected = m_player->affectedChannels();
        for (const int id : affected) m_activeChannels.append(id);
    } else {
        m_runningFlowId.clear();
        m_activeChannels.clear();
    }
    emit activeChannelsChanged();
    emit runningChanged();
}

void FlowManager::onPanic() { m_player->stopSilent(); }

void FlowManager::renameFlow(const QString& id, const QString& name)
{
    const int index = indexOf(id);
    if (index < 0) return;
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) return;
    if (m_flows.at(index).value(QLatin1String(k_KeyName)).toString() == trimmed) return;
    m_flows[index][QLatin1String(k_KeyName)] = trimmed;
    emit flowsChanged();
}

QVariantMap FlowManager::flowData(const QString& id) const
{
    const int index = indexOf(id);
    if (index < 0) return {};
    return m_flows.at(index).toVariantMap();
}

void FlowManager::updateFlow(const QString& id, const QVariantMap& data)
{
    const int index = indexOf(id);
    if (index < 0) return;
    QJsonObject updated = QJsonObject::fromVariantMap(data);
    updated[QLatin1String(k_KeyId)] = id;
    m_flows[index] = updated;
    emit flowsChanged();
}

void FlowManager::runFlow(const QString& id)
{
    const int index = indexOf(id);
    if (index < 0) return;
    m_runningFlowId = id;
    qCInfo(lcFlows) << "runFlow:" << id;
    m_player->start(m_flows.at(index));
}

void FlowManager::stopFlow() { m_player->stop(); }

QJsonArray FlowManager::toJson() const
{
    QJsonArray array;
    for (const QJsonObject& flow : m_flows) array.append(flow);
    return array;
}

void FlowManager::applyJson(const QJsonArray& flows)
{
    m_flows.clear();
    int maxId = 0;
    for (const QJsonValue& v : flows) {
        if (!v.isObject()) continue;
        const QJsonObject flow = v.toObject();
        const QString id = flow.value(QLatin1String(k_KeyId)).toString();
        if (id.isEmpty()) continue;
        m_flows.append(flow);
        if (id.startsWith(QLatin1String("flow-"))) {
            bool ok = false;
            const int n = QStringView { id }.mid(5).toInt(&ok);
            if (ok && n > maxId) maxId = n;
        }
    }
    m_nextId = maxId + 1;
    if (m_player->running()) m_player->stop();
    emit flowsChanged();
}
