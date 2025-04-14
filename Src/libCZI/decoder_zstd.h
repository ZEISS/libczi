// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <memory>
#include "libCZI_Pixels.h"
#include "libCZI_Site.h"

class CZstd0Decoder : public libCZI::IDecoder
{
public:
    static const char* kOption_handle_data_size_mismatch;
    static std::shared_ptr<CZstd0Decoder> Create();

    /// Passing in a block of zstd0-compressed data, decode the image and return a bitmap object.
    /// This decoder requires that pixelType, width and height are passed in, the parameters must not be nullptr.
    /// The additional_arguments parameter is a semicolon-separated list of items, where the string 'handle_data_size_mismatch'
    /// currently is the only valid option. If this option is set, the decoder will not throw an exception if the size of
    /// the compressed data does not match the size of the bitmap described by the arguments specified. Instead, the resolution
    /// protocol is applied, which means that the bitmap is cropped or padded to the size described by the arguments. 
    /// 
    /// \param ptrData              Pointer to a block of memory (which contains the zstd0-compressed data).
    /// \param size                 The size of the memory block pointed by `ptrData`.
    /// \param pixelType            If non-null, the pixel type of the expected bitmap.
    /// \param width                If non-null, the width of the expected bitmap.
    /// \param height               If non-null, the height of the expected bitmap.
    /// \param additional_arguments If non-null, additional arguments for the decoder.
    ///
    /// \return A bitmap object with the decoded data.
    std::shared_ptr<libCZI::IBitmapData> Decode(const void* ptrData, size_t size, const libCZI::PixelType* pixelType, const std::uint32_t* width, const std::uint32_t* height, const char* additional_arguments) override;

    std::shared_ptr<libCZI::IBitmapData> Decode(const void* ptrData, size_t size, libCZI::PixelType pixelType, std::uint32_t width, std::uint32_t height, const char* additional_arguments = nullptr)
    {
        return this->Decode(ptrData, size, &pixelType, &width, &height, additional_arguments);
    }
};

class CZstd1Decoder : public libCZI::IDecoder
{
public:
    static const char* kOption_handle_data_size_mismatch;
    static std::shared_ptr<CZstd1Decoder> Create();
    
    /// Passing in a block of zstd1-compressed data, decode the image and return a bitmap object.
    /// This decoder requires that pixelType, width and height are passed in, the parameters must not be nullptr.
    /// The additional_arguments parameter is a semicolon-separated list of items, where the string 'handle_data_size_mismatch'
    /// currently is the only valid option. If this option is set, the decoder will not throw an exception if the size of
    /// the compressed data does not match the size of the bitmap described by the arguments specified. Instead, the resolution
    /// protocol is applied, which means that the bitmap is cropped or padded to the size described by the arguments. 
    /// 
    /// \param ptrData              Pointer to a block of memory (which contains the zstd1-compressed data).
    /// \param size                 The size of the memory block pointed by `ptrData`.
    /// \param pixelType            If non-null, the pixel type of the expected bitmap.
    /// \param width                If non-null, the width of the expected bitmap.
    /// \param height               If non-null, the height of the expected bitmap.
    /// \param additional_arguments If non-null, additional arguments for the decoder.
    ///
    /// \return A bitmap object with the decoded data.
    std::shared_ptr<libCZI::IBitmapData> Decode(const void* ptrData, size_t size, const libCZI::PixelType* pixelType, const std::uint32_t* width, const std::uint32_t* height, const char* additional_arguments) override;

    std::shared_ptr<libCZI::IBitmapData> Decode(const void* ptrData, size_t size, libCZI::PixelType pixelType, std::uint32_t width, std::uint32_t height, const char* additional_arguments = nullptr)
    {
        return this->Decode(ptrData, size, &pixelType, &width, &height, additional_arguments);
    }
};
