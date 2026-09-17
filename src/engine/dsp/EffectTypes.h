// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

namespace fx {

inline constexpr bool k_ReverbEnabled = false;
inline constexpr float k_ReverbRoomSize = 0.5f;
inline constexpr float k_ReverbDamping = 0.5f;
inline constexpr float k_ReverbMix = 0.3f;

inline constexpr bool k_DelayEnabled = false;
inline constexpr float k_DelayTimeMs = 300.0f;
inline constexpr float k_DelayTimeMaxMs = 1000.0f;
inline constexpr float k_DelayFeedback = 0.35f;
inline constexpr float k_DelayFeedbackMax = 0.95f;
inline constexpr float k_DelayMix = 0.3f;

inline constexpr bool k_DistortionEnabled = false;
inline constexpr float k_DistortionDrive = 5.0f;
inline constexpr float k_DistortionDriveMin = 1.0f;
inline constexpr float k_DistortionDriveMax = 50.0f;
inline constexpr float k_DistortionMix = 0.5f;

inline constexpr bool k_PitchEnabled = false;
inline constexpr float k_PitchSemitones = 0.0f;
inline constexpr float k_PitchSemitonesMax = 12.0f;

inline constexpr bool k_SpeedEnabled = false;
inline constexpr float k_SpeedTempo = 1.0f;
inline constexpr float k_SpeedTempoMin = 0.5f;
inline constexpr float k_SpeedTempoMax = 2.0f;

inline constexpr bool k_EqEnabled = false;
inline constexpr float k_EqBandGain = 0.0f;
inline constexpr float k_EqGainMaxDb = 12.0f;
inline constexpr int k_EqBandCount = 10;
inline constexpr float k_EqBandQ = 1.414f;
inline constexpr float k_EqBandFreq[k_EqBandCount] = {
    32.0f, 64.0f, 125.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f
};

}
