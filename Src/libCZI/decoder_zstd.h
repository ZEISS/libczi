// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "libCZI_Pixels.h"
#include "libCZI_Site.h"

class CZstd0Decoder : public libCZI::IDecoder
{
public:
		static std::shared_ptr<CZstd0Decoder> Create();
		std::shared_ptr<libCZI::IBitmapData> Decode(const void* ptrData, size_t size, libCZI::PixelType pixelType, std::uint32_t width, std::uint32_t height) override;
};

class CZstd1Decoder : public libCZI::IDecoder
{
public:
		static std::shared_ptr<CZstd1Decoder> Create();
		std::shared_ptr<libCZI::IBitmapData> Decode(const void* ptrData, size_t size, libCZI::PixelType pixelType, std::uint32_t width, std::uint32_t height) override;
};