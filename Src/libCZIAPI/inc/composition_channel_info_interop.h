// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#pragma pack(push, 4)

/// This structure gathers all information about a channel for the purpose of multi-channel-composition.
struct CompositionChannelInfoInterop
{
    float weight; ///< The weight of the channel in the composition.

    /// True if tinting is enabled for this channel (in which case the tinting member is to be 
    /// examined), false if no tinting is to be applied (the tinting member is then not used).
    std::uint8_t enable_tinting;

    /// The tinting color (only examined if enableTinting is true). This is the red-component.
    std::uint8_t tinting_color_r;

    /// The tinting color (only examined if enableTinting is true). This is the green-component.
    std::uint8_t tinting_color_g;

    /// The tinting color (only examined if enableTinting is true). This is the blue-component.
    std::uint8_t tinting_color_b;

    /// The black point - it is a float between 0 and 1, where 0 corresponds to the lowest pixel value
    /// (of the pixel-type for the channel) and 1 to the highest pixel value (of the pixel-type of this channel).
    /// All pixel values below the black point are mapped to 0.
    float black_point;

    /// The white point - it is a float between 0 and 1, where 0 corresponds to the lowest pixel value
    /// (of the pixel-type for the channel) and 1 to the highest pixel value (of the pixel-type of this channel).
    /// All pixel value above the white pointer are mapped to the highest pixel value.
    float white_point;

    /// Number of elements in the look-up table. If 0, then the look-up table
    /// is not used. If this channelInfo applies to a Gray8/Bgr24-channel, then the size
    /// of the look-up table must be 256. In case of a Gray16/Bgr48-channel, the size must be
    /// 65536.
    /// \remark
    /// If a look-up table is provided, then \c blackPoint and \c whitePoint are not used anymore .
    int look_up_table_element_count;

    /// The pointer to the look-up table. If the property 'look_up_table_element_count' is <> 0, then this pointer
    /// must be valid.
    const std::uint8_t* ptr_look_up_table;
};

#pragma pack(pop)
