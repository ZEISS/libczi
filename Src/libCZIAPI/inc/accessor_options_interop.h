// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#pragma pack(push, 4)
struct AccessorOptionsInterop
{
    float back_ground_color_r;
    float back_ground_color_g;
    float back_ground_color_b;

    bool sort_by_m;
    bool use_visibility_check_optimization;

    const char* additional_parameters;
};

#pragma pack(pop)
