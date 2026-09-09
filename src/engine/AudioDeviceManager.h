// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

class IAudioBackend;

class AudioDeviceManager final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList outputDevices READ outputDevices NOTIFY outputDevicesChanged FINAL)
    Q_PROPERTY(QString mainOutput READ mainOutput WRITE setMainOutput NOTIFY mainOutputChanged FINAL)
    Q_PROPERTY(QString auxOutput READ auxOutput WRITE setAuxOutput NOTIFY auxOutputChanged FINAL)

public:
    explicit AudioDeviceManager(IAudioBackend* backend, QObject* parent = nullptr);

    [[nodiscard]] QVariantList outputDevices() const;
    [[nodiscard]] QString mainOutput() const;
    [[nodiscard]] QString auxOutput() const;

    void setMainOutput(const QString& name);
    void setAuxOutput(const QString& name);

    Q_INVOKABLE void refresh();

    void applySaved(const QString& mainName, const QString& auxName);

signals:
    void outputDevicesChanged();
    void mainOutputChanged();
    void auxOutputChanged();

private:
    void applyToBackend() const;
    [[nodiscard]] int indexForName(const QString& name) const;
    [[nodiscard]] bool hasDevice(const QString& name) const;

    IAudioBackend* m_backend;
    QVariantList m_outputDevices;
    QString m_mainOutput;
    QString m_auxOutput;
};
