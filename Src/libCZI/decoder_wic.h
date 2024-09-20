// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <libCZI_Config.h>

#if LIBCZI_WINDOWSAPI_AVAILABLE

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
    std::shared_ptr<libCZI::IBitmapData> Decode(const void* ptrData, size_t size, libCZI::PixelType pixelType, std::uint32_t height, std::uint32_t width) override;
};

#endif
