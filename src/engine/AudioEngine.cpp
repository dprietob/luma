// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "AudioEngine.h"

#include "AudioChannel.h"
#include "MasterBus.h"
#include "TrackLibrary.h"
#include "interfaces/IAudioBackend.h"

#include <QLoggingCategory>
#include <QTimer>

#include <algorithm>

Q_LOGGING_CATEGORY(lcAudioEngine, "luma.audio.engine")

AudioEngine::AudioEngine(IAudioBackend* backend, MasterBus* masterBus, TrackLibrary* trackLibrary,
                         QObject* parent)
    : QObject(parent)
    , m_backend(backend)
    , m_masterBus(masterBus)
    , m_trackLibrary(trackLibrary)
{
    Q_ASSERT_X(m_backend && m_masterBus && m_trackLibrary, "AudioEngine",
               "all dependencies must be injected");

    m_channels.reserve(k_MaxChannelCount);
    for (int i = 0; i < k_MaxChannelCount; ++i) {
        auto* channel = new AudioChannel(i, m_backend, this);
        connect(channel, &AudioChannel::errorOccurred, this, &AudioEngine::errorOccurred);
        m_channels.append(channel);
    }
    for (int i = 0; i < m_channelCount; ++i) m_channelOrder.append(i);

    connect(m_backend, &IAudioBackend::vuLevelReady, this, &AudioEngine::onBackendVuLevel,
            Qt::QueuedConnection);
    connect(m_backend, &IAudioBackend::voicePositionReady, this,
            &AudioEngine::onBackendVoicePosition, Qt::QueuedConnection);
    connect(m_backend, &IAudioBackend::voiceFinished, this, &AudioEngine::onBackendVoiceFinished,
            Qt::QueuedConnection);
    connect(m_backend, &IAudioBackend::errorOccurred, this, &AudioEngine::errorOccurred,
            Qt::QueuedConnection);
    connect(m_backend, &IAudioBackend::channelBusVuReady, m_masterBus, &MasterBus::updateMasterVu,
            Qt::QueuedConnection);
}

AudioEngine::~AudioEngine() = default;

// --- Propiedades expuestas ---

MasterBus* AudioEngine::masterBus() const { return m_masterBus; }
TrackLibrary* AudioEngine::trackLibrary() const { return m_trackLibrary; }

QQmlListProperty<AudioChannel> AudioEngine::channels()
{
    return { this, this, &AudioEngine::channelListCount, &AudioEngine::channelAtIndex };
}

AudioChannel* AudioEngine::channelAt(int id) const
{
    return isValidChannel(id) ? m_channels.at(id) : nullptr;
}

int AudioEngine::channelCount() const { return m_channelCount; }

void AudioEngine::setChannelCount(int count)
{
    const int n = std::clamp(count, k_MinChannelCount, k_MaxChannelCount);
    if (n == m_channelCount) return;

    if (n < m_channelCount) {
        for (int i = n; i < m_channelCount; ++i) {
            AudioChannel* ch = m_channels.at(i);
            ch->unbindTrack();
            ch->reset();
            ch->setName(AudioChannel::defaultName(i));
            ch->setColor(AudioChannel::defaultColor());
        }
    }

    m_channelCount = n;
    normalizeChannelOrder();
    emit channelCountChanged();
    emit channelsChanged();
}

void AudioEngine::normalizeChannelOrder()
{
    QList<int> filtered;
    for (const int id : m_channelOrder)
        if (id >= 0 && id < m_channelCount && !filtered.contains(id)) filtered.append(id);
    for (int id = 0; id < m_channelCount; ++id)
        if (!filtered.contains(id)) filtered.append(id);
    m_channelOrder = filtered;
}

// --- Control de canales ---

void AudioEngine::playChannel(int id)
{
    if (!isValidChannel(id)) return;
    m_channels.at(id)->play();
}

void AudioEngine::stopChannel(int id)
{
    if (!isValidChannel(id)) return;
    m_channels.at(id)->stop();
}

void AudioEngine::pauseChannel(int id)
{
    if (!isValidChannel(id)) return;
    m_channels.at(id)->pause();
}

void AudioEngine::setVolume(int id, float volume)
{
    if (!isValidChannel(id)) return;
    m_channels.at(id)->setVolume(volume);
}

void AudioEngine::setPan(int id, float pan)
{
    if (!isValidChannel(id)) return;
    m_channels.at(id)->setPan(pan);
}

bool AudioEngine::bindTrack(int channelId, const QString& filePath)
{
    if (!isValidChannel(channelId)) return false;
    return m_channels.at(channelId)->bindTrack(filePath);
}

bool AudioEngine::bindTrackFromUrl(int channelId, const QUrl& url)
{
    if (!url.isLocalFile()) {
        emit errorOccurred(tr("Only local files are supported: %1").arg(url.toString()));
        return false;
    }

    const QString filePath = url.toLocalFile();
    if (!bindTrack(channelId, filePath)) return false;

    m_trackLibrary->addFile(filePath);
    return true;
}

void AudioEngine::unbindChannel(int id)
{
    if (!isValidChannel(id)) return;
    m_channels.at(id)->unbindTrack();
}

void AudioEngine::resetChannelColor(int id)
{
    if (!isValidChannel(id)) return;
    m_channels.at(id)->setColor(AudioChannel::defaultColor());
}

QVariantList AudioEngine::channelOrder() const
{
    QVariantList list;
    list.reserve(m_channelOrder.size());
    for (const int id : m_channelOrder) list.append(id);
    return list;
}

void AudioEngine::setChannelOrder(const QVariantList& order)
{
    if (order.size() != m_channelCount) return;

    QList<int> parsed;
    QList<bool> seen(m_channelCount, false);
    for (const QVariant& v : order) {
        bool ok = false;
        const int id = v.toInt(&ok);
        if (!ok || id < 0 || id >= m_channelCount || seen.at(id)) return;
        seen[id] = true;
        parsed.append(id);
    }
    m_channelOrder = parsed;
}

bool AudioEngine::gridMode() const { return m_gridMode; }

void AudioEngine::setGridMode(bool gridMode)
{
    if (m_gridMode == gridMode) return;
    m_gridMode = gridMode;
    emit gridModeChanged();
}

// --- Control global ---

void AudioEngine::panic()
{
    qCWarning(lcAudioEngine) << "PANIC: stopping all playback immediately";
    emit panicRequested();
    m_backend->panic();
    stopAll();
}

bool AudioEngine::warming() const { return m_warming; }

void AudioEngine::warmTrackCache()
{
    if (!m_warmTimer) {
        m_warmTimer = new QTimer(this);
        m_warmTimer->setInterval(0);
        connect(m_warmTimer, &QTimer::timeout, this, &AudioEngine::onWarmTick);
    }
    m_warmQueue = m_trackLibrary->trackPaths();
    if (m_warmQueue.isEmpty()) return;

    if (!m_warming) {
        m_warming = true;
        emit warmingChanged();
    }
    m_warmTimer->start();
}

void AudioEngine::onWarmTick()
{
    if (m_warmQueue.isEmpty()) {
        m_warmTimer->stop();
        if (m_warming) {
            m_warming = false;
            emit warmingChanged();
        }
        qCInfo(lcAudioEngine) << "decode cache warmed";
        return;
    }
    m_backend->preloadDecode(m_warmQueue.takeFirst());
}

void AudioEngine::stopAll()
{
    for (AudioChannel* ch : m_channels) {
        ch->stop();
        ch->updateVuLevel(0.0f, 0.0f);
    }
    m_masterBus->updateMasterVu(0.0f, 0.0f);
}

// --- Routing de señales del backend (hilo principal, ya encolado) ---

void AudioEngine::onBackendVuLevel(int voiceId, float left, float right)
{
    for (AudioChannel* ch : m_channels) {
        if (ch->voiceId() != voiceId) continue;
        ch->updateVuLevel(left, right);
        emit vuLevelChanged(ch->id(), left, right);
        return;
    }
}

void AudioEngine::onBackendVoicePosition(int voiceId, float position)
{
    for (AudioChannel* ch : m_channels) {
        if (ch->voiceId() != voiceId) continue;
        ch->updateProgress(position);
        return;
    }
}

void AudioEngine::onBackendVoiceFinished(int voiceId)
{
    for (AudioChannel* ch : m_channels) {
        if (ch->voiceId() == voiceId) {
            ch->stop();
            return;
        }
    }
}

// --- Helpers privados ---

bool AudioEngine::isValidChannel(int id) const { return id >= 0 && id < m_channelCount; }

// --- QQmlListProperty (solo lectura) ---

qsizetype AudioEngine::channelListCount(QQmlListProperty<AudioChannel>* list)
{
    return static_cast<AudioEngine*>(list->data)->m_channelCount;
}

AudioChannel* AudioEngine::channelAtIndex(QQmlListProperty<AudioChannel>* list, qsizetype i)
{
    return static_cast<AudioEngine*>(list->data)->m_channels.at(i);
}
