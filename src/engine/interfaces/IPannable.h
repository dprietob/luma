// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#pragma once

class IPannable
{
public:
    virtual ~IPannable() = default;

    virtual void setPan(float pan) = 0;
};
