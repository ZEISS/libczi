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
    std::shared_ptr<libCZI::IBitmapData> Decode(const void* ptrData, size_t size, const libCZI::PixelType* pixelType, const std::uint32_t* width, const std::uint32_t* height, const char* additional_arguments) override;
};

class CZstd1Decoder : public libCZI::IDecoder
{
public:
    static const char* kOption_handle_data_size_mismatch;
    static std::shared_ptr<CZstd1Decoder> Create();
    std::shared_ptr<libCZI::IBitmapData> Decode(const void* ptrData, size_t size, const libCZI::PixelType* pixelType, const std::uint32_t* width, const std::uint32_t* height, const char* additional_arguments) override;
};
