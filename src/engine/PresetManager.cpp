// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "PresetManager.h"

#include "AudioChannel.h"
#include "AudioEngine.h"
#include "ChannelEffects.h"
#include "MasterBus.h"
#include "TrackTimeline.h"

#include <QColor>
#include <QCoreApplication>
#include <QEventLoop>
#include <QHash>
#include <QJsonArray>
#include <QLoggingCategory>

#include <algorithm>

Q_LOGGING_CATEGORY(lcPreset, "luma.preset")

namespace {
constexpr float k_DefaultChannelMasterVolume = 0.85f;
constexpr float k_DefaultChannelVolume = 0.8f;
constexpr int k_DefaultFadeSeconds = 2;
const QColor k_DefaultChannelColor = AudioChannel::defaultColor();

QHash<int, QJsonObject> indexById(const QJsonArray& array)
{
    QHash<int, QJsonObject> byId;
    for (const QJsonValue& v : array) {
        const QJsonObject o = v.toObject();
        byId.insert(o.value(QStringLiteral("id")).toInt(-1), o);
    }
    return byId;
}
}

PresetManager::PresetManager(AudioEngine* engine, MasterBus* masterBus, QObject* parent)
    : QObject(parent)
    , m_engine(engine)
    , m_masterBus(masterBus)
    , m_scenes(k_PresetCount)
{
    Q_ASSERT_X(m_engine && m_masterBus, "PresetManager", "all dependencies must be injected");

    wireChannelBindings();
    connect(m_engine, &AudioEngine::channelCountChanged, this,
            &PresetManager::onChannelCountChanged);
}

void PresetManager::wireChannelBindings()
{
    for (int i = 0; i < m_engine->channelCount(); ++i) {
        if (const AudioChannel* ch = m_engine->channelAt(i))
            connect(ch, &AudioChannel::filePathChanged, this, &PresetManager::bumpBindings,
                    Qt::UniqueConnection);
    }
}

void PresetManager::onChannelCountChanged()
{
    const int n = m_engine->channelCount();
    for (int p = 0; p < k_PresetCount; ++p) {
        QJsonObject scene = m_scenes.at(p);
        if (!scene.contains(QStringLiteral("channels"))) continue;
        const QJsonArray channels = scene.value(QStringLiteral("channels")).toArray();
        QJsonArray kept;
        for (const QJsonValue& v : channels)
            if (v.toObject().value(QStringLiteral("id")).toInt(-1) < n) kept.append(v);
        scene[QStringLiteral("channels")] = kept;
        m_scenes[p] = scene;
    }
    wireChannelBindings();
    bumpBindings();
}

int PresetManager::count() const { return k_PresetCount; }
int PresetManager::activePreset() const { return m_activeIndex; }
int PresetManager::bindingsRevision() const { return m_bindingsRevision; }
bool PresetManager::loading() const { return m_loading; }
qreal PresetManager::loadProgress() const { return m_loadProgress; }

void PresetManager::setLoadProgress(qreal progress)
{
    const qreal clamped = std::clamp(progress, 0.0, 1.0);
    if (qFuzzyCompare(m_loadProgress, clamped)) return;
    m_loadProgress = clamped;
    emit loadProgressChanged();
}

void PresetManager::setLoading(bool loading)
{
    if (m_loading == loading) return;
    m_loading = loading;
    emit loadingChanged();
}

void PresetManager::bumpBindings()
{
    ++m_bindingsRevision;
    emit bindingsChanged();
}

QString PresetManager::bindingsFor(const QString& filePath) const
{
    if (filePath.isEmpty()) return {};

    QStringList tokens;
    for (int p = 0; p < k_PresetCount; ++p) {
        if (p == m_activeIndex) {
            for (int c = 0; c < m_engine->channelCount(); ++c) {
                const AudioChannel* ch = m_engine->channelAt(c);
                if (ch && ch->filePath() == filePath)
                    tokens << QStringLiteral("P%1·C%2").arg(p + 1).arg(c + 1);
            }
        } else {
            const QJsonArray channels = m_scenes.at(p).value(QStringLiteral("channels")).toArray();
            for (const QJsonValue& v : channels) {
                const QJsonObject o = v.toObject();
                if (o.value(QStringLiteral("filePath")).toString() == filePath)
                    tokens << QStringLiteral("P%1·C%2").arg(p + 1).arg(
                        o.value(QStringLiteral("id")).toInt() + 1);
            }
        }
    }
    return tokens.join(QStringLiteral(", "));
}

void PresetManager::unbindTrackEverywhere(const QString& filePath)
{
    if (filePath.isEmpty()) return;

    bool changed = false;

    for (int c = 0; c < m_engine->channelCount(); ++c) {
        AudioChannel* ch = m_engine->channelAt(c);
        if (ch && ch->filePath() == filePath) {
            ch->unbindTrack();
            changed = true;
        }
    }

    for (int p = 0; p < k_PresetCount; ++p) {
        if (p == m_activeIndex) continue;

        QJsonObject scene = m_scenes.at(p);
        const QJsonArray channels = scene.value(QStringLiteral("channels")).toArray();
        QJsonArray updated;
        bool sceneChanged = false;
        for (const QJsonValue& v : channels) {
            QJsonObject o = v.toObject();
            if (o.value(QStringLiteral("filePath")).toString() == filePath) {
                o.remove(QStringLiteral("filePath"));
                sceneChanged = true;
            }
            updated.append(o);
        }
        if (sceneChanged) {
            scene[QStringLiteral("channels")] = updated;
            m_scenes[p] = scene;
            changed = true;
        }
    }

    if (changed) bumpBindings();
}

void PresetManager::selectPreset(int index)
{
    if (index < 0 || index >= k_PresetCount || index == m_activeIndex || m_loading) return;

    m_scenes[m_activeIndex] = captureScene();
    m_engine->stopAll();

    setLoading(true);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    applyScene(m_scenes.at(index));
    setLoading(false);

    m_activeIndex = index;
    qCInfo(lcPreset) << "active preset:" << m_activeIndex;
    emit activePresetChanged();
    bumpBindings();
}

// --- Captura / aplicación de escenas ---

QJsonObject PresetManager::captureScene() const
{
    QJsonObject scene;
    scene[QStringLiteral("channelMasterVolume")] = m_masterBus->channelMasterVolume();

    QJsonArray channels;
    for (int i = 0; i < m_engine->channelCount(); ++i) {
        const AudioChannel* ch = m_engine->channelAt(i);
        if (!ch) continue;
        QJsonObject o;
        o[QStringLiteral("id")] = ch->id();
        o[QStringLiteral("name")] = ch->name();
        if (!ch->filePath().isEmpty()) o[QStringLiteral("filePath")] = ch->filePath();
        o[QStringLiteral("volume")] = ch->volume();
        o[QStringLiteral("pan")] = ch->pan();
        o[QStringLiteral("color")] = ch->color().name(QColor::HexRgb);
        o[QStringLiteral("fadeSeconds")] = ch->fadeSeconds();
        o[QStringLiteral("fadeMode")] = ch->fadeMode();
        o[QStringLiteral("fadeMax")] = ch->fadeMax();
        o[QStringLiteral("fadeMin")] = ch->fadeMin();
        o[QStringLiteral("loop")] = ch->loop();
        o[QStringLiteral("aux")] = ch->aux();
        o[QStringLiteral("effects")] = ch->effects()->toJson();
        o[QStringLiteral("region")] = ch->timeline()->toJson();
        channels.append(o);
    }
    scene[QStringLiteral("channels")] = channels;

    return scene;
}

void PresetManager::applyScene(const QJsonObject& scene)
{
    m_masterBus->setChannelMasterVolume(static_cast<float>(
        scene.value(QStringLiteral("channelMasterVolume")).toDouble(k_DefaultChannelMasterVolume)));

    const QHash<int, QJsonObject> channels =
        indexById(scene.value(QStringLiteral("channels")).toArray());
    const int total = m_engine->channelCount();
    setLoadProgress(0.0);
    for (int id = 0; id < total; ++id) {
        setLoadProgress(total > 0 ? static_cast<qreal>(id) / total : 0.0);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        AudioChannel* ch = m_engine->channelAt(id);
        if (!ch) continue;

        const auto it = channels.constFind(id);
        const QJsonObject o = it == channels.constEnd() ? QJsonObject() : it.value();
        const QString path = o.value(QStringLiteral("filePath")).toString();
        const bool sameTrack = !path.isEmpty() && ch->filePath() == path;

        ch->reset();
        if (!sameTrack) {
            ch->releaseAudio();
            if (!path.isEmpty()) {
                if (!m_engine->bindTrack(id, path))
                    qCWarning(lcPreset) << "channel" << id << "could not load" << path;
            }
        }

        if (it == channels.constEnd()) {
            ch->setName(AudioChannel::defaultName(id));
            ch->setColor(k_DefaultChannelColor);
            continue;
        }

        const QString savedName = o.value(QStringLiteral("name")).toString();
        ch->setName(savedName.isEmpty() ? AudioChannel::defaultName(id) : savedName);
        ch->setColor(
            QColor(o.value(QStringLiteral("color")).toString(k_DefaultChannelColor.name())));
        ch->setVolume(
            static_cast<float>(o.value(QStringLiteral("volume")).toDouble(k_DefaultChannelVolume)));
        ch->setPan(static_cast<float>(o.value(QStringLiteral("pan")).toDouble(0.0)));
        ch->setFadeSeconds(o.value(QStringLiteral("fadeSeconds")).toInt(k_DefaultFadeSeconds));
        ch->setFadeMode(o.value(QStringLiteral("fadeMode")).toInt(0));
        ch->setFadeMax(static_cast<float>(o.value(QStringLiteral("fadeMax")).toDouble(1.0)));
        ch->setFadeMin(static_cast<float>(o.value(QStringLiteral("fadeMin")).toDouble(0.0)));
        ch->setLoop(o.value(QStringLiteral("loop")).toBool(false));
        ch->setAux(o.value(QStringLiteral("aux")).toBool(false));
        ch->effects()->applyJson(o.value(QStringLiteral("effects")).toObject());
        ch->timeline()->applyJson(o.value(QStringLiteral("region")).toObject());
    }
    setLoadProgress(1.0);
}

// --- Persistencia ---

QJsonObject PresetManager::toJson() const
{
    QJsonArray presets;
    for (int i = 0; i < k_PresetCount; ++i) {
        presets.append(i == m_activeIndex ? captureScene() : m_scenes.at(i));
    }
    QJsonObject root;
    root[QStringLiteral("activePreset")] = m_activeIndex;
    root[QStringLiteral("channelCount")] = m_engine->channelCount();
    root[QStringLiteral("presets")] = presets;

    QJsonArray channelOrder;
    for (const QVariant& v : m_engine->channelOrder()) channelOrder.append(v.toInt());
    root[QStringLiteral("channelOrder")] = channelOrder;

    root[QStringLiteral("gridMode")] = m_engine->gridMode();

    return root;
}

void PresetManager::applyJson(const QJsonObject& root)
{
    m_engine->setChannelCount(
        root.value(QStringLiteral("channelCount")).toInt(AudioEngine::k_DefaultChannelCount));

    const QJsonArray presets = root.value(QStringLiteral("presets")).toArray();
    for (int i = 0; i < k_PresetCount; ++i)
        m_scenes[i] = i < presets.size() ? presets.at(i).toObject() : QJsonObject();

    m_activeIndex =
        qBound(0, root.value(QStringLiteral("activePreset")).toInt(0), k_PresetCount - 1);
    applyScene(m_scenes.at(m_activeIndex));

    if (root.contains(QStringLiteral("channelOrder"))) {
        QVariantList order;
        for (const QJsonValue& v : root.value(QStringLiteral("channelOrder")).toArray())
            order.append(v.toInt());
        m_engine->setChannelOrder(order);
    }

    m_engine->setGridMode(root.value(QStringLiteral("gridMode")).toBool(false));

    emit activePresetChanged();
    bumpBindings();
}
