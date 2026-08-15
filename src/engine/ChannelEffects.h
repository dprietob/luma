// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QStringList>
#include <QVariantList>

class IAudioBackend;

class ChannelEffects final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool reverbEnabled READ reverbEnabled WRITE setReverbEnabled NOTIFY reverbChanged FINAL)
    Q_PROPERTY(float reverbRoomSize READ reverbRoomSize WRITE setReverbRoomSize NOTIFY reverbChanged FINAL)
    Q_PROPERTY(float reverbDamping READ reverbDamping WRITE setReverbDamping NOTIFY reverbChanged FINAL)
    Q_PROPERTY(float reverbMix READ reverbMix WRITE setReverbMix NOTIFY reverbChanged FINAL)

    Q_PROPERTY(bool delayEnabled READ delayEnabled WRITE setDelayEnabled NOTIFY delayChanged FINAL)
    Q_PROPERTY(float delayTime READ delayTime WRITE setDelayTime NOTIFY delayChanged FINAL)
    Q_PROPERTY(float delayFeedback READ delayFeedback WRITE setDelayFeedback NOTIFY delayChanged FINAL)
    Q_PROPERTY(float delayMix READ delayMix WRITE setDelayMix NOTIFY delayChanged FINAL)

    Q_PROPERTY(bool distortionEnabled READ distortionEnabled WRITE setDistortionEnabled NOTIFY distortionChanged FINAL)
    Q_PROPERTY(float distortionDrive READ distortionDrive WRITE setDistortionDrive NOTIFY distortionChanged FINAL)
    Q_PROPERTY(float distortionMix READ distortionMix WRITE setDistortionMix NOTIFY distortionChanged FINAL)

    Q_PROPERTY(bool pitchEnabled READ pitchEnabled WRITE setPitchEnabled NOTIFY pitchChanged FINAL)
    Q_PROPERTY(float pitchSemitones READ pitchSemitones WRITE setPitchSemitones NOTIFY pitchChanged FINAL)

    Q_PROPERTY(bool speedEnabled READ speedEnabled WRITE setSpeedEnabled NOTIFY speedChanged FINAL)
    Q_PROPERTY(float speedTempo READ speedTempo WRITE setSpeedTempo NOTIFY speedChanged FINAL)

    Q_PROPERTY(bool eqEnabled READ eqEnabled WRITE setEqEnabled NOTIFY eqChanged FINAL)
    Q_PROPERTY(QVariantList eqBands READ eqBands NOTIFY eqChanged FINAL)
    Q_PROPERTY(QStringList eqBandLabels READ eqBandLabels CONSTANT FINAL)
    Q_PROPERTY(float eqGainMax READ eqGainMax CONSTANT FINAL)

    Q_PROPERTY(bool anyEnabled READ anyEnabled NOTIFY anyEnabledChanged FINAL)

public:
    explicit ChannelEffects(IAudioBackend* backend, QObject* parent = nullptr);

    [[nodiscard]] bool reverbEnabled() const;
    [[nodiscard]] float reverbRoomSize() const;
    [[nodiscard]] float reverbDamping() const;
    [[nodiscard]] float reverbMix() const;
    [[nodiscard]] bool delayEnabled() const;
    [[nodiscard]] float delayTime() const;
    [[nodiscard]] float delayFeedback() const;
    [[nodiscard]] float delayMix() const;
    [[nodiscard]] bool distortionEnabled() const;
    [[nodiscard]] float distortionDrive() const;
    [[nodiscard]] float distortionMix() const;
    [[nodiscard]] bool pitchEnabled() const;
    [[nodiscard]] float pitchSemitones() const;
    [[nodiscard]] bool speedEnabled() const;
    [[nodiscard]] float speedTempo() const;
    [[nodiscard]] bool eqEnabled() const;
    [[nodiscard]] QVariantList eqBands() const;
    [[nodiscard]] QStringList eqBandLabels() const;
    [[nodiscard]] float eqGainMax() const;
    [[nodiscard]] bool anyEnabled() const;

    void setReverbEnabled(bool enabled);
    void setReverbRoomSize(float value);
    void setReverbDamping(float value);
    void setReverbMix(float value);
    void setDelayEnabled(bool enabled);
    void setDelayTime(float value);
    void setDelayFeedback(float value);
    void setDelayMix(float value);
    void setDistortionEnabled(bool enabled);
    void setDistortionDrive(float value);
    void setDistortionMix(float value);
    void setPitchEnabled(bool enabled);
    void setPitchSemitones(float value);
    void setSpeedEnabled(bool enabled);
    void setSpeedTempo(float value);
    void setEqEnabled(bool enabled);
    Q_INVOKABLE void setEqBandGain(int band, float db);

    Q_INVOKABLE void resetReverb();
    Q_INVOKABLE void resetDelay();
    Q_INVOKABLE void resetDistortion();
    Q_INVOKABLE void resetPitch();
    Q_INVOKABLE void resetSpeed();
    Q_INVOKABLE void resetEq();
    void resetAll();

    void setVoiceId(int voiceId);
    void reapply() const;

    [[nodiscard]] QJsonObject toJson() const;
    void applyJson(const QJsonObject& o);

signals:
    void reverbChanged();
    void delayChanged();
    void distortionChanged();
    void pitchChanged();
    void speedChanged();
    void eqChanged();
    void anyEnabledChanged();

private:
    void pushReverb() const;
    void pushDelay() const;
    void pushDistortion() const;
    void pushPitch() const;
    void pushSpeed() const;
    void pushEq() const;

    IAudioBackend* m_backend;
    int m_voiceId;

    bool m_reverbEnabled;
    float m_reverbRoomSize;
    float m_reverbDamping;
    float m_reverbMix;

    bool m_delayEnabled;
    float m_delayTime;
    float m_delayFeedback;
    float m_delayMix;

    bool m_distortionEnabled;
    float m_distortionDrive;
    float m_distortionMix;

    bool m_pitchEnabled;
    float m_pitchSemitones;

    bool m_speedEnabled;
    float m_speedTempo;

    bool m_eqEnabled;
    QList<float> m_eqGains;
};
