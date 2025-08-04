// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "libCZI.h"
#include "inc_libCZI_Config.h"
#include "decoder.h"
#include "decoder_zstd.h"
#include <mutex>
#include <cstdlib>
#include "bitmapData.h"
#include "decoder_wic.h"

using namespace libCZI;
using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////

class CSiteImpBase : public ISite
{
public:
    bool IsEnabled(int logLevel) override
    {
        return false;
    }

    void Log(int level, const char* szMsg) override
    {
    }

    std::shared_ptr<libCZI::IBitmapData> CreateBitmap(libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height, std::uint32_t stride, std::uint32_t extraRows, std::uint32_t extraColumns) override
    {
        return CStdBitmapData::Create(pixeltype, width, height, stride, extraRows, extraColumns);
    }

    std::shared_ptr<IDecoder> GetDecoder(ImageDecoderType type, const char* arguments) override
    {
        throw std::runtime_error("must not be called...");
    }

    void TerminateProgram(TerminationReason reason, const char* message) override
    {
        abort();
    }
};

#if LIBCZI_WINDOWSAPI_AVAILABLE
class CSiteImpWic : public CSiteImpBase
{
private:
    std::once_flag  jxrDecoderInitialized;
    std::shared_ptr<IDecoder> jpgXrdecoder;
    std::once_flag  zstd0DecoderInitialized;
    std::shared_ptr<IDecoder> zstd0decoder;
    std::once_flag  zstd1DecoderInitialized;
    std::shared_ptr<IDecoder> zstd1decoder;
public:
    std::shared_ptr<IDecoder> GetDecoder(ImageDecoderType type, const char* arguments) override
    {
        switch (type)
        {
#if LIBCZI_HAVE_WINCODECS_API
        case ImageDecoderType::JPXR_JxrLib:
        {
            std::call_once(jxrDecoderInitialized,
                [this]()
                {
                    this->jpgXrdecoder = CWicJpgxrDecoder::Create();
                });

            return this->jpgXrdecoder;
        }
#endif // LIBCZI_HAVE_WINCODECS_API
        case ImageDecoderType::ZStd0:
        {
            std::call_once(zstd0DecoderInitialized,
                [this]()
                {
                    this->zstd0decoder = CZstd0Decoder::Create();
                });

            return this->zstd0decoder;
        }
        case ImageDecoderType::ZStd1:
        {
            std::call_once(zstd1DecoderInitialized,
                [this]()
                {
                    this->zstd1decoder = CZstd1Decoder::Create();
                });

            return this->zstd1decoder;
        }
        }

        return shared_ptr<IDecoder>();
    }
};
#endif

class CSiteImpJxrLib : public CSiteImpBase
{
private:
    std::once_flag  jxrDecoderInitialized;
    std::shared_ptr<IDecoder> jpgXrdecoder;
    std::once_flag  zstd0DecoderInitialized;
    std::shared_ptr<IDecoder> zstd0decoder;
    std::once_flag  zstd1DecoderInitialized;
    std::shared_ptr<IDecoder> zstd1decoder;
public:
    std::shared_ptr<IDecoder> GetDecoder(ImageDecoderType type, const char* arguments) override
    {
        switch (type)
        {
        case ImageDecoderType::JPXR_JxrLib:
        {
            std::call_once(jxrDecoderInitialized,
                [this]()
                {
                    this->jpgXrdecoder = CJxrLibDecoder::Create();
                });

            return this->jpgXrdecoder;
        }
        case ImageDecoderType::ZStd0:
        {
            std::call_once(zstd0DecoderInitialized,
                [this]()
                {
                    this->zstd0decoder = CZstd0Decoder::Create();
                });

            return this->zstd0decoder;
        }
        case ImageDecoderType::ZStd1:
        {
            std::call_once(zstd1DecoderInitialized,
                [this]()
                {
                    this->zstd1decoder = CZstd1Decoder::Create();
                });

            return this->zstd1decoder;
        }
        }

        return shared_ptr<IDecoder>();
    }
};

#if LIBCZI_WINDOWSAPI_AVAILABLE
static CSiteImpWic theWicSite;
#endif
static CSiteImpJxrLib theJxrLibSite;


///////////////////////////////////////////////////////////////////////////////////////////

static std::once_flag gSite_init;
static ISite* g_site = nullptr;

libCZI::ISite* GetSite()
{
    std::call_once(gSite_init,
        []()
        {
            if (g_site == nullptr)
            {
                g_site = libCZI::GetDefaultSiteObject(SiteObjectType::Default);
            }
        });

    return g_site;
}

libCZI::ISite* libCZI::GetDefaultSiteObject(SiteObjectType type)
{
    switch (type)
    {
#if LIBCZI_WINDOWSAPI_AVAILABLE
    case SiteObjectType::WithWICDecoder:
        return &theWicSite;
#endif
    case SiteObjectType::WithJxrDecoder:
    case SiteObjectType::Default:
        return &theJxrLibSite;
    default:
        return nullptr;
    }
}

void libCZI::SetSiteObject(libCZI::ISite* pSite)
{
    if (g_site != nullptr)
    {
        throw std::logic_error("Site was already initialized");
    }

    g_site = pSite;
}
