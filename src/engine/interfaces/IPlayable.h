// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

class IPlayable
{
public:
    virtual ~IPlayable() = default;

    virtual void play() = 0;
    virtual void stop() = 0;
    virtual void setVolume(float volume) = 0;

    [[nodiscard]] virtual bool isPlaying() const = 0;
};
