#include "save_bitmap.h"
#include <memory>
#include <wincodec.h>

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
            char errorMsg[255];
            _snprintf_s(errorMsg, _TRUNCATE, "COM-ERROR hr=0x%08X (%s)", hr, function);
            throw std::runtime_error(errorMsg);
        }
    }
};


/*static*/std::shared_ptr<ISaveBitmap> CSaveBitmapFactory::CreateDefaultSaveBitmapObj()
{
    return std::make_shared<CWicSaveBitmap>();
}
