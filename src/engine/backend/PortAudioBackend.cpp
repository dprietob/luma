// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "PortAudioBackend.h"

#include "dsp/VoiceEffects.h"

#include <QLoggingCategory>
#include <QTimer>
#include <QVarLengthArray>

#include <algorithm>
#include <cmath>
#include <cstring>

Q_LOGGING_CATEGORY(lcPortAudio, "luma.audio.backend.portaudio")

namespace {
constexpr auto k_Relaxed = std::memory_order_relaxed;
}

PortAudioBackend::PortAudioBackend(QObject* parent)
    : IAudioBackend(parent)
{
    m_voiceScratch.resize(static_cast<size_t>(k_MaxBlockFrames) * k_Channels);
    m_stFeed.resize(static_cast<size_t>(k_FramesPerBuffer) * k_Channels);
    m_voiceScratchAux.resize(static_cast<size_t>(k_MaxBlockFrames) * k_Channels);
    m_stFeedAux.resize(static_cast<size_t>(k_FramesPerBuffer) * k_Channels);
    m_vuTimer = new QTimer(this);
    m_vuTimer->setInterval(k_VuIntervalMs);
    connect(m_vuTimer, &QTimer::timeout, this, &PortAudioBackend::onVuTimerTick);
}

PortAudioBackend::~PortAudioBackend() { shutdown(); }

bool PortAudioBackend::initialize()
{
    if (m_initialized) return true;

    const PaError err = Pa_Initialize();
    if (err != paNoError) {
        qCCritical(lcPortAudio) << "Pa_Initialize failed:" << Pa_GetErrorText(err);
        emit errorOccurred(
            tr("Could not initialize audio: %1").arg(QString::fromUtf8(Pa_GetErrorText(err))));
        return false;
    }
    m_paReady = true;

    reconfigureStreams();
    if (!m_streamMain) {
        Pa_Terminate();
        m_paReady = false;
        return false;
    }

    m_vuTimer->start();
    m_initialized = true;
    return true;
}

PaDeviceIndex PortAudioBackend::preferredDefaultDevice() const
{
    const PaHostApiIndex jackApi = Pa_HostApiTypeIdToHostApiIndex(paJACK);
    if (jackApi >= 0) {
        if (const PaHostApiInfo* info = Pa_GetHostApiInfo(jackApi))
            if (info->defaultOutputDevice != paNoDevice) return info->defaultOutputDevice;
    }
    return Pa_GetDefaultOutputDevice();
}

PaDeviceIndex PortAudioBackend::resolveDevice(int requestedIndex) const
{
    if (requestedIndex >= 0 && requestedIndex < Pa_GetDeviceCount()) {
        const PaDeviceInfo* di = Pa_GetDeviceInfo(requestedIndex);
        if (di && di->maxOutputChannels > 0) return requestedIndex;
    }
    return preferredDefaultDevice();
}

bool PortAudioBackend::openStreamOn(PaDeviceIndex dev, PaStream** stream, void* userData,
                                    double* outSampleRate)
{
    if (dev == paNoDevice) {
        qCWarning(lcPortAudio) << "no output device available";
        emit errorOccurred(tr("No audio output device available."));
        return false;
    }
    const PaDeviceInfo* di = Pa_GetDeviceInfo(dev);
    if (!di) return false;

    PaStreamParameters out {};
    out.device = dev;
    out.channelCount = k_Channels;
    out.sampleFormat = paFloat32;
    out.suggestedLatency = di->defaultLowOutputLatency;
    out.hostApiSpecificStreamInfo = nullptr;

    const PaError err = Pa_OpenStream(stream, nullptr, &out, k_SampleRate, k_FramesPerBuffer,
                                      paClipOff, &PortAudioBackend::paCallback, userData);
    if (err != paNoError) {
        qCWarning(lcPortAudio) << "Pa_OpenStream failed on device" << dev << ":"
                               << Pa_GetErrorText(err);
        emit errorOccurred(
            tr("Could not open audio stream: %1").arg(QString::fromUtf8(Pa_GetErrorText(err))));
        *stream = nullptr;
        return false;
    }

    if (outSampleRate) {
        if (const PaStreamInfo* si = Pa_GetStreamInfo(*stream))
            if (si->sampleRate > 0.0) *outSampleRate = si->sampleRate;
    }

    Pa_StartStream(*stream);
    const PaHostApiInfo* hi = Pa_GetHostApiInfo(di->hostApi);
    qCInfo(lcPortAudio) << "output stream on:" << di->name << "via" << (hi ? hi->name : "?");
    return true;
}

void PortAudioBackend::closeStream(PaStream** stream)
{
    if (*stream) {
        Pa_StopStream(*stream);
        Pa_CloseStream(*stream);
        *stream = nullptr;
    }
}

void PortAudioBackend::reconfigureStreams()
{
    if (!m_paReady) return;

    const PaDeviceIndex mainDev = resolveDevice(m_reqMainIndex);
    if (mainDev != m_mainDeviceIndex || !m_streamMain) {
        closeStream(&m_streamMain);
        m_mainDeviceIndex = openStreamOn(mainDev, &m_streamMain, &m_mainCtx, &m_outputSampleRate)
                                ? mainDev
                                : paNoDevice;
    }

    const PaDeviceIndex auxDev = resolveDevice(m_reqAuxIndex);
    const bool wantAux = m_reqAuxIndex >= 0 && auxDev != paNoDevice && auxDev != m_mainDeviceIndex;
    if (wantAux) {
        if (auxDev != m_auxDeviceIndex || !m_streamAux) {
            closeStream(&m_streamAux);
            m_auxDeviceIndex = openStreamOn(auxDev, &m_streamAux, &m_auxCtx, &m_auxSampleRate)
                                   ? auxDev
                                   : paNoDevice;
        }
    } else {
        closeStream(&m_streamAux);
        m_auxDeviceIndex = paNoDevice;
    }
    m_auxStreamActive = (m_streamAux != nullptr);
}

void PortAudioBackend::refreshDevices()
{
    if (!m_paReady) {
        if (Pa_Initialize() == paNoError) m_paReady = true;
        return;
    }

    const bool wasRunning = m_initialized;
    m_vuTimer->stop();
    closeStream(&m_streamMain);
    closeStream(&m_streamAux);
    m_auxStreamActive = false;
    m_mainDeviceIndex = paNoDevice;
    m_auxDeviceIndex = paNoDevice;

    Pa_Terminate();
    if (Pa_Initialize() != paNoError) {
        m_paReady = false;
        m_initialized = false;
        return;
    }

    const PaDeviceIndex def = preferredDefaultDevice();
    if (openStreamOn(def, &m_streamMain, &m_mainCtx, &m_outputSampleRate)) {
        m_mainDeviceIndex = def;
        m_initialized = true;
        if (wasRunning) m_vuTimer->start();
    } else {
        m_initialized = false;
    }
}

QList<AudioDeviceInfo> PortAudioBackend::outputDevices() const
{
    QList<AudioDeviceInfo> devices;
    if (!m_paReady) return devices;

    const PaDeviceIndex count = Pa_GetDeviceCount();
    for (PaDeviceIndex i = 0; i < count; ++i) {
        const PaDeviceInfo* di = Pa_GetDeviceInfo(i);
        if (!di || di->maxOutputChannels <= 0) continue;
        const PaHostApiInfo* hi = Pa_GetHostApiInfo(di->hostApi);
        const QString name = hi ? QStringLiteral("%1 — %2").arg(QString::fromUtf8(di->name),
                                                                QString::fromUtf8(hi->name))
                                : QString::fromUtf8(di->name);
        devices.append({ i, name });
    }
    return devices;
}

void PortAudioBackend::setOutputDevices(int mainIndex, int auxIndex)
{
    m_reqMainIndex = mainIndex;
    m_reqAuxIndex = auxIndex;
    if (m_initialized) reconfigureStreams();
}

void PortAudioBackend::shutdown()
{
    if (!m_paReady) return;
    m_vuTimer->stop();
    closeStream(&m_streamMain);
    closeStream(&m_streamAux);
    m_auxStreamActive = false;
    Pa_Terminate();
    m_paReady = false;
    {
        QMutexLocker lock(&m_voiceMutex);
        m_voices.clear();
    }
    m_initialized = false;
    qCInfo(lcPortAudio) << "audio stream stopped";
}

std::shared_ptr<PortAudioBackend::Voice> PortAudioBackend::voiceFor(int voiceId) const
{
    QMutexLocker lock(&m_voiceMutex);
    return m_voices.value(voiceId, nullptr);
}

std::shared_ptr<const DecodedAudio> PortAudioBackend::cachedDecode(const QString& filePath)
{
    const auto it = m_decodeCache.constFind(filePath);
    if (it == m_decodeCache.constEnd()) return nullptr;
    m_decodeCacheOrder.removeOne(filePath);
    m_decodeCacheOrder.prepend(filePath);
    return it.value();
}

void PortAudioBackend::cacheStore(const QString& filePath,
                                  const std::shared_ptr<const DecodedAudio>& audio)
{
    const qint64 bytes = static_cast<qint64>(audio->samples.size() * sizeof(float));

    if (m_decodeCache.contains(filePath)) {
        m_decodeCacheBytes -=
            static_cast<qint64>(m_decodeCache.value(filePath)->samples.size() * sizeof(float));
        m_decodeCacheOrder.removeOne(filePath);
    }
    m_decodeCache.insert(filePath, audio);
    m_decodeCacheOrder.prepend(filePath);
    m_decodeCacheBytes += bytes;

    while (m_decodeCacheBytes > m_decodeCacheMaxBytes && !m_decodeCacheOrder.isEmpty()) {
        const QString victim = m_decodeCacheOrder.takeLast();
        m_decodeCacheBytes -=
            static_cast<qint64>(m_decodeCache.value(victim)->samples.size() * sizeof(float));
        m_decodeCache.remove(victim);
    }
}

void PortAudioBackend::setDecodeCacheBytes(qint64 bytes)
{
    m_decodeCacheMaxBytes = std::max<qint64>(0, bytes);
    while (m_decodeCacheBytes > m_decodeCacheMaxBytes && !m_decodeCacheOrder.isEmpty()) {
        const QString victim = m_decodeCacheOrder.takeLast();
        m_decodeCacheBytes -=
            static_cast<qint64>(m_decodeCache.value(victim)->samples.size() * sizeof(float));
        m_decodeCache.remove(victim);
    }
}

void PortAudioBackend::preloadDecode(const QString& filePath)
{
    if (filePath.isEmpty() || cachedDecode(filePath)) return;
    QString error;
    std::optional<DecodedAudio> decoded = AudioFileDecoder::decode(filePath, &error);
    if (!decoded) {
        qCWarning(lcPortAudio) << "preload decode failed for" << filePath << ":" << error;
        return;
    }
    cacheStore(filePath, std::make_shared<const DecodedAudio>(std::move(*decoded)));
}

int PortAudioBackend::loadSource(const QString& filePath)
{
    std::shared_ptr<const DecodedAudio> audio = cachedDecode(filePath);
    if (!audio) {
        QString error;
        std::optional<DecodedAudio> decoded = AudioFileDecoder::decode(filePath, &error);
        if (!decoded) {
            qCWarning(lcPortAudio) << "decode failed for" << filePath << ":" << error;
            return k_InvalidVoice;
        }
        audio = std::make_shared<const DecodedAudio>(std::move(*decoded));
        cacheStore(filePath, audio);
    }

    auto voice = std::make_shared<Voice>();
    voice->filePath = filePath;
    voice->audio = audio;
    voice->fx = std::make_unique<VoiceEffects>(static_cast<int>(m_outputSampleRate));

    QMutexLocker lock(&m_voiceMutex);
    voice->id = nextVoiceId();
    m_voices.insert(voice->id, voice);
    qCInfo(lcPortAudio) << "loaded voice" << voice->id << ":" << filePath << "source"
                        << voice->audio->sampleRate << "Hz," << voice->audio->frameCount()
                        << "frames, output" << m_outputSampleRate << "Hz (ratio"
                        << (voice->audio->sampleRate / m_outputSampleRate) << ")";
    return voice->id;
}

void PortAudioBackend::releaseSource(int voiceId)
{
    QMutexLocker lock(&m_voiceMutex);
    m_voices.remove(voiceId);
}

void PortAudioBackend::playVoice(int voiceId, bool restart)
{
    const auto voice = voiceFor(voiceId);
    if (!voice) return;
    if (restart) voice->resetRequested.store(true, k_Relaxed);
    voice->finished.store(false, k_Relaxed);
    voice->playing.store(true, k_Relaxed);
}

void PortAudioBackend::stopVoice(int voiceId)
{
    const auto voice = voiceFor(voiceId);
    if (!voice) return;
    voice->playing.store(false, k_Relaxed);
    voice->resetRequested.store(true, k_Relaxed);
    voice->positionFrames.store(0.0, k_Relaxed);
}

void PortAudioBackend::pauseVoice(int voiceId)
{
    const auto voice = voiceFor(voiceId);
    if (!voice) return;
    voice->playing.store(false, k_Relaxed);
}

void PortAudioBackend::setVoiceVolume(int voiceId, float volume)
{
    if (const auto voice = voiceFor(voiceId)) voice->volume.store(volume, k_Relaxed);
}

void PortAudioBackend::setVoicePan(int voiceId, float pan)
{
    if (const auto voice = voiceFor(voiceId)) voice->pan.store(pan, k_Relaxed);
}

void PortAudioBackend::setVoiceLooping(int voiceId, bool looping)
{
    if (const auto voice = voiceFor(voiceId)) voice->looping.store(looping, k_Relaxed);
}

void PortAudioBackend::setVoiceAux(int voiceId, bool aux)
{
    if (const auto voice = voiceFor(voiceId)) voice->aux.store(aux, k_Relaxed);
}

void PortAudioBackend::setVoiceReverb(int voiceId, bool enabled, float roomSize, float damping,
                                      float mix)
{
    if (const auto voice = voiceFor(voiceId); voice && voice->fx)
        voice->fx->setReverb(enabled, roomSize, damping, mix);
}

void PortAudioBackend::setVoiceDelay(int voiceId, bool enabled, float timeMs, float feedback,
                                     float mix)
{
    if (const auto voice = voiceFor(voiceId); voice && voice->fx)
        voice->fx->setDelay(enabled, timeMs, feedback, mix);
}

void PortAudioBackend::setVoiceDistortion(int voiceId, bool enabled, float drive, float mix)
{
    if (const auto voice = voiceFor(voiceId); voice && voice->fx)
        voice->fx->setDistortion(enabled, drive, mix);
}

void PortAudioBackend::setVoicePitch(int voiceId, bool enabled, float semitones)
{
    if (const auto voice = voiceFor(voiceId); voice && voice->fx)
        voice->fx->setPitch(enabled, semitones);
}

void PortAudioBackend::setVoiceSpeed(int voiceId, bool enabled, float tempo)
{
    if (const auto voice = voiceFor(voiceId); voice && voice->fx)
        voice->fx->setSpeed(enabled, tempo);
}

void PortAudioBackend::setVoiceEq(int voiceId, bool enabled, const QList<float>& gainsDb)
{
    if (const auto voice = voiceFor(voiceId); voice && voice->fx)
        voice->fx->setEq(enabled, gainsDb.constData(), static_cast<int>(gainsDb.size()));
}

void PortAudioBackend::setVoiceRegion(int voiceId, double startFrac, double endFrac)
{
    if (const auto voice = voiceFor(voiceId)) {
        voice->regionStart.store(std::clamp(startFrac, 0.0, 1.0), k_Relaxed);
        voice->regionEnd.store(std::clamp(endFrac, 0.0, 1.0), k_Relaxed);
    }
}

void PortAudioBackend::setVoicePlayhead(int voiceId, double seconds)
{
    const auto voice = voiceFor(voiceId);
    if (!voice || !voice->audio || voice->audio->sampleRate <= 0) return;
    const double frames = static_cast<double>(voice->audio->frameCount());
    const double target = std::clamp(seconds * voice->audio->sampleRate, 0.0, frames);
    voice->seekFrames.store(target, k_Relaxed);
}

double PortAudioBackend::voiceDurationSeconds(int voiceId) const
{
    const auto voice = voiceFor(voiceId);
    if (!voice || !voice->audio || voice->audio->sampleRate <= 0) return 0.0;
    return static_cast<double>(voice->audio->frameCount()) / voice->audio->sampleRate;
}

QList<float> PortAudioBackend::voiceWaveform(int voiceId, int buckets) const
{
    QList<float> peaks;
    const auto voice = voiceFor(voiceId);
    if (!voice || !voice->audio || buckets <= 0) return peaks;

    const std::vector<float>& s = voice->audio->samples;
    const std::int64_t frames = voice->audio->frameCount();
    if (frames <= 0) return peaks;

    peaks.reserve(buckets);
    for (int b = 0; b < buckets; ++b) {
        const std::int64_t f0 = frames * b / buckets;
        const std::int64_t f1 = std::max(f0 + 1, frames * (b + 1) / buckets);
        float peak = 0.0f;
        for (std::int64_t f = f0; f < f1 && f < frames; ++f)
            peak = std::max(peak, std::max(std::fabs(s[static_cast<size_t>(f) * 2]),
                                           std::fabs(s[static_cast<size_t>(f) * 2 + 1])));
        peaks.append(peak);
    }
    return peaks;
}

void PortAudioBackend::setChannelMasterVolume(float volume)
{
    m_channelMasterVolume.store(volume, k_Relaxed);
}

void PortAudioBackend::panic()
{
    QMutexLocker lock(&m_voiceMutex);
    for (const auto& voice : std::as_const(m_voices)) {
        voice->playing.store(false, k_Relaxed);
        voice->resetRequested.store(true, k_Relaxed);
    }
}

// --- Hilo de audio de tiempo real ---

int PortAudioBackend::paCallback(const void*, void* output, unsigned long frameCount,
                                 const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags,
                                 void* userData)
{
    auto* ctx = static_cast<StreamContext*>(userData);
    ctx->backend->renderRoute(static_cast<float*>(output), frameCount, ctx->aux);
    return paContinue;
}

void PortAudioBackend::renderRoute(float* output, unsigned long frameCount, bool auxRoute)
{
    const size_t sampleCount = static_cast<size_t>(frameCount) * k_Channels;
    std::fill(output, output + sampleCount, 0.0f);

    const unsigned long block = std::min<unsigned long>(frameCount, k_MaxBlockFrames);

    QVarLengthArray<std::shared_ptr<Voice>, 64> active;
    {
        QMutexLocker lock(&m_voiceMutex);
        for (const auto& voice : std::as_const(m_voices)) active.append(voice);
    }

    const float channelMasterVol = m_channelMasterVolume.load(k_Relaxed);
    const double outRate = auxRoute ? m_auxSampleRate : m_outputSampleRate;
    float* scratch = auxRoute ? m_voiceScratchAux.data() : m_voiceScratch.data();
    float* feed = auxRoute ? m_stFeedAux.data() : m_stFeed.data();
    const bool auxActive = m_auxStreamActive;

    for (const auto& voice : active) {
        if (!voice->playing.load(k_Relaxed)) continue;
        const std::shared_ptr<const DecodedAudio> audio = voice->audio;
        if (!audio || audio->isEmpty()) continue;

        const bool isAux = voice->aux.load(k_Relaxed);

        if (auxRoute) {
            if (!isAux) continue;
        } else if (isAux && auxActive) {
            continue;
        }

        if (voice->rendering.exchange(true, std::memory_order_acquire)) continue;

        const float vol = voice->volume.load(k_Relaxed);
        const float pan = voice->pan.load(k_Relaxed);
        const float gainLeft = vol * (pan <= 0.0f ? 1.0f : 1.0f - pan);
        const float gainRight = vol * (pan >= 0.0f ? 1.0f : 1.0f + pan);
        const bool looping = voice->looping.load(k_Relaxed);

        const std::int64_t frames = audio->frameCount();
        const float* src = audio->samples.data();
        const double ratio = static_cast<double>(audio->sampleRate) / outRate;

        const double framesD = static_cast<double>(frames);
        const double startPos = std::clamp(voice->regionStart.load(k_Relaxed), 0.0, 1.0) * framesD;
        double endPos = std::clamp(voice->regionEnd.load(k_Relaxed), 0.0, 1.0) * framesD;
        if (endPos <= startPos) endPos = framesD;
        const double regionLen = endPos - startPos;

        const double pendingSeek = voice->seekFrames.exchange(-1.0, k_Relaxed);
        const bool resetting = voice->resetRequested.exchange(false, k_Relaxed);
        if (pendingSeek >= 0.0)
            voice->readPos = std::clamp(pendingSeek, 0.0, framesD);
        else if (resetting)
            voice->readPos = startPos;

        double pos = voice->readPos;
        bool ended = false;

        const auto readFrames = [&](float* dst, unsigned long want) -> unsigned long {
            unsigned long n = 0;
            for (; n < want; ++n) {
                if (pos >= endPos) {
                    if (looping && regionLen > 0.0) {
                        pos = startPos + std::fmod(pos - startPos, regionLen);
                    } else {
                        ended = true;
                        break;
                    }
                }
                const std::int64_t i0 = static_cast<std::int64_t>(pos);
                const std::int64_t i1 = std::min(i0 + 1, frames - 1);
                const float frac = static_cast<float>(pos - static_cast<double>(i0));
                dst[n * 2] = src[i0 * 2] + (src[i1 * 2] - src[i0 * 2]) * frac;
                dst[n * 2 + 1] = src[i0 * 2 + 1] + (src[i1 * 2 + 1] - src[i0 * 2 + 1]) * frac;
                pos += ratio;
            }
            return n;
        };

        VoiceEffects* fx = voice->fx.get();
        const bool stActive = fx && fx->timeStretchActive();
        unsigned long produced = 0;

        if (stActive) {
            soundtouch::SoundTouch& st = fx->stretcher();
            if (!voice->stActive) {
                st.clear();
                voice->stActive = true;
            }
            fx->syncStretchParams();
            bool flushed = false;
            while (produced < block) {
                const unsigned int got = st.receiveSamples(
                    scratch + produced * 2, static_cast<unsigned int>(block - produced));
                produced += got;
                if (produced >= block) break;
                if (ended) {
                    if (!flushed) {
                        st.flush();
                        flushed = true;
                        continue;
                    }
                    break;
                }
                const unsigned long n =
                    readFrames(feed, static_cast<unsigned long>(k_FramesPerBuffer));
                if (n > 0) st.putSamples(feed, static_cast<unsigned int>(n));
            }
        } else {
            if (voice->stActive && fx) {
                fx->stretcher().clear();
                voice->stActive = false;
            }
            produced = readFrames(scratch, block);
        }

        for (unsigned long i = produced * 2; i < block * 2; ++i) scratch[i] = 0.0f;

        if (fx) fx->processChain(scratch, static_cast<int>(block));

        float peakLeft = 0.0f;
        float peakRight = 0.0f;
        for (unsigned long i = 0; i < block; ++i) {
            const float outLeft = scratch[i * 2] * gainLeft;
            const float outRight = scratch[i * 2 + 1] * gainRight;
            output[i * 2] += outLeft * channelMasterVol;
            output[i * 2 + 1] += outRight * channelMasterVol;
            peakLeft = std::max(peakLeft, std::fabs(outLeft));
            peakRight = std::max(peakRight, std::fabs(outRight));
        }

        voice->readPos = pos;
        voice->positionFrames.store(pos, k_Relaxed);
        voice->peakLeft.store(std::max(voice->peakLeft.load(k_Relaxed), peakLeft), k_Relaxed);
        voice->peakRight.store(std::max(voice->peakRight.load(k_Relaxed), peakRight), k_Relaxed);

        if (ended) {
            voice->playing.store(false, k_Relaxed);
            voice->readPos = startPos;
            voice->positionFrames.store(startPos, k_Relaxed);
            voice->finished.store(true, k_Relaxed);
            if (fx) fx->stretcher().clear();
            voice->stActive = false;
        }

        voice->rendering.store(false, std::memory_order_release);
    }

    float channelBusPeakLeft = 0.0f;
    float channelBusPeakRight = 0.0f;
    for (unsigned long i = 0; i < frameCount; ++i) {
        channelBusPeakLeft = std::max(channelBusPeakLeft, std::fabs(output[i * 2]));
        channelBusPeakRight = std::max(channelBusPeakRight, std::fabs(output[i * 2 + 1]));
    }

    if (!auxRoute) {
        m_channelBusPeakLeft.store(
            std::max(m_channelBusPeakLeft.load(k_Relaxed), channelBusPeakLeft), k_Relaxed);
        m_channelBusPeakRight.store(
            std::max(m_channelBusPeakRight.load(k_Relaxed), channelBusPeakRight), k_Relaxed);
    }

    for (size_t i = 0; i < sampleCount; ++i) {
        output[i] = std::clamp(output[i], -1.0f, 1.0f);
    }
}

// --- Publicación de VU / fin de voz al hilo de UI ---

void PortAudioBackend::onVuTimerTick()
{
    struct Report
    {
        int id;
        float left;
        float right;
        float position;
        bool finished;
    };
    QVarLengthArray<Report, 64> reports;
    {
        QMutexLocker lock(&m_voiceMutex);
        for (const auto& voice : std::as_const(m_voices)) {
            const std::int64_t frames = voice->audio ? voice->audio->frameCount() : 0;
            const double posFrames = voice->positionFrames.load(k_Relaxed);
            const float position =
                frames > 0 ? static_cast<float>(
                                 std::clamp(posFrames / static_cast<double>(frames), 0.0, 1.0))
                           : 0.0f;
            reports.append({ voice->id, voice->peakLeft.exchange(0.0f, k_Relaxed),
                             voice->peakRight.exchange(0.0f, k_Relaxed), position,
                             voice->finished.exchange(false, k_Relaxed) });
        }
    }
    for (const Report& r : reports) {
        emit vuLevelReady(r.id, r.left, r.right);
        emit voicePositionReady(r.id, r.position);
        if (r.finished) emit voiceFinished(r.id);
    }
    emit channelBusVuReady(m_channelBusPeakLeft.exchange(0.0f, k_Relaxed),
                           m_channelBusPeakRight.exchange(0.0f, k_Relaxed));
}

int PortAudioBackend::nextVoiceId() { return m_nextVoiceId++; }
