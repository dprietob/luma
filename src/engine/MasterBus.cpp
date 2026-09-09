// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "MasterBus.h"

#include "interfaces/IAudioBackend.h"

#include <QtGlobal>

#include <algorithm>

MasterBus::MasterBus(IAudioBackend* backend, QObject* parent)
    : QObject(parent)
    , m_backend(backend)
{
    Q_ASSERT_X(m_backend != nullptr, "MasterBus", "backend must be injected");
    m_backend->setChannelMasterVolume(m_channelMasterVolume);
}

float MasterBus::channelMasterVolume() const { return m_channelMasterVolume; }
float MasterBus::masterVuLeft() const { return m_masterVuLeft; }
float MasterBus::masterVuRight() const { return m_masterVuRight; }

void MasterBus::setChannelMasterVolume(float volume)
{
    const float clamped = std::clamp(volume, 0.0f, 1.0f);
    if (qFuzzyCompare(m_channelMasterVolume, clamped)) return;
    m_channelMasterVolume = clamped;
    m_backend->setChannelMasterVolume(m_channelMasterVolume);
    emit channelMasterVolumeChanged();
}

void MasterBus::updateMasterVu(float left, float right)
{
    const float l = std::clamp(left, 0.0f, 1.0f);
    const float r = std::clamp(right, 0.0f, 1.0f);
    if (qFuzzyCompare(m_masterVuLeft, l) && qFuzzyCompare(m_masterVuRight, r)) return;
    m_masterVuLeft = l;
    m_masterVuRight = r;
    emit masterVuChanged();
}
