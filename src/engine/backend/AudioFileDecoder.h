// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

#include <QString>

#include <cstdint>
#include <optional>
#include <vector>

struct DecodedAudio
{
    std::vector<float> samples;
    int sampleRate { 0 };

    [[nodiscard]] std::int64_t frameCount() const
    {
        return static_cast<std::int64_t>(samples.size() / 2);
    }
    [[nodiscard]] bool isEmpty() const { return samples.empty(); }
};

namespace AudioFileDecoder {

[[nodiscard]] std::optional<DecodedAudio> decode(const QString& filePath,
                                                 QString* errorOut = nullptr);

[[nodiscard]] int durationMs(const QString& filePath);

}
