// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <libCZI_Config.h>

#if LIBCZI_WINDOWSAPI_AVAILABLE && LIBCZI_HAVE_WINCODECS_API

#include <memory>
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
    explicit CWicJpgxrDecoder(IWICImagingFactory* pFactory);
    virtual ~CWicJpgxrDecoder();

public:
    std::shared_ptr<libCZI::IBitmapData> Decode(const void* ptrData, size_t size, const libCZI::PixelType* pixelType, const std::uint32_t* height, const std::uint32_t* width, const char* additional_arguments) override;
};

#endif
