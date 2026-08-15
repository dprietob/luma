// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "AudioFileDecoder.h"

#include <QFileInfo>
#include <QLoggingCategory>

#include <sndfile.h>
#include <mpg123.h>

#include <cstring>
#include <mutex>

Q_LOGGING_CATEGORY(lcDecoder, "luma.audio.decoder")

namespace {

void ensureMpg123Initialized()
{
    static std::once_flag flag;
    std::call_once(flag, []() { mpg123_init(); });
}

void appendAsStereo(DecodedAudio& out, const float* frames, std::int64_t frameCount, int channels)
{
    const size_t base = out.samples.size();
    out.samples.resize(base + static_cast<size_t>(frameCount) * 2);
    for (std::int64_t f = 0; f < frameCount; ++f) {
        float left;
        float right;
        if (channels == 1) {
            left = right = frames[f];
        } else {
            left = frames[f * channels + 0];
            right = frames[f * channels + 1];
        }
        out.samples[base + static_cast<size_t>(f) * 2 + 0] = left;
        out.samples[base + static_cast<size_t>(f) * 2 + 1] = right;
    }
}

std::optional<DecodedAudio> decodeWithSndfile(const QString& path, QString* errorOut)
{
    SF_INFO info {};
    SNDFILE* snd = sf_open(path.toLocal8Bit().constData(), SFM_READ, &info);
    if (!snd) {
        if (errorOut) *errorOut = QString::fromUtf8(sf_strerror(nullptr));
        return std::nullopt;
    }

    DecodedAudio out;
    out.sampleRate = info.samplerate;
    const int channels = info.channels;

    constexpr sf_count_t k_BlockFrames = 8192;
    std::vector<float> block(static_cast<size_t>(k_BlockFrames) * channels);
    sf_count_t read = 0;
    while ((read = sf_readf_float(snd, block.data(), k_BlockFrames)) > 0) {
        appendAsStereo(out, block.data(), read, channels);
    }
    sf_close(snd);

    if (out.isEmpty()) {
        if (errorOut) *errorOut = QStringLiteral("file has no audio samples");
        return std::nullopt;
    }
    qCDebug(lcDecoder) << "sndfile decoded" << out.frameCount() << "frames @" << out.sampleRate
                       << "Hz from" << path;
    return out;
}

std::optional<DecodedAudio> decodeWithMpg123(const QString& path, QString* errorOut)
{
    ensureMpg123Initialized();

    int err = MPG123_OK;
    mpg123_handle* mh = mpg123_new(nullptr, &err);
    if (!mh) {
        if (errorOut) *errorOut = QString::fromUtf8(mpg123_plain_strerror(err));
        return std::nullopt;
    }

    if (mpg123_open(mh, path.toLocal8Bit().constData()) != MPG123_OK) {
        if (errorOut) *errorOut = QString::fromUtf8(mpg123_strerror(mh));
        mpg123_delete(mh);
        return std::nullopt;
    }

    long rate = 0;
    int channels = 0;
    int encoding = 0;
    if (mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) {
        if (errorOut) *errorOut = QString::fromUtf8(mpg123_strerror(mh));
        mpg123_close(mh);
        mpg123_delete(mh);
        return std::nullopt;
    }

    mpg123_format_none(mh);
    if (mpg123_format(mh, rate, MPG123_STEREO, MPG123_ENC_SIGNED_16) != MPG123_OK) {
        if (errorOut) *errorOut = QString::fromUtf8(mpg123_strerror(mh));
        mpg123_close(mh);
        mpg123_delete(mh);
        return std::nullopt;
    }

    DecodedAudio out;
    out.sampleRate = static_cast<int>(rate);

    constexpr float k_Int16ToFloat = 1.0f / 32768.0f;
    const size_t blockBytes = mpg123_outblock(mh);
    std::vector<unsigned char> block(blockBytes);
    size_t done = 0;
    int ret = MPG123_OK;
    do {
        ret = mpg123_read(mh, block.data(), block.size(), &done);
        const size_t sampleCount = done / sizeof(std::int16_t);
        if (sampleCount > 0) {
            const size_t base = out.samples.size();
            out.samples.resize(base + sampleCount);
            std::int16_t pcm = 0;
            for (size_t i = 0; i < sampleCount; ++i) {
                std::memcpy(&pcm, block.data() + i * sizeof(std::int16_t), sizeof(std::int16_t));
                out.samples[base + i] = static_cast<float>(pcm) * k_Int16ToFloat;
            }
        }
    } while (ret == MPG123_OK);

    mpg123_close(mh);
    mpg123_delete(mh);

    if (ret != MPG123_DONE) {
        qCWarning(lcDecoder) << "mpg123 stopped early on" << path << "code" << ret;
    }
    if (out.isEmpty()) {
        if (errorOut) *errorOut = QStringLiteral("MP3 has no decodable samples");
        return std::nullopt;
    }
    qCDebug(lcDecoder) << "mpg123 decoded" << out.frameCount() << "frames @" << out.sampleRate
                       << "Hz from" << path;
    return out;
}

}

namespace AudioFileDecoder {

std::optional<DecodedAudio> decode(const QString& filePath, QString* errorOut)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix == QLatin1String("mp3")) {
        return decodeWithMpg123(filePath, errorOut);
    }
    return decodeWithSndfile(filePath, errorOut);
}

int durationMs(const QString& filePath)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix == QLatin1String("mp3")) {
        ensureMpg123Initialized();
        int err = MPG123_OK;
        mpg123_handle* mh = mpg123_new(nullptr, &err);
        if (!mh) return 0;
        int ms = 0;
        if (mpg123_open(mh, filePath.toLocal8Bit().constData()) == MPG123_OK) {
            long rate = 0;
            int channels = 0;
            int encoding = 0;
            mpg123_getformat(mh, &rate, &channels, &encoding);
            mpg123_scan(mh);
            const off_t frames = mpg123_length(mh);
            if (rate > 0 && frames > 0) {
                ms = static_cast<int>(frames * 1000 / rate);
            }
            mpg123_close(mh);
        }
        mpg123_delete(mh);
        return ms;
    }

    SF_INFO info {};
    SNDFILE* snd = sf_open(filePath.toLocal8Bit().constData(), SFM_READ, &info);
    if (!snd) return 0;
    const int ms =
        (info.samplerate > 0) ? static_cast<int>(info.frames * 1000 / info.samplerate) : 0;
    sf_close(snd);
    return ms;
}

}
