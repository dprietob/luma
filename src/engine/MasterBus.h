// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

#include <QObject>

class IAudioBackend;

class MasterBus final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(float channelMasterVolume READ channelMasterVolume WRITE setChannelMasterVolume NOTIFY channelMasterVolumeChanged FINAL)
    Q_PROPERTY(float masterVuLeft READ masterVuLeft NOTIFY masterVuChanged FINAL)
    Q_PROPERTY(float masterVuRight READ masterVuRight NOTIFY masterVuChanged FINAL)

public:
    explicit MasterBus(IAudioBackend* backend, QObject* parent = nullptr);
    ~MasterBus() override = default;

    [[nodiscard]] float channelMasterVolume() const;
    [[nodiscard]] float masterVuLeft() const;
    [[nodiscard]] float masterVuRight() const;

    void setChannelMasterVolume(float volume);
    void updateMasterVu(float left, float right);

signals:
    void channelMasterVolumeChanged();
    void masterVuChanged();

private:
    IAudioBackend* m_backend;

    float m_channelMasterVolume { 0.85f };
    float m_masterVuLeft { 0.0f };
    float m_masterVuRight { 0.0f };
};
