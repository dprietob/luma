// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "AudioDeviceManager.h"

#include "interfaces/IAudioBackend.h"

#include <QVariantMap>

AudioDeviceManager::AudioDeviceManager(IAudioBackend* backend, QObject* parent)
    : QObject(parent)
    , m_backend(backend)
{
    Q_ASSERT_X(m_backend != nullptr, "AudioDeviceManager", "backend must be injected");
}

QVariantList AudioDeviceManager::outputDevices() const { return m_outputDevices; }
QString AudioDeviceManager::mainOutput() const { return m_mainOutput; }
QString AudioDeviceManager::auxOutput() const { return m_auxOutput; }

void AudioDeviceManager::setMainOutput(const QString& name)
{
    if (m_mainOutput == name) return;
    m_mainOutput = name;
    applyToBackend();
    emit mainOutputChanged();
}

void AudioDeviceManager::setAuxOutput(const QString& name)
{
    if (m_auxOutput == name) return;
    m_auxOutput = name;
    applyToBackend();
    emit auxOutputChanged();
}

void AudioDeviceManager::refresh()
{
    m_backend->refreshDevices();

    m_outputDevices.clear();
    const QList<AudioDeviceInfo> devices = m_backend->outputDevices();
    for (const AudioDeviceInfo& d : devices) {
        QVariantMap entry;
        entry[QStringLiteral("index")] = d.index;
        entry[QStringLiteral("name")] = d.name;
        m_outputDevices.append(entry);
    }
    applyToBackend();
    emit outputDevicesChanged();
}

void AudioDeviceManager::applySaved(const QString& mainName, const QString& auxName)
{
    refresh();

    if (hasDevice(mainName) && hasDevice(auxName)) {
        setMainOutput(mainName);
        setAuxOutput(auxName);
    } else {
        setMainOutput(QString());
        setAuxOutput(QString());
    }
}

void AudioDeviceManager::applyToBackend() const
{
    m_backend->setOutputDevices(indexForName(m_mainOutput), indexForName(m_auxOutput));
}

int AudioDeviceManager::indexForName(const QString& name) const
{
    if (name.isEmpty()) return -1;
    for (const QVariant& v : m_outputDevices) {
        const QVariantMap entry = v.toMap();
        if (entry.value(QStringLiteral("name")).toString() == name)
            return entry.value(QStringLiteral("index")).toInt();
    }
    return -1;
}

bool AudioDeviceManager::hasDevice(const QString& name) const
{
    if (name.isEmpty()) return true;
    for (const QVariant& v : m_outputDevices) {
        if (v.toMap().value(QStringLiteral("name")).toString() == name) return true;
    }
    return false;
}
