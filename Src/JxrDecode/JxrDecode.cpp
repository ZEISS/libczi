// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "JxrDecode.h"
#include <memory>
#include <stdexcept> 
#include <sstream>
#include "jxrlib/jxrgluelib/JXRGlue.h"

#include "jxrlib/image/sys/windowsmediaphoto.h"

using namespace std;

static void ApplyQuality(float quality, JxrDecode::PixelFormat pixel_format, std::uint32_t width, PKImageEncode* pEncoder);

static bool IsEqualGuid(const GUID& guid1, const GUID& guid2)
{
    return IsEqualGUID(&guid1, &guid2) != 0;
}

static const char* ERR_to_string(ERR error_code)
{
    switch (error_code)
    {
    case WMP_errSuccess:return "WMP_errSuccess";
    case WMP_errFail:return "WMP_errFail";
    case WMP_errNotYetImplemented:return "WMP_errNotYetImplemented";
    case WMP_errAbstractMethod:return "WMP_errAbstractMethod";
    case WMP_errOutOfMemory:return "WMP_errOutOfMemory";
    case WMP_errFileIO:return "WMP_errFileIO";
    case WMP_errBufferOverflow:return "WMP_errBufferOverflow";
    case WMP_errInvalidParameter:return "WMP_errInvalidParameter";
    case WMP_errInvalidArgument:return "WMP_errInvalidArgument";
    case WMP_errUnsupportedFormat:return "WMP_errUnsupportedFormat";
    case WMP_errIncorrectCodecVersion:return "WMP_errIncorrectCodecVersion";
    case WMP_errIndexNotFound:return "WMP_errIndexNotFound";
    case WMP_errOutOfSequence:return "WMP_errOutOfSequence";
    case WMP_errNotInitialized:return "WMP_errNotInitialized";
    case WMP_errMustBeMultipleOf16LinesUntilLastCall:return "WMP_errMustBeMultipleOf16LinesUntilLastCall";
    case WMP_errPlanarAlphaBandedEncRequiresTempFile:return "WMP_errPlanarAlphaBandedEncRequiresTempFile";
    case WMP_errAlphaModeCannotBeTranscoded:return "WMP_errAlphaModeCannotBeTranscoded";
    case WMP_errIncorrectCodecSubVersion:return "WMP_errIncorrectCodecSubVersion";
    }

    return nullptr;
}

static JxrDecode::PixelFormat JxrPixelFormatGuidToEnum(const GUID& guid)
{
    if (IsEqualGuid(guid, GUID_PKPixelFormat8bppGray))
    {
        return JxrDecode::PixelFormat::kGray8;
    }
    else if (IsEqualGuid(guid, GUID_PKPixelFormat16bppGray))
    {
        return JxrDecode::PixelFormat::kGray16;
    }
    else if (IsEqualGuid(guid, GUID_PKPixelFormat24bppBGR))
    {
        return JxrDecode::PixelFormat::kBgr24;
    }
    else if (IsEqualGuid(guid, GUID_PKPixelFormat48bppRGB))
    {
        return JxrDecode::PixelFormat::kBgr48;
    }
    else if (IsEqualGuid(guid, GUID_PKPixelFormat32bppGrayFloat))
    {
        return JxrDecode::PixelFormat::kGray32Float;
    }

    return JxrDecode::PixelFormat::kInvalid;
}

static void WriteGuidToStream(ostringstream& string_stream, const GUID& guid)
{
    string_stream << std::uppercase;
    string_stream.width(8);
    string_stream << std::hex << guid.Data1 << '-';
    string_stream.width(4);
    string_stream << std::hex << guid.Data2 << '-';
    string_stream.width(4);
    string_stream << std::hex << guid.Data3 << '-';
    string_stream.width(2);
    string_stream << std::hex
        << static_cast<short>(guid.Data4[0])
        << static_cast<short>(guid.Data4[1])
        << '-'
        << static_cast<short>(guid.Data4[2])
        << static_cast<short>(guid.Data4[3])
        << static_cast<short>(guid.Data4[4])
        << static_cast<short>(guid.Data4[5])
        << static_cast<short>(guid.Data4[6])
        << static_cast<short>(guid.Data4[7]);
    string_stream << std::nouppercase;
}

void JxrDecode::Decode(
            const void* ptrData,
            size_t size,
            const std::function<std::tuple<void*, std::uint32_t>(PixelFormat pixel_format, std::uint32_t  width, std::uint32_t  height)>& get_destination_func)
{
    if (ptrData == nullptr)
    {
        throw invalid_argument("ptrData");
    }

    if (size == 0)
    {
        throw invalid_argument("size");
    }

    if (!get_destination_func)
    {
        throw invalid_argument("get_destination_func");
    }

    WMPStream* pStream = nullptr;
    ERR err = CreateWS_Memory(&pStream, const_cast<void*>(ptrData), size);
    if (Failed(err))
    {
        // note: the call "CreateWS_Memory" cannot fail (or the only way it can fail is that the memory allocation fails),
        //        so we do not have to release/free the stream object here.
        ThrowJxrlibError("'CreateWS_Memory' failed", err);
    }

    unique_ptr<WMPStream, void(*)(WMPStream*)> upStream(pStream, [](WMPStream* p)->void {p->Close(&p); });

    PKImageDecode* pDecoder = nullptr;
    err = PKCodecFactory_CreateDecoderFromStream(pStream, &pDecoder);
    if (Failed(err))
    {
        // unfortunately, "PKCodecFactory_CreateDecoderFromStream" may fail leaving us with a partially constructed
        //  decoder object, so we need to release/free the decoder object here.
        if (pDecoder != nullptr)
        {
            pDecoder->Release(&pDecoder);
        }

        ThrowJxrlibError("'PKCodecFactory_CreateDecoderFromStream' failed", err);
    }

    // construct a smart pointer which will destroy the decoder object when it goes out of scope
    std::unique_ptr<PKImageDecode, void(*)(PKImageDecode*)> upDecoder(pDecoder, [](PKImageDecode* p)->void {p->Release(&p); });

    U32 frame_count;
    err = upDecoder->GetFrameCount(upDecoder.get(), &frame_count);
    if (Failed(err))
    {
        ThrowJxrlibError("'decoder::GetFrameCount' failed", err);
    }

    if (frame_count != 1)
    {
        ostringstream string_stream;
        string_stream << "Expecting to find a frame_count of 1, but found frame_count = ." << frame_count;
        throw runtime_error(string_stream.str());
    }

    I32 width, height;
    upDecoder->GetSize(upDecoder.get(), &width, &height);
    if (Failed(err))
    {
        ThrowJxrlibError("'decoder::GetSize' failed", err);
    }

    PKPixelFormatGUID pixel_format_of_decoder;
    upDecoder->GetPixelFormat(upDecoder.get(), &pixel_format_of_decoder);
    if (Failed(err))
    {
        ThrowJxrlibError("'decoder::GetPixelFormat' failed", err);
    }

    const auto jxrpixel_format = JxrPixelFormatGuidToEnum(pixel_format_of_decoder);
    if (jxrpixel_format == JxrDecode::PixelFormat::kInvalid)
    {
        ostringstream string_stream;
        string_stream << "Unsupported pixel format: {";
        WriteGuidToStream(string_stream, pixel_format_of_decoder);
        string_stream << "}";
        throw runtime_error(string_stream.str());
    }

    const auto decode_info = get_destination_func(
        jxrpixel_format,
        width,
        height);

    const PKRect rc{ 0, 0, width, height };
    err = upDecoder->Copy(
        upDecoder.get(),
        &rc,
        static_cast<U8*>(get<0>(decode_info)),
        get<1>(decode_info));
    if (Failed(err))
    {
        ThrowJxrlibError("decoder::Copy failed", err);
    }
}

/*static*/JxrDecode::CompressedData JxrDecode::Encode(
                    JxrDecode::PixelFormat pixel_format,
                    std::uint32_t width,
                    std::uint32_t height,
                    std::uint32_t stride,
                    const void* ptr_bitmap,
                    float quality/*=1.f*/)
{
    if (ptr_bitmap == nullptr)
    {
        throw invalid_argument("ptr_bitmap");
    }

    if (quality < 0.f || quality > 1.f)
    {
        throw invalid_argument("quality");
    }

    if (width == 0)
    {
        throw invalid_argument("width");
    }

    if (height == 0)
    {
        throw invalid_argument("height");
    }

    const auto bytes_per_pel = JxrDecode::GetBytesPerPel(pixel_format);
    if (bytes_per_pel == 0xff)
    {
        throw invalid_argument("pixel_format");
    }

    if (stride < width * bytes_per_pel)
    {
        throw invalid_argument("stride");
    }

    PKImageEncode* pImageEncoder;
    ERR err = PKCodecFactory_CreateCodec(&IID_PKImageWmpEncode, reinterpret_cast<void**>(&pImageEncoder));
    if (Failed(err))
    {
        ThrowJxrlibError("'PKCodecFactory_CreateCodec' failed", err);
    }

    unique_ptr<PKImageEncode, void(*)(PKImageEncode*)> upImageEncoder(
        pImageEncoder,
        [](PKImageEncode* p)->void
        {
            // If we get here, we need to 'disassociate' the stream from the encoder,
            //  because this image-encode-object would otherwise try to destroy the stream-object.
            //  However, we want to keep the stream-object around, so we can return it to the caller
            //  (and, in case of leaving with an exception, the stream-object will be destroyed by the unique_ptr below).
            p->pStream = nullptr;
            p->Release(&p);
        });

    CWMIStrCodecParam codec_parameters = {};
    codec_parameters.bVerbose = FALSE;
    codec_parameters.cfColorFormat = YUV_444;
    codec_parameters.bdBitDepth = BD_LONG;
    codec_parameters.bfBitstreamFormat = FREQUENCY;
    codec_parameters.bProgressiveMode = TRUE;
    codec_parameters.olOverlap = OL_ONE;
    codec_parameters.cNumOfSliceMinus1H = codec_parameters.cNumOfSliceMinus1V = 0;
    codec_parameters.sbSubband = SB_ALL;
    codec_parameters.uAlphaMode = 0;
    codec_parameters.uiDefaultQPIndex = 1;
    codec_parameters.uiDefaultQPIndexAlpha = 1;

    struct tagWMPStream* pEncodeStream = nullptr;
    err = CreateWS_HeapBackedWriteableStream(&pEncodeStream, 1024, 0);
    if (Failed(err))
    {
        ThrowJxrlibError("'CreateWS_HeapBackedWriteableStream' failed", err);
    }

    unique_ptr<WMPStream, void(*)(WMPStream*)> upEncodeStream(pEncodeStream, [](WMPStream* p)->void {p->Close(&p); });

    err = upImageEncoder->Initialize(upImageEncoder.get(), pEncodeStream, &codec_parameters, sizeof(codec_parameters));
    if (Failed(err))
    {
        ThrowJxrlibError("'encoder::Initialize' failed", err);
    }

    if (quality < 1.f)
    {
        ApplyQuality(quality, pixel_format, width, upImageEncoder.get());
    }

    switch (pixel_format)
    {
    case PixelFormat::kBgr24:
        err = upImageEncoder->SetPixelFormat(upImageEncoder.get(), GUID_PKPixelFormat24bppBGR);
        break;
    case PixelFormat::kGray8:
        err = upImageEncoder->SetPixelFormat(upImageEncoder.get(), GUID_PKPixelFormat8bppGray);
        break;
    case PixelFormat::kBgr48:
        err = upImageEncoder->SetPixelFormat(upImageEncoder.get(), GUID_PKPixelFormat48bppRGB);
        break;
    case PixelFormat::kGray16:
        err = upImageEncoder->SetPixelFormat(upImageEncoder.get(), GUID_PKPixelFormat16bppGray);
        break;
    case PixelFormat::kGray32Float:
        err = upImageEncoder->SetPixelFormat(upImageEncoder.get(), GUID_PKPixelFormat32bppGrayFloat);
        break;
    default:
    {
        ostringstream string_stream;
        string_stream << "Unsupported pixel format specified: " << static_cast<int>(pixel_format);
        throw invalid_argument(string_stream.str());
    }
    }

    if (Failed(err))
    {
        ThrowJxrlibError("'PKImageEncode::SetPixelFormat' failed", err);
    }

    err = upImageEncoder->SetSize(upImageEncoder.get(), static_cast<I32>(width), static_cast<I32>(height));
    if (Failed(err))
    {
        ostringstream string_stream;
        string_stream << "'PKImageEncode::SetSize(" << width << "," << height << ")' failed.";
        ThrowJxrlibError(string_stream, err);
    }

    err = upImageEncoder->SetResolution(upImageEncoder.get(), 96.f, 96.f);
    if (Failed(err))
    {
        ThrowJxrlibError("'PKImageEncode::SetResolution' failed", err);
    }

    err = upImageEncoder->WritePixels(upImageEncoder.get(), height, const_cast<U8*>(static_cast<const U8*>(ptr_bitmap)), stride);
    if (Failed(err))
    {
        ThrowJxrlibError("'PKImageEncode::WritePixels' failed", err);
    }

    return { upEncodeStream.release() };
}

/*static*/void JxrDecode::ThrowJxrlibError(const std::string& message, int error_code)
{
    ostringstream string_stream(message);
    JxrDecode::ThrowJxrlibError(string_stream, error_code);
}

/*static*/void JxrDecode::ThrowJxrlibError(std::ostringstream& message, int error_code)
{
    message << " - ERR=" << error_code;
    const char* error_code_string = ERR_to_string(error_code);
    if (error_code_string != nullptr)
    {
        message << " (" << error_code_string << ")";
    }

    throw runtime_error(message.str());
}

JxrDecode::CompressedData::~CompressedData()
{
    if (this->obj_handle_ != nullptr)
    {
        CloseWS_HeapBackedWriteableStream(reinterpret_cast<struct tagWMPStream**>(&this->obj_handle_));
    }
}

void* JxrDecode::CompressedData::GetMemory() const
{
    void* data = nullptr;
    GetWS_HeapBackedWriteableStreamBuffer(
        static_cast<struct tagWMPStream*>(this->obj_handle_),
        &data,
        nullptr);
    return data;
}

size_t JxrDecode::CompressedData::GetSize() const
{
    size_t size = 0;
    GetWS_HeapBackedWriteableStreamBuffer(
        static_cast<struct tagWMPStream*>(this->obj_handle_),
        nullptr,
        &size);
    return size;
}

/*static*/std::uint8_t JxrDecode::GetBytesPerPel(PixelFormat pixel_format)
{
    switch (pixel_format)
    {
    case PixelFormat::kBgr24:
        return 3;
    case PixelFormat::kBgr48:
        return 6;
    case PixelFormat::kGray8:
        return 1;
    case PixelFormat::kGray16:
        return 2;
    case PixelFormat::kGray32Float:
        return 4;
    default:
        return 0xff;
    }
}

/// Makes adjustments to the encoder object, based on the quality parameter. The quality parameter is expected to be a number between 0 and 1.
///
/// \param          quality         The quality (must be between 0 and 1).
/// \param          pixel_format    The pixel format.
/// \param          width           The width in pixels.
/// \param [in,out] pEncoder        The encoder object (where the adjustments, according to the quality setting, are made).
/*static*/void ApplyQuality(float quality, JxrDecode::PixelFormat pixel_format, uint32_t width, PKImageEncode* pEncoder)
{
    // this is resembling the code from https://github.com/ptahmose/jxrlib/blob/f7521879862b9085318e814c6157490dd9dbbdb4/jxrencoderdecoder/JxrEncApp.c#L677C1-L738C10
    // It has been tweaked to our more limited case (i.e. only 8 bit and 16 bit, and only 1 channel)
#if 1
    // optimized for PSNR
    static const int DPK_QPS_420[11][6] =
    {      // for 8 bit only
        { 66, 65, 70, 72, 72, 77 },
        { 59, 58, 63, 64, 63, 68 },
        { 52, 51, 57, 56, 56, 61 },
        { 48, 48, 54, 51, 50, 55 },
        { 43, 44, 48, 46, 46, 49 },
        { 37, 37, 42, 38, 38, 43 },
        { 26, 28, 31, 27, 28, 31 },
        { 16, 17, 22, 16, 17, 21 },
        { 10, 11, 13, 10, 10, 13 },
        {  5,  5,  6,  5,  5,  6 },
        {  2,  2,  3,  2,  2,  2 }
    };

    static const int DPK_QPS_8[12][6] =
    {
        { 67, 79, 86, 72, 90, 98 },
        { 59, 74, 80, 64, 83, 89 },
        { 53, 68, 75, 57, 76, 83 },
        { 49, 64, 71, 53, 70, 77 },
        { 45, 60, 67, 48, 67, 74 },
        { 40, 56, 62, 42, 59, 66 },
        { 33, 49, 55, 35, 51, 58 },
        { 27, 44, 49, 28, 45, 50 },
        { 20, 36, 42, 20, 38, 44 },
        { 13, 27, 34, 13, 28, 34 },
        {  7, 17, 21,  8, 17, 21 }, // Photoshop 100%
        {  2,  5,  6,  2,  5,  6 }
    };
#else
    // optimized for SSIM
    static const int DPK_QPS_420[11][6] =
    {      // for 8 bit only
        { 67, 77, 80, 75, 82, 86 },
        { 58, 67, 71, 63, 74, 78 },
        { 50, 60, 64, 54, 66, 69 },
        { 46, 55, 59, 49, 60, 63 },
        { 41, 48, 53, 43, 52, 56 },
        { 35, 43, 48, 36, 44, 49 },
        { 29, 37, 41, 30, 38, 41 },
        { 22, 29, 33, 22, 29, 33 },
        { 15, 20, 26, 14, 20, 25 },
        {  9, 14, 18,  8, 14, 17 },
        {  4,  6,  7,  3,  5,  5 }
    };

    static const int DPK_QPS_8[12][6] =
    {
        { 67, 93, 98, 71, 98, 104 },
        { 59, 83, 88, 61, 89,  95 },
        { 50, 76, 81, 53, 85,  90 },
        { 46, 71, 77, 47, 79,  85 },
        { 41, 67, 71, 42, 75,  78 },
        { 34, 59, 65, 35, 66,  72 },
        { 30, 54, 60, 29, 60,  66 },
        { 24, 48, 53, 22, 53,  58 },
        { 18, 39, 45, 17, 43,  48 },
        { 13, 34, 38, 11, 35,  38 },
        {  8, 20, 24,  7, 22,  25 }, // Photoshop 100%
        {  2,  5,  6,  2,  5,   6 }
    };
#endif

    static const int DPK_QPS_16[11][6] =
    {
        { 197, 203, 210, 202, 207, 213 },
        { 174, 188, 193, 180, 189, 196 },
        { 152, 167, 173, 156, 169, 174 },
        { 135, 152, 157, 137, 153, 158 },
        { 119, 137, 141, 119, 138, 142 },
        { 102, 120, 125, 100, 120, 124 },
        {  82,  98, 104,  79,  98, 103 },
        {  60,  76,  81,  58,  76,  81 },
        {  39,  52,  58,  36,  52,  58 },
        {  16,  27,  33,  14,  27,  33 },
        {   5,   8,   9,   4,   7,   8 }
    };

    /*static const int DPK_QPS_16f[11][6] = {
        { 148, 177, 171, 165, 187, 191 },
        { 133, 155, 153, 147, 172, 181 },
        { 114, 133, 138, 130, 157, 167 },
        {  97, 118, 120, 109, 137, 144 },
        {  76,  98, 103,  85, 115, 121 },
        {  63,  86,  91,  62,  96,  99 },
        {  46,  68,  71,  43,  73,  75 },
        {  29,  48,  52,  27,  48,  51 },
        {  16,  30,  35,  14,  29,  34 },
        {   8,  14,  17,   7,  13,  17 },
        {   3,   5,   7,   3,   5,   6 }
    };*/

    static const int DPK_QPS_32f[11][6] =
    {
        { 194, 206, 209, 204, 211, 217 },
        { 175, 187, 196, 186, 193, 205 },
        { 157, 170, 177, 167, 180, 190 },
        { 133, 152, 156, 144, 163, 168 },
        { 116, 138, 142, 117, 143, 148 },
        {  98, 120, 123,  96, 123, 126 },
        {  80,  99, 102,  78,  99, 102 },
        {  65,  79,  84,  63,  79,  84 },
        {  48,  61,  67,  45,  60,  66 },
        {  27,  41,  46,  24,  40,  45 },
        {   3,  22,  24,   2,  21,  22 }
    };

    // Image width must be at least 2 MB wide for subsampled chroma and two levels of overlap!
    if (quality >= 0.5F || width < 2 * MB_WIDTH_PIXEL)
    {
        pEncoder->WMP.wmiSCP.olOverlap = OL_ONE;
    }
    else
    {
        pEncoder->WMP.wmiSCP.olOverlap = OL_TWO;
    }

    if (quality >= 0.5F || /*PI.uBitsPerSample > 8*/(pixel_format == JxrDecode::PixelFormat::kBgr48 || pixel_format == JxrDecode::PixelFormat::kGray16))
    {
        pEncoder->WMP.wmiSCP.cfColorFormat = YUV_444;
    }
    else
    {
        pEncoder->WMP.wmiSCP.cfColorFormat = YUV_420;
    }

    // remap [0.8, 0.866, 0.933, 1.0] to [0.8, 0.9, 1.0, 1.1]
    // to use 8-bit DPK QP table (0.933 == Photoshop JPEG 100)

    if (quality > 0.8f &&
        (pixel_format == JxrDecode::PixelFormat::kBgr24 || pixel_format == JxrDecode::PixelFormat::kGray8) &&
        pEncoder->WMP.wmiSCP.cfColorFormat != YUV_420 &&
        pEncoder->WMP.wmiSCP.cfColorFormat != YUV_422)
    {
        quality = 0.8f + (quality - 0.8f) * 1.5f;
    }

    const int qi = static_cast<int>(10.f * quality);
    const float qf = 10.f * quality - static_cast<float>(qi);

    const int* pQPs =
        (pEncoder->WMP.wmiSCP.cfColorFormat == YUV_420 || pEncoder->WMP.wmiSCP.cfColorFormat == YUV_422) ?
        DPK_QPS_420[qi] :
        (pixel_format == JxrDecode::PixelFormat::kBgr24 || pixel_format == JxrDecode::PixelFormat::kGray8) ? DPK_QPS_8[qi] :
        ((pixel_format == JxrDecode::PixelFormat::kBgr48 || pixel_format == JxrDecode::PixelFormat::kGray16) ? DPK_QPS_16[qi] :
        (DPK_QPS_32f[qi]));

    pEncoder->WMP.wmiSCP.uiDefaultQPIndex = static_cast<U8>(0.5f +
        static_cast<float>(pQPs[0]) * (1.f - qf) + static_cast<float>((pQPs + 6)[0]) * qf);
    pEncoder->WMP.wmiSCP.uiDefaultQPIndexU = static_cast<U8>(0.5f +
        static_cast<float>(pQPs[1]) * (1.f - qf) + static_cast<float>((pQPs + 6)[1]) * qf);
    pEncoder->WMP.wmiSCP.uiDefaultQPIndexV = static_cast<U8>(0.5f +
        static_cast<float>(pQPs[2]) * (1.f - qf) + static_cast<float>((pQPs + 6)[2]) * qf);
    pEncoder->WMP.wmiSCP.uiDefaultQPIndexYHP = static_cast<U8>(0.5f +
        static_cast<float>(pQPs[3]) * (1.f - qf) + static_cast<float>((pQPs + 6)[3]) * qf);
    pEncoder->WMP.wmiSCP.uiDefaultQPIndexUHP = static_cast<U8>(0.5f +
        static_cast<float>(pQPs[4]) * (1.f - qf) + static_cast<float>((pQPs + 6)[4]) * qf);
    pEncoder->WMP.wmiSCP.uiDefaultQPIndexVHP = static_cast<U8>(0.5f +
        static_cast<float>(pQPs[5]) * (1.f - qf) + static_cast<float>((pQPs + 6)[5]) * qf);
}
