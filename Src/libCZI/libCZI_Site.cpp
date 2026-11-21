// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "libCZI.h"
#include "libCZI_Config_Internal.h"
#include "decoder.h"
#include "decoder_zstd.h"
#include <mutex>
#include <cstdlib>
#include "bitmapData.h"
#include "decoder_chunkedcompression.h"
#include "decoder_wic.h"
#include "Site.h"

using namespace libCZI;
using namespace libCZI::detail;
using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////

class CSiteImpBase : public ISite
{
public:
    bool IsEnabled(int logLevel) override
    {
        (void)logLevel;

        return false;
    }

    void Log(int logLevel, const char* szMsg) override
    {
        (void)logLevel;
        (void)szMsg;
    }

    std::shared_ptr<libCZI::IBitmapData> CreateBitmap(libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height, std::uint32_t stride, std::uint32_t extraRows, std::uint32_t extraColumns) override
    {
        return CStdBitmapData::Create(pixeltype, width, height, stride, extraRows, extraColumns);
    }

    std::shared_ptr<IDecoder> GetDecoder(ImageDecoderType type, const char* arguments) override
    {
        (void)type;
        (void)arguments;

        throw std::runtime_error("must not be called...");
    }

    void TerminateProgram(TerminationReason reason, const char* message) override
    {
        (void)reason;
        (void)message;

        abort();
    }
};

#if LIBCZI_WINDOWSAPI_AVAILABLE
class CSiteImpWic : public CSiteImpBase
{
private:
    std::once_flag  jpgxr_decoder_initialized_;
    std::shared_ptr<IDecoder> jpgxr_decoder_;
    std::once_flag  zstd0_decoder_initialized_;
    std::shared_ptr<IDecoder> zstd0_decoder_;
    std::once_flag  zstd1_decoder_initialized_;
    std::shared_ptr<IDecoder> zstd1_decoder_;
    std::once_flag  jpg_decoder_initialized_;
    std::shared_ptr<IDecoder> jpg_decoder_;
#if LIBCZI_EXPERIMENTAL_CHUNKED_COMPRESSION_AVAILABLE
    std::once_flag  chunked_compression_decoder_initialized_;
    std::shared_ptr<IDecoder> chunked_compression_decoder_;
#endif
public:
    std::shared_ptr<IDecoder> GetDecoder(ImageDecoderType type, const char* arguments) override
    {
        (void)arguments;

        switch (type)
        {
        case ImageDecoderType::Jpg:
            {
            std::call_once(this->jpg_decoder_initialized_,
                [this]()
                {
                    this->jpg_decoder_ = CWicJpgDecoder::Create();
                });

            return this->jpg_decoder_;
            }
        case ImageDecoderType::JPXR_JxrLib:
        {
            std::call_once(this->jpgxr_decoder_initialized_,
                [this]()
                {
                    this->jpgxr_decoder_ = CWicJpgxrDecoder::Create();
                });

            return this->jpgxr_decoder_;
        }
        case ImageDecoderType::ZStd0:
        {
            std::call_once(this->zstd0_decoder_initialized_,
                [this]()
                {
                    this->zstd0_decoder_ = CZstd0Decoder::Create();
                });

            return this->zstd0_decoder_;
        }
        case ImageDecoderType::ZStd1:
        {
            std::call_once(this->zstd1_decoder_initialized_,
                [this]()
                {
                    this->zstd1_decoder_ = CZstd1Decoder::Create();
                });

            return this->zstd1_decoder_;
        }
#if LIBCZI_EXPERIMENTAL_CHUNKED_COMPRESSION_AVAILABLE
        case ImageDecoderType::ChunkedCompression:
        {
            std::call_once(this->chunked_compression_decoder_initialized_,
                [this]()
                {
                    this->chunked_compression_decoder_ = CChunkedCompressionDecoder::Create();
                });

            return this->chunked_compression_decoder_;
        }
#endif
        }

        return shared_ptr<IDecoder>();
    }
};
#endif

class CSiteImpJxrLib : public CSiteImpBase
{
private:
    std::once_flag  jpgxr_decoder_initialized_;
    std::shared_ptr<IDecoder> jpgxr_decoder_;
    std::once_flag  jpg_decoder_initialized_;
    std::shared_ptr<IDecoder> jpg_decoder_;
    std::once_flag  zstd0_decoder_initialized_;
    std::shared_ptr<IDecoder> zstd0_decoder_;
    std::once_flag  zstd1_decoder_initialized_;
    std::shared_ptr<IDecoder> zstd1_decoder_;
#if LIBCZI_EXPERIMENTAL_CHUNKED_COMPRESSION_AVAILABLE
    std::once_flag  chunked_compression_decoder_initialized_;
    std::shared_ptr<IDecoder> chunked_compression_decoder_;
#endif
public:
    std::shared_ptr<IDecoder> GetDecoder(ImageDecoderType type, const char* arguments) override
    {
        (void)arguments;

        switch (type)
        {
        case ImageDecoderType::Jpg:
        {
            std::call_once(this->jpg_decoder_initialized_,
                [this]()
                {
                    this->jpg_decoder_ = JpgDecoder::Create();
                });

            return this->jpg_decoder_;
        }
        case ImageDecoderType::JPXR_JxrLib:
        {
            std::call_once(this->jpgxr_decoder_initialized_,
                [this]()
                {
                    this->jpgxr_decoder_ = CJxrLibDecoder::Create();
                });

            return this->jpgxr_decoder_;
        }
        case ImageDecoderType::ZStd0:
        {
            std::call_once(this->zstd0_decoder_initialized_,
                [this]()
                {
                    this->zstd0_decoder_ = CZstd0Decoder::Create();
                });

            return this->zstd0_decoder_;
        }
        case ImageDecoderType::ZStd1:
        {
            std::call_once(this->zstd1_decoder_initialized_,
                [this]()
                {
                    this->zstd1_decoder_ = CZstd1Decoder::Create();
                });

            return this->zstd1_decoder_;
        }
#if LIBCZI_EXPERIMENTAL_CHUNKED_COMPRESSION_AVAILABLE
        case ImageDecoderType::ChunkedCompression:
        {
            std::call_once(this->chunked_compression_decoder_initialized_,
                [this]()
                {
                    this->chunked_compression_decoder_ = CChunkedCompressionDecoder::Create();
                });

            return this->chunked_compression_decoder_;
        }
#endif
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

libCZI::ISite* libCZI::detail::GetSite()
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
