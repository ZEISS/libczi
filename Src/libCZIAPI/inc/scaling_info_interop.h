// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#pragma pack(push, 4)

/// This structure gathers the information about the scaling.
struct ScalingInfoInterop
{
    double scale_x; ///< The length of a pixel in x-direction in the unit meters. If unknown/invalid, this value is numeric_limits<double>::quiet_NaN().
    double scale_y; ///< The length of a pixel in y-direction in the unit meters. If unknown/invalid, this value is numeric_limits<double>::quiet_NaN().
    double scale_z; ///< The length of a pixel in z-direction in the unit meters. If unknown/invalid, this value is numeric_limits<double>::quiet_NaN().
};

#pragma pack(pop)
