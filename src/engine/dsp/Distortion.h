// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

class Distortion
{
public:
    void setParams(float drive, float mix);
    void process(float* interleavedStereo, int frames);

private:
    float m_drive { 1.0f };
    float m_mix { 0.0f };
};
