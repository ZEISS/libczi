// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "decoder_wic.h"

#if LIBCZI_WINDOWSAPI_AVAILABLE && LIBCZI_HAVE_WINCODECS_API
#include "BitmapOperations.h"
#include <wincodec.h>
#include <sstream>
#include <iomanip>

#include "Site.h"

using namespace std;
using namespace libCZI;

struct COMDeleter
{
    template <typename T>
    void operator()(T* ptr)
    {
        if (ptr)
        {
            ptr->Release();
        }
    }
};

static void ThrowIfFailed(const char* function, HRESULT hr)
{
    if (FAILED(hr))
    {
        ostringstream string_stream;
        string_stream << "COM-ERROR hr=0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << hr << " (" << function << ")";
        throw std::runtime_error(string_stream.str());
    }
}

/*static*/std::shared_ptr<CWicJpgxrDecoder> CWicJpgxrDecoder::Create()
{
    IWICImagingFactory* wicImagingFactor;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, reinterpret_cast<LPVOID*>(&wicImagingFactor));
    if (hr == CO_E_NOTINITIALIZED)
    {
        // that's for sure a bit controversial (that we call this COM-initialization here, you better make sure
        // that the current thread is already initialized by the caller
        CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
        hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, reinterpret_cast<LPVOID*>(&wicImagingFactor));
    }

    ThrowIfFailed("Creating WICImageFactory", hr);
    return make_shared<CWicJpgxrDecoder>(wicImagingFactor);
}

CWicJpgxrDecoder::CWicJpgxrDecoder(IWICImagingFactory* pFactory)
{
    this->pFactory = pFactory;
    this->pFactory->AddRef();
}

CWicJpgxrDecoder::~CWicJpgxrDecoder()
{
    // DO NOT release the thing here because very likely we have already called CoUninitialize
    // before getting here, and then this call will crash
    //this->pFactory->Release();
}

static const char* GetInformativeString(const WICPixelFormatGUID& wicPxlFmt)
{
    static const struct
    {
        GUID wicPxlFmt;
        const char* szText;
    } WicPxlFmtAndName[] =
    {
        { GUID_WICPixelFormatBlackWhite ,"WICPixelFormatBlackWhite" },
        { GUID_WICPixelFormat8bppGray ,"WICPixelFormat8bppGray" },
        { GUID_WICPixelFormat16bppBGR555 ,"WICPixelFormat16bppBGR555" },
        { GUID_WICPixelFormat16bppGray ,"WICPixelFormat16bppGray" },
        { GUID_WICPixelFormat24bppBGR ,"WICPixelFormat24bppBGR" },
        { GUID_WICPixelFormat24bppRGB ,"WICPixelFormat24bppRGB" },
        { GUID_WICPixelFormat32bppBGR ,"WICPixelFormat32bppBGR" },
        { GUID_WICPixelFormat32bppBGRA ,"WICPixelFormat32bppBGRA" },
        { GUID_WICPixelFormat48bppRGBFixedPoint ,"WICPixelFormat48bppRGBFixedPoint" },
        { GUID_WICPixelFormat16bppGrayFixedPoint ,"WICPixelFormat16bppGrayFixedPoint" },
        { GUID_WICPixelFormat32bppBGR101010  ,"WICPixelFormat32bppBGR101010" },
        { GUID_WICPixelFormat48bppRGB,"WICPixelFormat48bppRGB" },
        { GUID_WICPixelFormat64bppRGBA,"WICPixelFormat64bppRGBA" },
        { GUID_WICPixelFormat96bppRGBFixedPoint,"WICPixelFormat96bppRGBFixedPoint" },
        { GUID_WICPixelFormat96bppRGBFixedPoint,"WICPixelFormat96bppRGBFixedPoint" },
        { GUID_WICPixelFormat128bppRGBFloat,"WICPixelFormat128bppRGBFloat" },
        { GUID_WICPixelFormat32bppCMYK,"WICPixelFormat32bppCMYK" },
        { GUID_WICPixelFormat64bppRGBAFixedPoint,"WICPixelFormat64bppRGBAFixedPoint" },
        { GUID_WICPixelFormat128bppRGBAFixedPoint,"WICPixelFormat128bppRGBAFixedPoint" },
        { GUID_WICPixelFormat64bppCMYK,"WICPixelFormat64bppCMYK" },
        { GUID_WICPixelFormat24bpp3Channels,"WICPixelFormat24bpp3Channels" },
        { GUID_WICPixelFormat32bpp4Channels,"WICPixelFormat32bpp4Channels" },
        { GUID_WICPixelFormat40bpp5Channels,"WICPixelFormat40bpp5Channels" },
        { GUID_WICPixelFormat48bpp6Channels,"WICPixelFormat48bpp6Channels" },
        { GUID_WICPixelFormat56bpp7Channels,"WICPixelFormat56bpp7Channels" },
        { GUID_WICPixelFormat64bpp8Channels,"WICPixelFormat64bpp8Channels" },
        { GUID_WICPixelFormat48bpp3Channels,"WICPixelFormat48bpp3Channels" },
        { GUID_WICPixelFormat64bpp4Channels,"WICPixelFormat64bpp4Channels" },
        { GUID_WICPixelFormat80bpp5Channels,"WICPixelFormat80bpp5Channels" },
        { GUID_WICPixelFormat96bpp6Channels,"WICPixelFormat96bpp6Channels" },
        { GUID_WICPixelFormat112bpp7Channels,"WICPixelFormat112bpp7Channels" },
        { GUID_WICPixelFormat128bpp8Channels,"WICPixelFormat128bpp8Channels" },
        { GUID_WICPixelFormat40bppCMYKAlpha,"WICPixelFormat40bppCMYKAlpha" },
        { GUID_WICPixelFormat80bppCMYKAlpha,"WICPixelFormat80bppCMYKAlpha" },
        { GUID_WICPixelFormat32bpp3ChannelsAlpha,"WICPixelFormat32bpp3ChannelsAlpha" },
        { GUID_WICPixelFormat64bpp7ChannelsAlpha,"WICPixelFormat64bpp7ChannelsAlpha" },
        { GUID_WICPixelFormat72bpp8ChannelsAlpha,"WICPixelFormat72bpp8ChannelsAlpha" },
        { GUID_WICPixelFormat64bpp3ChannelsAlpha,"WICPixelFormat64bpp3ChannelsAlpha" },
        { GUID_WICPixelFormat80bpp4ChannelsAlpha,"WICPixelFormat80bpp4ChannelsAlpha" },
        { GUID_WICPixelFormat96bpp5ChannelsAlpha,"WICPixelFormat96bpp5ChannelsAlpha" },
        { GUID_WICPixelFormat112bpp6ChannelsAlpha,"WICPixelFormat112bpp6ChannelsAlpha" },
        { GUID_WICPixelFormat128bpp7ChannelsAlpha,"WICPixelFormat128bpp7ChannelsAlpha" },
        { GUID_WICPixelFormat144bpp8ChannelsAlpha,"WICPixelFormat144bpp8ChannelsAlpha" },
        { GUID_WICPixelFormat64bppRGBAHalf,"WICPixelFormat64bppRGBAHalf" },
        { GUID_WICPixelFormat48bppRGBHalf,"WICPixelFormat48bppRGBHalf" },
        { GUID_WICPixelFormat32bppRGBE,"WICPixelFormat32bppRGBE" },
        { GUID_WICPixelFormat16bppGrayHalf,"WICPixelFormat16bppGrayHalf" },
        { GUID_WICPixelFormat32bppGrayFixedPoint,"WICPixelFormat32bppGrayFixedPoint" },
        { GUID_WICPixelFormat64bppRGBFixedPoint,"WICPixelFormat64bppRGBFixedPoint" },
        { GUID_WICPixelFormat128bppRGBFixedPoint,"xelFormat128bppRGBFixedPoint" },
        { GUID_WICPixelFormat64bppRGBHalf,"GUID_WICPixelFormat64bppRGBHalf" }
    };

    for (size_t i = 0; i < sizeof(WicPxlFmtAndName) / sizeof(WicPxlFmtAndName[0]); ++i)
    {
        if (WicPxlFmtAndName[i].wicPxlFmt == wicPxlFmt)
        {
            return WicPxlFmtAndName[i].szText;
        }
    }

    return "Unknown";
}

static bool DeterminePixelType(const WICPixelFormatGUID& wicPxlFmt, GUID* destPixelFmt, PixelType* pxlType)
{
    static const struct
    {
        /// <summary>   The WIC-pixel-format as reported by the decoder. </summary>
        GUID wicPxlFmt;

        /// <summary>   The WIC-pixel-format  that we wish to get from the decoder (if necessary, utilizing a WIC-fomat converter). 
        ///             If this has the value "GUID_WICPixelFormatUndefined" it means: I am not sure at this point, I have never 
        ///             seen this and it is not obvious to me what to do. </summary>
        GUID wicDstPxlFmt;

        /// <summary>   The libCZI-pixelType which we finally want to end up with. </summary>
        PixelType pxlType;
    } WicPxlFmtAndPixelType[] =
    {
        {GUID_WICPixelFormatBlackWhite ,GUID_WICPixelFormat8bppGray ,PixelType::Gray8 },
        {GUID_WICPixelFormat8bppGray ,GUID_WICPixelFormat8bppGray ,PixelType::Gray8 },
        {GUID_WICPixelFormat16bppBGR555 ,GUID_WICPixelFormat24bppBGR,PixelType::Bgr24},
        {GUID_WICPixelFormat16bppGray ,GUID_WICPixelFormat16bppGray,PixelType::Gray16},
        {GUID_WICPixelFormat24bppBGR ,GUID_WICPixelFormat24bppBGR,PixelType::Bgr24},
        {GUID_WICPixelFormat24bppRGB ,GUID_WICPixelFormat24bppBGR,PixelType::Bgr24 },
        {GUID_WICPixelFormat32bppBGR ,GUID_WICPixelFormat24bppBGR,PixelType::Bgr24 },
        {GUID_WICPixelFormat32bppBGRA ,GUID_WICPixelFormat24bppBGR,PixelType::Bgr24 },
        {GUID_WICPixelFormat48bppRGBFixedPoint ,GUID_WICPixelFormatUndefined , PixelType::Invalid },
        {GUID_WICPixelFormat16bppGrayFixedPoint ,GUID_WICPixelFormatUndefined , PixelType::Invalid },
        {GUID_WICPixelFormat32bppBGR101010  ,GUID_WICPixelFormatUndefined , PixelType::Invalid },
        {GUID_WICPixelFormat48bppRGB, GUID_WICPixelFormat48bppRGB, PixelType::Bgr48 },
        {GUID_WICPixelFormat64bppRGBA,GUID_WICPixelFormat48bppRGB, PixelType::Bgr48 },
        {GUID_WICPixelFormat96bppRGBFixedPoint,GUID_WICPixelFormatUndefined , PixelType::Invalid },
        {GUID_WICPixelFormat96bppRGBFixedPoint,GUID_WICPixelFormatUndefined , PixelType::Invalid },
        {GUID_WICPixelFormat128bppRGBFloat,GUID_WICPixelFormatUndefined , PixelType::Invalid },
        {GUID_WICPixelFormat32bppCMYK,GUID_WICPixelFormat24bppBGR,PixelType::Bgr24 },
        {GUID_WICPixelFormat64bppRGBAFixedPoint,GUID_WICPixelFormatUndefined , PixelType::Invalid },
        {GUID_WICPixelFormat128bppRGBAFixedPoint,GUID_WICPixelFormatUndefined , PixelType::Invalid },
        {GUID_WICPixelFormat64bppCMYK,GUID_WICPixelFormatUndefined , PixelType::Invalid },
        {GUID_WICPixelFormat24bpp3Channels,GUID_WICPixelFormatUndefined , PixelType::Invalid },
        {GUID_WICPixelFormat32bpp4Channels,GUID_WICPixelFormatUndefined , PixelType::Invalid },
        {GUID_WICPixelFormat40bpp5Channels,GUID_WICPixelFormatUndefined , PixelType::Invalid },
        {GUID_WICPixelFormat48bpp6Channels, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat56bpp7Channels, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat64bpp8Channels, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat48bpp3Channels, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat64bpp4Channels, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat80bpp5Channels, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat96bpp6Channels, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat112bpp7Channels, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat128bpp8Channels, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat40bppCMYKAlpha, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat80bppCMYKAlpha, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat32bpp3ChannelsAlpha, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat64bpp7ChannelsAlpha, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat72bpp8ChannelsAlpha, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat64bpp3ChannelsAlpha, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat80bpp4ChannelsAlpha, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat96bpp5ChannelsAlpha, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat112bpp6ChannelsAlpha, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat128bpp7ChannelsAlpha, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat144bpp8ChannelsAlpha, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat64bppRGBAHalf, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat48bppRGBHalf, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat32bppRGBE, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat16bppGrayHalf, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat32bppGrayFixedPoint, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat64bppRGBFixedPoint, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat128bppRGBFixedPoint, GUID_WICPixelFormatUndefined, PixelType::Invalid },
        {GUID_WICPixelFormat64bppRGBHalf,  GUID_WICPixelFormatUndefined, PixelType::Invalid }
    };

    for (size_t i = 0; i < sizeof(WicPxlFmtAndPixelType) / sizeof(WicPxlFmtAndPixelType[0]); ++i)
    {
        if (WicPxlFmtAndPixelType[i].wicPxlFmt == wicPxlFmt)
        {
            if (destPixelFmt != nullptr) { *destPixelFmt = WicPxlFmtAndPixelType[i].wicDstPxlFmt; }
            if (pxlType != nullptr) { *pxlType = WicPxlFmtAndPixelType[i].pxlType; }
            return true;
        }
    }

    return false;
}

/*virtual*/std::shared_ptr<libCZI::IBitmapData> CWicJpgxrDecoder::Decode(const void* ptrData, size_t size, const libCZI::PixelType* pixelType, const uint32_t* width, const uint32_t* height, const char* additional_arguments)
{
    if (GetSite()->IsEnabled(LOGLEVEL_CHATTYINFORMATION))
    {
        stringstream ss; ss << "Begin WIC-JpgXR-Decode with " << size << " bytes";
        GetSite()->Log(LOGLEVEL_CHATTYINFORMATION, ss.str());
    }

    IWICStream* wicStream;
    HRESULT hr = this->pFactory->CreateStream(&wicStream);
    ThrowIfFailed("pFactory->CreateStream", hr);
    unique_ptr<IWICStream, COMDeleter> up_wicStream(wicStream);

    // Initialize the stream with the memory pointer and size.
    hr = up_wicStream->InitializeFromMemory(static_cast<BYTE*>(const_cast<void*>(ptrData)), static_cast<DWORD>(size));
    ThrowIfFailed("wicStream->InitializeFromMemory", hr);

    IWICBitmapDecoder* wicBitmapDecoder;
    hr = this->pFactory->CreateDecoderFromStream(
        up_wicStream.get(),
        NULL,/*decoder vendor*/
        WICDecodeMetadataCacheOnDemand,
        &wicBitmapDecoder);
    ThrowIfFailed("pFactory->CreateDecoderFromStream", hr);
    unique_ptr< IWICBitmapDecoder, COMDeleter> up_wicBitmapDecoder(wicBitmapDecoder);

    IWICBitmapFrameDecode* wicBitmapFrameDecode;
    hr = up_wicBitmapDecoder->GetFrame(0, &wicBitmapFrameDecode);
    ThrowIfFailed("wicBitmapDecoder->GetFrame", hr);
    unique_ptr<IWICBitmapFrameDecode, COMDeleter> up_wicBitmapFrameDecode(wicBitmapFrameDecode);

    IntSize sizeBitmap;
    WICPixelFormatGUID wicPxlFmt;
    WICPixelFormatGUID wicDestPxlFmt;
    PixelType px_type;
    hr = up_wicBitmapFrameDecode->GetPixelFormat(&wicPxlFmt);
    ThrowIfFailed("wicBitmapFrameDecode->GetPixelFormat", hr);

    if (GetSite()->IsEnabled(LOGLEVEL_CHATTYINFORMATION))
    {
        stringstream ss; ss << " Encoded PixelFormat:" << GetInformativeString(wicPxlFmt);
        GetSite()->Log(LOGLEVEL_CHATTYINFORMATION, ss.str());
    }

    bool b = DeterminePixelType(wicPxlFmt, &wicDestPxlFmt, &px_type);
    if (b == false || wicDestPxlFmt == GUID_WICPixelFormatUndefined)
    {
        throw  std::logic_error("need to look into these formats...");
    }

    if (pixelType != nullptr && px_type != *pixelType)
    {
        throw  std::logic_error("pixel type validation failed...");
    }

    hr = up_wicBitmapFrameDecode->GetSize(&sizeBitmap.w, &sizeBitmap.h);
    ThrowIfFailed("wicBitmapFrameDecode->GetSize", hr);

    if (width != nullptr && sizeBitmap.w != *width)
    {
        ostringstream ss;
        ss << "width mismatch: expected " << *width << ", but got " << sizeBitmap.w;
        throw std::logic_error(ss.str());
    }

    if (height != nullptr && sizeBitmap.h != *height)
    {
        ostringstream ss;
        ss << "height mismatch: expected " << *height << ", but got " << sizeBitmap.h;
        throw std::logic_error(ss.str());
    }

    if (GetSite()->IsEnabled(LOGLEVEL_CHATTYINFORMATION))
    {
        stringstream ss; ss << " Requested Decoded PixelFormat:" << GetInformativeString(wicDestPxlFmt) << " Width:" << sizeBitmap.w << " Height:" << sizeBitmap.h;
        GetSite()->Log(LOGLEVEL_CHATTYINFORMATION, ss.str());
    }

    auto bm = GetSite()->CreateBitmap(px_type, sizeBitmap.w, sizeBitmap.h);
    auto bmLckInfo = ScopedBitmapLockerSP(bm);

    if (wicPxlFmt == wicDestPxlFmt)
    {
        // in this case we do not need to create a converter
        hr = up_wicBitmapFrameDecode->CopyPixels(NULL, bmLckInfo.stride, bmLckInfo.stride * sizeBitmap.h, static_cast<BYTE*>(bmLckInfo.ptrDataRoi));
        ThrowIfFailed("wicBitmapFrameDecode->CopyPixels", hr);
    }
    else
    {
        IWICFormatConverter* pFormatConverter;
        hr = this->pFactory->CreateFormatConverter(&pFormatConverter);
        ThrowIfFailed("pFactory->CreateFormatConverter", hr);
        hr = pFormatConverter->Initialize(
            up_wicBitmapFrameDecode.get(),   // Input bitmap to convert
            wicDestPxlFmt,                   // Destination pixel format
            WICBitmapDitherTypeNone,         // Specified dither pattern
            nullptr,                         // Specify a particular palette 
            0,                               // Alpha threshold
            WICBitmapPaletteTypeCustom);     // Palette translation type
        ThrowIfFailed("pFormatConverter->Initialize", hr);
        unique_ptr<IWICFormatConverter, COMDeleter> up_formatConverter(pFormatConverter);
        hr = pFormatConverter->CopyPixels(NULL, bmLckInfo.stride, bmLckInfo.stride * sizeBitmap.h, static_cast<BYTE*>(bmLckInfo.ptrDataRoi));
        ThrowIfFailed("pFormatConverter->CopyPixels", hr);
    }

    // WIC-codec does not directly support "BGR48", so we need to convert (#36)
    if (px_type == PixelType::Bgr48)
    {
        CBitmapOperations::RGB48ToBGR48(sizeBitmap.w, sizeBitmap.h, static_cast<uint16_t*>(bmLckInfo.ptrDataRoi), bmLckInfo.stride);
    }

    if (GetSite()->IsEnabled(LOGLEVEL_CHATTYINFORMATION))
    {
        GetSite()->Log(LOGLEVEL_CHATTYINFORMATION, "Finished WIC-JpgXR-Decode");
    }

    return bm;
}

#endif
