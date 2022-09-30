// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#if defined(_WIN32)

#include "libCZI_Pixels.h"
#include "libCZI_Site.h"

struct IWICBitmapDecoder;
struct IWICImagingFactory;

class CWicJpgxrDecoder : public libCZI::IDecoder
{
private:
		IWICImagingFactory* pFactory;
public:
		static std::shared_ptr<CWicJpgxrDecoder> Create();

		CWicJpgxrDecoder() = delete;
		CWicJpgxrDecoder(IWICImagingFactory* pFactory);
		virtual ~CWicJpgxrDecoder();

public:
		virtual std::shared_ptr<libCZI::IBitmapData> Decode(const void* ptrData, size_t size, libCZI::PixelType pixelType, std::uint32_t height, std::uint32_t width) override;
};

#endif
