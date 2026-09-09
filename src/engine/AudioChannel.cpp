// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "AudioChannel.h"

#include "ChannelEffects.h"
#include "interfaces/IAudioBackend.h"

#include <QFileInfo>
#include <QLoggingCategory>

#include <algorithm>
#include <cmath>

Q_LOGGING_CATEGORY(lcAudioChannel, "luma.audio.channel")

AudioChannel::AudioChannel(int id, IAudioBackend* backend, QObject* parent)
    : QObject(parent)
    , m_id(id)
    , m_backend(backend)
{
    Q_ASSERT_X(m_backend != nullptr, "AudioChannel", "backend must be injected");

    m_name = defaultName(m_id);
    m_effects = new ChannelEffects(m_backend, this);
    m_timeline = new TrackTimeline(m_backend, this);
}

QString AudioChannel::defaultName(int id)
{
    return QStringLiteral("CH %1").arg(id + 1, 2, 10, QLatin1Char('0'));
}

QColor AudioChannel::defaultColor() { return QColor(0xE8, 0x60, 0x1C); }

AudioChannel::~AudioChannel()
{
    if (m_backend && m_voiceId != IAudioBackend::k_InvalidVoice) {
        m_backend->releaseSource(m_voiceId);
    }
}

// --- Getters ---

int AudioChannel::id() const { return m_id; }
int AudioChannel::voiceId() const { return m_voiceId; }
QString AudioChannel::name() const { return m_name; }
QString AudioChannel::filePath() const { return m_filePath; }
bool AudioChannel::hasTrack() const { return !m_filePath.isEmpty(); }
float AudioChannel::volume() const { return m_volume; }
float AudioChannel::pan() const { return m_pan; }
QColor AudioChannel::color() const { return m_color; }
bool AudioChannel::loop() const { return m_loop; }
bool AudioChannel::aux() const { return m_aux; }
bool AudioChannel::isPlaying() const { return m_isPlaying; }
bool AudioChannel::isPaused() const { return m_isPaused; }
int AudioChannel::fadeSeconds() const { return m_fadeSeconds; }
int AudioChannel::fadeMode() const { return m_fadeMode; }
float AudioChannel::fadeMax() const { return m_fadeMax; }
float AudioChannel::fadeMin() const { return m_fadeMin; }
float AudioChannel::vuLeft() const { return m_vuLeft; }
float AudioChannel::vuRight() const { return m_vuRight; }
float AudioChannel::progress() const { return m_progress; }
ChannelEffects* AudioChannel::effects() const { return m_effects; }
TrackTimeline* AudioChannel::timeline() const { return m_timeline; }

// --- Setters con emisión condicional ---

void AudioChannel::setName(const QString& name)
{
    if (m_name == name) return;
    m_name = name;
    emit nameChanged();
}

void AudioChannel::setVolume(float volume)
{
    const float clamped = std::clamp(volume, 0.0f, k_MaxVolume);
    if (qFuzzyCompare(m_volume, clamped)) return;
    m_volume = clamped;
    applyEffectiveVolume();
    qCDebug(lcAudioChannel) << "channel" << m_id << "volume ->" << m_volume;
    emit volumeChanged();
}

void AudioChannel::setColor(const QColor& color)
{
    if (m_color == color) return;
    m_color = color;
    emit colorChanged();
}

void AudioChannel::setLoop(bool loop)
{
    if (m_loop == loop) return;
    m_loop = loop;
    if (m_voiceId != IAudioBackend::k_InvalidVoice) {
        m_backend->setVoiceLooping(m_voiceId, m_loop);
    }
    emit loopChanged();
}

void AudioChannel::setAux(bool aux)
{
    if (m_aux == aux) return;
    m_aux = aux;
    if (m_voiceId != IAudioBackend::k_InvalidVoice) {
        m_backend->setVoiceAux(m_voiceId, m_aux);
    }
    emit auxChanged();
}

void AudioChannel::setFadeSeconds(int seconds)
{
    const int clamped = std::max(0, seconds);
    if (m_fadeSeconds == clamped) return;
    m_fadeSeconds = clamped;
    emit fadeSecondsChanged();
}

void AudioChannel::setFadeMode(int mode)
{
    const int clamped = std::max(0, mode);
    if (m_fadeMode == clamped) return;
    m_fadeMode = clamped;
    emit fadeModeChanged();
}

void AudioChannel::setFadeMax(float value)
{
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    if (qFuzzyCompare(m_fadeMax, clamped)) return;
    m_fadeMax = clamped;
    emit fadeMaxChanged();
}

void AudioChannel::setFadeMin(float value)
{
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    if (qFuzzyCompare(m_fadeMin, clamped)) return;
    m_fadeMin = clamped;
    emit fadeMinChanged();
}

void AudioChannel::setPan(float pan)
{
    const float clamped = std::clamp(pan, -1.0f, 1.0f);
    if (qFuzzyCompare(m_pan, clamped)) return;
    m_pan = clamped;
    if (m_voiceId != IAudioBackend::k_InvalidVoice) {
        m_backend->setVoicePan(m_voiceId, m_pan);
    }
    emit panChanged();
}

// --- Vinculación de pista ---

bool AudioChannel::bindTrack(const QString& filePath)
{
    if (!QFileInfo::exists(filePath)) {
        qCWarning(lcAudioChannel) << "channel" << m_id << "file not found:" << filePath;
        emit errorOccurred(tr("File not found: %1").arg(filePath));
        return false;
    }

    if (m_voiceId != IAudioBackend::k_InvalidVoice) {
        m_backend->releaseSource(m_voiceId);
        m_voiceId = IAudioBackend::k_InvalidVoice;
    }

    const int voice = m_backend->loadSource(filePath);
    if (voice == IAudioBackend::k_InvalidVoice) {
        qCWarning(lcAudioChannel) << "channel" << m_id << "backend failed to load:" << filePath;
        emit errorOccurred(tr("Could not load file: %1").arg(filePath));
        return false;
    }

    m_voiceId = voice;
    m_filePath = filePath;
    m_backend->setVoicePan(m_voiceId, m_pan);
    m_backend->setVoiceLooping(m_voiceId, m_loop);
    m_backend->setVoiceAux(m_voiceId, m_aux);
    applyEffectiveVolume();
    m_effects->setVoiceId(m_voiceId);
    m_timeline->bindVoice(m_voiceId);

    emit filePathChanged();
    emit hasTrackChanged();
    qCDebug(lcAudioChannel) << "channel" << m_id << "bound:" << filePath;
    return true;
}

void AudioChannel::reset()
{
    resetTransport();
    m_effects->resetAll();
    m_timeline->reset();
    m_timeline->setRegionEnabled(true);
    setVolume(k_DefaultVolume);
    setPan(0.0f);
    setLoop(false);
    setAux(false);
    setFadeSeconds(k_DefaultFadeSeconds);
    setFadeMode(0);
    setFadeMax(1.0f);
    setFadeMin(0.0f);
}

void AudioChannel::releaseAudio()
{
    if (!hasTrack()) return;
    if (m_voiceId != IAudioBackend::k_InvalidVoice) {
        m_backend->releaseSource(m_voiceId);
        m_voiceId = IAudioBackend::k_InvalidVoice;
    }
    m_filePath.clear();
    m_effects->setVoiceId(IAudioBackend::k_InvalidVoice);
    m_timeline->unbindVoice();
    emit filePathChanged();
    emit hasTrackChanged();
}

void AudioChannel::unbindTrack()
{
    if (!hasTrack()) return;
    reset();
    releaseAudio();
}

void AudioChannel::setPlayhead(double seconds)
{
    if (m_voiceId != IAudioBackend::k_InvalidVoice) m_backend->setVoicePlayhead(m_voiceId, seconds);
}

QJsonObject AudioChannel::captureState() const
{
    QJsonObject o;
    o[QStringLiteral("volume")] = m_volume;
    o[QStringLiteral("pan")] = m_pan;
    o[QStringLiteral("loop")] = m_loop;
    o[QStringLiteral("aux")] = m_aux;
    o[QStringLiteral("fadeSeconds")] = m_fadeSeconds;
    o[QStringLiteral("fadeMode")] = m_fadeMode;
    o[QStringLiteral("fadeMax")] = m_fadeMax;
    o[QStringLiteral("fadeMin")] = m_fadeMin;
    o[QStringLiteral("effects")] = m_effects->toJson();
    o[QStringLiteral("region")] = m_timeline->toJson();
    o[QStringLiteral("playing")] = m_isPlaying;
    const double duration = m_timeline->durationSeconds();
    o[QStringLiteral("positionSeconds")] =
        (m_isPlaying || m_isPaused) ? m_progress * duration : 0.0;
    return o;
}

void AudioChannel::restoreState(const QJsonObject& state, bool restoreTransport)
{
    stop();
    setVolume(static_cast<float>(state.value(QStringLiteral("volume")).toDouble(k_DefaultVolume)));
    setPan(static_cast<float>(state.value(QStringLiteral("pan")).toDouble(0.0)));
    setLoop(state.value(QStringLiteral("loop")).toBool(false));
    setAux(state.value(QStringLiteral("aux")).toBool(false));
    setFadeSeconds(state.value(QStringLiteral("fadeSeconds")).toInt(k_DefaultFadeSeconds));
    setFadeMode(state.value(QStringLiteral("fadeMode")).toInt(0));
    setFadeMax(static_cast<float>(state.value(QStringLiteral("fadeMax")).toDouble(1.0)));
    setFadeMin(static_cast<float>(state.value(QStringLiteral("fadeMin")).toDouble(0.0)));
    m_effects->applyJson(state.value(QStringLiteral("effects")).toObject());
    m_timeline->applyJson(state.value(QStringLiteral("region")).toObject());

    if (restoreTransport && state.value(QStringLiteral("playing")).toBool(false)) {
        setPlayhead(state.value(QStringLiteral("positionSeconds")).toDouble(0.0));
        play();
    }
}

// --- IPlayable ---

void AudioChannel::play()
{
    if (!hasTrack()) return;
    applyEffectiveVolume();
    m_backend->playVoice(m_voiceId, !m_isPaused);
    m_isPaused = false;
    emit isPausedChanged();
    setPlaying(true);
}

void AudioChannel::stop()
{
    if (m_voiceId != IAudioBackend::k_InvalidVoice) {
        m_backend->stopVoice(m_voiceId);
    }
    m_isPaused = false;
    emit isPausedChanged();
    setPlaying(false);
}

void AudioChannel::pause()
{
    if (!m_isPlaying) return;
    m_backend->pauseVoice(m_voiceId);
    m_isPaused = true;
    emit isPausedChanged();
    setPlaying(false);
}

void AudioChannel::updateVuLevel(float left, float right)
{
    const float l = std::clamp(left, 0.0f, 1.0f);
    const float r = std::clamp(right, 0.0f, 1.0f);
    if (qFuzzyCompare(m_vuLeft, l) && qFuzzyCompare(m_vuRight, r)) return;
    m_vuLeft = l;
    m_vuRight = r;
    emit vuLevelChanged();
}

void AudioChannel::updateProgress(float progress)
{
    const float clamped = std::clamp(progress, 0.0f, 1.0f);
    if (qFuzzyCompare(m_progress, clamped)) return;
    m_progress = clamped;
    emit progressChanged();
}

void AudioChannel::resetTransport()
{
    if (m_voiceId != IAudioBackend::k_InvalidVoice) m_backend->stopVoice(m_voiceId);
    if (m_isPaused) {
        m_isPaused = false;
        emit isPausedChanged();
    }
    setPlaying(false);
    updateVuLevel(0.0f, 0.0f);
    m_progress = 0.0f;
    emit progressChanged();
}

// --- Helpers privados ---

void AudioChannel::applyEffectiveVolume()
{
    if (m_voiceId == IAudioBackend::k_InvalidVoice) return;
    m_backend->setVoiceVolume(m_voiceId, m_volume);
}

void AudioChannel::setPlaying(bool playing)
{
    if (m_isPlaying == playing) return;
    m_isPlaying = playing;
    emit isPlayingChanged();
}
