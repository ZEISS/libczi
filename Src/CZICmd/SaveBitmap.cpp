// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SaveBitmap.h"
#include <fstream>
#include <stdexcept>
#include "utils.h"
#include "inc_CZIcmd_Config.h"

//-----------------------------------------------------------------------------

#if CZICMD_USE_WIC == 1
#include <memory>
#include <wincodec.h>
#include <sstream>
#include <iomanip>

using namespace std;

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

class CWicSaveBitmap :public ISaveBitmap
{
private:
    unique_ptr<IWICImagingFactory, COMDeleter> cpWicImagingFactory;
public:
    CWicSaveBitmap()
    {
        IWICImagingFactory* pWicImagingFactory;
        const HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, reinterpret_cast<LPVOID*>(&pWicImagingFactory));
        ThrowIfFailed("Creating WICImageFactory", hr);
        this->cpWicImagingFactory = unique_ptr<IWICImagingFactory, COMDeleter>(pWicImagingFactory);
    }

    virtual void Save(const wchar_t* fileName, SaveDataFormat dataFormat, libCZI::IBitmapData* bitmap)
    {
        static const struct
        {
            libCZI::PixelType   pixelType;
            GUID                wicPixelFormat;
        } LibCZIPixelTypeToWicPixelFormat[] =
        {
            { libCZI::PixelType::Bgr24 ,  GUID_WICPixelFormat24bppBGR },
            { libCZI::PixelType::Gray16 , GUID_WICPixelFormat16bppGray },
            { libCZI::PixelType::Gray8 ,  GUID_WICPixelFormat8bppGray },
            { libCZI::PixelType::Bgr48 ,  GUID_WICPixelFormat48bppBGR },
            { libCZI::PixelType::Bgra32 , GUID_WICPixelFormat32bppBGRA }
        };

        for (int i = 0; i < static_cast<int>(sizeof(LibCZIPixelTypeToWicPixelFormat) / sizeof(LibCZIPixelTypeToWicPixelFormat[0])); ++i)
        {
            if (bitmap->GetPixelType() == LibCZIPixelTypeToWicPixelFormat[i].pixelType)
            {
                this->SaveWithWIC(fileName, GUID_ContainerFormatPng, LibCZIPixelTypeToWicPixelFormat[i].wicPixelFormat, bitmap);
                return;
            }
        }

        throw std::runtime_error("Unsupported pixeltype encountered.");
    }

    virtual ~CWicSaveBitmap() = default;
private:
    void SaveWithWIC(const wchar_t* filename, const GUID encoder, const WICPixelFormatGUID& wicPixelFmt, libCZI::IBitmapData* bitmap)
    {
        IWICStream* stream;
        // Create a stream for the encoder
        HRESULT hr = this->cpWicImagingFactory->CreateStream(&stream);
        ThrowIfFailed("IWICImagingFactory::CreateStream", hr);
        const unique_ptr<IWICStream, COMDeleter> up_stream(stream);

        // Initialize the stream using the output file path
        hr = up_stream->InitializeFromFilename(filename, GENERIC_WRITE);
        ThrowIfFailed("IWICStream::InitializeFromFilename", hr);

        this->SaveWithWIC(up_stream.get(), encoder, wicPixelFmt, bitmap);

        hr = up_stream->Commit(STGC_DEFAULT);
        ThrowIfFailed("IWICStream::Commit", hr, [](HRESULT ec) {return SUCCEEDED(ec) || ec == E_NOTIMPL; });
    }

    void SaveWithWIC(IWICStream* destStream, const GUID encoder, const WICPixelFormatGUID& wicPixelFmt, libCZI::IBitmapData* spBitmap)
    {
        // cf. http://msdn.microsoft.com/en-us/library/windows/desktop/ee719797(v=vs.85).aspx

        IWICBitmapEncoder* wicBitmapEncoder;
        HRESULT hr = this->cpWicImagingFactory->CreateEncoder(
            encoder,
            nullptr,    // No preferred codec vendor.
            &wicBitmapEncoder);
        ThrowIfFailed("Creating IWICImagingFactory::CreateEncoder", hr);
        unique_ptr<IWICBitmapEncoder, COMDeleter> up_wicBitmapEncoder(wicBitmapEncoder);

        // Create encoder to write to image file
        hr = wicBitmapEncoder->Initialize(destStream, WICBitmapEncoderNoCache);
        ThrowIfFailed("IWICBitmapEncoder::Initialize", hr);

        IWICBitmapFrameEncode* frameEncode;
        hr = wicBitmapEncoder->CreateNewFrame(&frameEncode, nullptr);
        ThrowIfFailed("IWICBitmapEncoder::CreateNewFrame", hr);
        unique_ptr<IWICBitmapFrameEncode, COMDeleter> up_frameEncode(frameEncode);

        hr = frameEncode->Initialize(nullptr);
        ThrowIfFailed("IWICBitmapFrameEncode::CreateNewFrame", hr);

        hr = frameEncode->SetSize(spBitmap->GetWidth(), spBitmap->GetHeight());
        ThrowIfFailed("IWICBitmapFrameEncode::SetSize", hr);

        WICPixelFormatGUID pixelFormat = wicPixelFmt;
        hr = frameEncode->SetPixelFormat(&pixelFormat);
        ThrowIfFailed("IWICBitmapFrameEncode::SetPixelFormat", hr);

        if (pixelFormat != wicPixelFmt)
        {
            // The encoder does not support the pixelformat we want to add, but it was as kind as
            // to give a proposal of the "closest comparable pixelformat it supports" -> so, now 
            // we better employ a converter...

            // TODO
        }

        const auto bitmapData = spBitmap->Lock();
        hr = frameEncode->WritePixels(spBitmap->GetHeight(), bitmapData.stride, spBitmap->GetHeight() * bitmapData.stride, static_cast<BYTE*>(bitmapData.ptrDataRoi));
        spBitmap->Unlock();
        ThrowIfFailed("IWICBitmapFrameEncode::WritePixels", hr);

        hr = frameEncode->Commit();
        ThrowIfFailed("IWICBitmapFrameEncode::Commit", hr);

        hr = wicBitmapEncoder->Commit();
        ThrowIfFailed("IWICBitmapEncoder::Commit", hr);
    }

    static void ThrowIfFailed(const char* function, HRESULT hr)
    {
        return ThrowIfFailed(function, hr, [](HRESULT hresult)->bool {return SUCCEEDED(hresult); });
    }

    static void ThrowIfFailed(const char* function, HRESULT hr, const std::function<bool(HRESULT)>& checkFunc)
    {
        if (!checkFunc(hr))
        {
            ostringstream string_stream;
            string_stream << "COM-ERROR hr=0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << hr << " (" << function << ")";
            throw std::runtime_error(string_stream.str());
        }
    }
};
#endif

//-----------------------------------------------------------------------------

#if CZICMD_USE_LIBPNG == 1
#include <png.h>

class CLibPngSaveBitmap :public ISaveBitmap
{
private:
    struct PngStructInfoGuard
    {
        png_structp png_ptr;
        png_infop info_ptr;
        PngStructInfoGuard(png_structp png_ptr, png_infop info_ptr) :
            png_ptr(png_ptr), info_ptr(info_ptr) {};
        PngStructInfoGuard() :png_ptr(nullptr), info_ptr(nullptr) {};
        ~PngStructInfoGuard()
        {
            png_destroy_write_struct(&this->png_ptr, &this->info_ptr);
        }
    };
public:
    virtual void Save(const wchar_t* fileName, SaveDataFormat dataFormat, libCZI::IBitmapData* bitmap)
    {
        switch (bitmap->GetPixelType())
        {
        case libCZI::PixelType::Bgr24:
            this->SaveBgr24(fileName, bitmap);
            break;
        case libCZI::PixelType::Bgr48:
            this->SaveBgr48(fileName, bitmap);
            break;
        case libCZI::PixelType::Gray16:
            this->SaveGray16(fileName, bitmap);
            break;
        case libCZI::PixelType::Gray8:
            this->SaveGray8(fileName, bitmap);
            break;
        case libCZI::PixelType::Bgra32:
            this->SaveBgra32(fileName, bitmap);
            break;
        default:
            throw std::logic_error("pixeltype not implemented");
        }
    }

    virtual ~CLibPngSaveBitmap()
    {}
private:
    void SaveBgr24(const wchar_t* fileName, libCZI::IBitmapData* bitmap)
    {
        this->SavePngTweakLineBeforeWritng(fileName, bitmap, 8, PNG_COLOR_TYPE_RGB,
            [](std::uint32_t width, void* ptrData)->void
            {
                char* p = (char*)ptrData;
                for (std::uint32_t x = 0; x < width; ++x)
                {
                    std::swap(*p, *(p + 2));
                    p += 3;
                }
            });
    }

    void SaveBgra32(const wchar_t* fileName, libCZI::IBitmapData* bitmap)
    {
        this->SavePngTweakLineBeforeWritng(fileName, bitmap, 8, PNG_COLOR_TYPE_RGB_ALPHA,
            [](std::uint32_t width, void* ptrData)->void
            {
                char* p = (char*)ptrData;
                for (std::uint32_t x = 0; x < width; ++x)
                {
                    std::swap(*p, *(p + 2));
                    p += 4;
                }
            });
    }

    void SaveBgr48(const wchar_t* fileName, libCZI::IBitmapData* bitmap)
    {
        this->SavePngTweakLineBeforeWritng(fileName, bitmap, 16, PNG_COLOR_TYPE_RGB,
            [](std::uint32_t width, void* ptrData)->void
            {
                unsigned short* p = (unsigned short*)ptrData;
                for (std::uint32_t x = 0; x < width; ++x)
                {
                    std::swap(*p, *(p + 2));
                    p += 3;
                }
            });
    }

    void SaveGray16(const wchar_t* fileName, libCZI::IBitmapData* bitmap)
    {
        this->SavePng(fileName, bitmap, 16, PNG_COLOR_TYPE_GRAY);
    }

    void SaveGray8(const wchar_t* fileName, libCZI::IBitmapData* bitmap)
    {
        this->SavePng(fileName, bitmap, 8, PNG_COLOR_TYPE_GRAY);
    }

    void SavePng(const wchar_t* fileName, libCZI::IBitmapData* bitmap, int bit_depth, int color_type)
    {
        std::unique_ptr<FILE, decltype (&fclose)> fp(this->OpenDestForWrite(fileName), &fclose);

        PngStructInfoGuard pngStructInfo;
        pngStructInfo.png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        this->ThrowIfNull(pngStructInfo.png_ptr, fileName, "'png_create_write_struct' failed.");
        pngStructInfo.info_ptr = png_create_info_struct(pngStructInfo.png_ptr);
        this->ThrowIfNull(pngStructInfo.info_ptr, fileName, "'png_create_info_struct' failed.");

        png_init_io(pngStructInfo.png_ptr, fp.get());

        png_set_IHDR(pngStructInfo.png_ptr, pngStructInfo.info_ptr, bitmap->GetWidth(), bitmap->GetHeight(),
            bit_depth, color_type, PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        png_write_info(pngStructInfo.png_ptr, pngStructInfo.info_ptr);

        {
            libCZI::ScopedBitmapLockerP lckScoped{ bitmap };
            for (std::uint32_t h = 0; h < bitmap->GetHeight(); ++h) 
            {
                png_bytep ptr = (png_bytep)(((char*)lckScoped.ptrDataRoi) + h * lckScoped.stride);
                png_write_row(pngStructInfo.png_ptr, ptr);
            }
        }

        png_write_end(pngStructInfo.png_ptr, NULL);
    }

    void SavePngTweakLineBeforeWritng(const wchar_t* fileName, libCZI::IBitmapData* bitmap, int bit_depth, int color_type, std::function<void(std::uint32_t, void*)> tweakLine)
    {
        std::unique_ptr<FILE, decltype (&fclose)> fp(this->OpenDestForWrite(fileName), &fclose);

        PngStructInfoGuard pngStructInfo;
        pngStructInfo.png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        this->ThrowIfNull(pngStructInfo.png_ptr, fileName, "'png_create_write_struct' failed.");
        pngStructInfo.info_ptr = png_create_info_struct(pngStructInfo.png_ptr);
        this->ThrowIfNull(pngStructInfo.info_ptr, fileName, "'png_create_info_struct' failed.");

        png_init_io(pngStructInfo.png_ptr, fp.get());

        png_set_IHDR(pngStructInfo.png_ptr, pngStructInfo.info_ptr, bitmap->GetWidth(), bitmap->GetHeight(),
            bit_depth, color_type, PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        png_write_info(pngStructInfo.png_ptr, pngStructInfo.info_ptr);

        {
            libCZI::ScopedBitmapLockerP lckScoped{ bitmap };
            std::unique_ptr<void, decltype(&free)> lineToTweak(malloc(lckScoped.stride), &free);
            for (std::uint32_t h = 0; h < bitmap->GetHeight(); ++h) 
            {
                void* ptr = (((char*)lckScoped.ptrDataRoi) + h * lckScoped.stride);
                memcpy(lineToTweak.get(), ptr, lckScoped.stride);
                tweakLine(bitmap->GetWidth(), lineToTweak.get());
                png_write_row(pngStructInfo.png_ptr, (png_bytep)lineToTweak.get());
            }
        }

        png_write_end(pngStructInfo.png_ptr, NULL);
    }

    FILE* OpenDestForWrite(const wchar_t* fileName)
    {
        std::string fileNameUtf8 = convertToUtf8(fileName);
        FILE* fp = fopen(fileNameUtf8.c_str(), "wb");
        this->ThrowIfNull(fp, fileName, "fopen failed");
        return fp;
    }

    void ThrowIfNull(const void* p, const wchar_t* fileName, const char* info)
    {
        if (p == nullptr)
        {
            std::stringstream ss;
            std::string fileNameUtf8 = convertToUtf8(fileName);
            ss << "Error while writing PNG to \"" << fileNameUtf8 << "\"";
            if (info != nullptr)
            {
                ss << " (" << info << ")";
            }

            throw std::runtime_error(ss.str());
        }
    }
};
#endif

/*static*/const char CSaveBitmapFactory::WIC_CLASS[] = "WIC";
/*static*/const char CSaveBitmapFactory::LIBPNG_CLASS[] = "PNG";

/*static*/std::shared_ptr<ISaveBitmap> CSaveBitmapFactory::CreateSaveBitmapObj(const char* className)
{
    if (className != nullptr)
    {
#if CZICMD_USE_WIC == 1
        if (strcmp(className, CSaveBitmapFactory::WIC_CLASS) == 0)
        {
            return std::make_shared<CWicSaveBitmap>();
        }
#endif
#if CZICMD_USE_LIBPNG == 1
        if (strcmp(className, CSaveBitmapFactory::LIBPNG_CLASS) == 0)
        {
            return std::make_shared<CLibPngSaveBitmap>();
        }
#endif
        return nullptr;
    }

    return  CSaveBitmapFactory::CreateDefaultSaveBitmapObj();
}

/*static*/std::shared_ptr<ISaveBitmap> CSaveBitmapFactory::CreateDefaultSaveBitmapObj()
{
#if CZICMD_USE_WIC == 1
    return std::make_shared<CWicSaveBitmap>();
#elif CZICMD_USE_LIBPNG == 1
    return std::make_shared<CLibPngSaveBitmap>();
#else
    return nullptr;
#endif
}
