// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#pragma pack(push, 4)
/// This structure is used to pass the accessor options to libCZIAPI.
struct AccessorOptionsInterop
{
    /// The red component of the background color. If the destination bitmap is a grayscale-type, then the mean from R, G and B is calculated and multiplied
    /// with the maximum pixel value (of the specific pixeltype). If it is an RGB-color type, then R, G and B are separately multiplied with
    /// the maximum pixel value.
    /// If any of R, G or B is NaN, then the background is not cleared.
    float back_ground_color_r;

    /// The green component of the background color. If the destination bitmap is a grayscale-type, then the mean from R, G and B is calculated and multiplied
    /// with the maximum pixel value (of the specific pixeltype). If it is an RGB-color type, then R, G and B are separately multiplied with
    /// the maximum pixel value.
    /// If any of R, G or B is NaN, then the background is not cleared.
    float back_ground_color_g;

    /// The blue component of the background color. If the destination bitmap is a grayscale-type, then the mean from R, G and B is calculated and multiplied
    /// with the maximum pixel value (of the specific pixeltype). If it is an RGB-color type, then R, G and B are separately multiplied with
    /// the maximum pixel value.
    /// If any of R, G or B is NaN, then the background is not cleared.
    float back_ground_color_b;

    /// If true, then the tiles are sorted by their M-index (tile with highest M-index will be 'on top').
    /// Otherwise, the Z-order is arbitrary.
    bool sort_by_m;

    /// If true, then the tile-visibility-check-optimization is used. When doing the multi-tile composition,
    /// all relevant tiles are checked whether they are visible in the destination bitmap. If a tile is not visible, then
    /// the corresponding sub-block is not read. This can speed up the operation considerably. The result is the same as
    /// without this optimization - i.e. there should be no reason to turn it off besides potential bugs.
    bool use_visibility_check_optimization;

    /// Additional parameters for the accessor.
    const char* additional_parameters;
};

#pragma pack(pop)
