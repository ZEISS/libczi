#include "JxrDecode2.h"
#include <memory>
#include <stdexcept> 
#include <sstream>
#include "jxrlib/jxrgluelib/JXRGlue.h"

#include "jxrlib/image/sys/windowsmediaphoto.h"

using namespace std;

static void ApplyQuality(float quality, JxrDecode2::PixelFormat pixel_format, std::uint32_t width, PKImageEncode* pEncoder);
static JxrDecode2::PixelFormat JxrPixelFormatGuidToEnum(const GUID& guid);
static void ThrowError(const char* error_message, ERR error_code);
static const char* ERR_to_string(ERR error_code);

void JxrDecode2::Decode(
           const void* ptrData,
           size_t size,
           const std::function<std::tuple<JxrDecode2::PixelFormat, std::uint32_t, void*>(PixelFormat pixel_format, std::uint32_t  width, std::uint32_t  height)>& get_destination_func)
{
    if (ptrData == nullptr) { throw invalid_argument("ptrData"); }
    if (size == 0) { throw invalid_argument("size"); }
    if (!get_destination_func) { throw invalid_argument("get_destination_func"); }

    WMPStream* pStream;
    ERR err = CreateWS_Memory(&pStream, const_cast<void*>(ptrData), size);
    if (Failed(err)) { ThrowError("'CreateWS_Memory' failed", err); }
    unique_ptr<WMPStream, void(*)(WMPStream*)> upStream(pStream, [](WMPStream* p)->void {p->Close(&p); });

    PKImageDecode* pDecoder;
    err = PKCodecFactory_CreateDecoderFromStream(pStream, &pDecoder);
    if (Failed(err)) { ThrowError("'PKCodecFactory_CreateDecoderFromStream' failed", err); }
    std::unique_ptr<PKImageDecode, void(*)(PKImageDecode*)> upDecoder(pDecoder, [](PKImageDecode* p)->void {p->Release(&p); });

    U32 frame_count;
    err = upDecoder->GetFrameCount(upDecoder.get(), &frame_count);
    if (Failed(err)) { ThrowError("'decoder::GetFrameCount' failed", err); }
    if (frame_count != 1)
    {
        ostringstream string_stream;
        string_stream << "Expecting to find a frame_count of 1, but found frame_count = ." << frame_count;
        throw runtime_error(string_stream.str());
    }

    I32 width, height;
    upDecoder->GetSize(upDecoder.get(), &width, &height);
    if (Failed(err)) { ThrowError("'decoder::GetSize' failed", err); }

    PKPixelFormatGUID pixel_format_of_decoder;
    upDecoder->GetPixelFormat(upDecoder.get(), &pixel_format_of_decoder);
    if (Failed(err)) { ThrowError("'decoder::GetPixelFormat' failed", err); }

    const auto decode_info = get_destination_func(
        JxrPixelFormatGuidToEnum(pixel_format_of_decoder),
        width,
        height);

    const PKRect rc{ 0, 0, width, height };
    err = upDecoder->Copy(
        upDecoder.get(),
        &rc,
        static_cast<U8*>(get<2>(decode_info)),
        get<1>(decode_info));
    if (Failed(err)) { ThrowError("decoder::Copy failed", err); }
}

JxrDecode2::CompressedData JxrDecode2::Encode(
           JxrDecode2::PixelFormat pixel_format,
           std::uint32_t width,
           std::uint32_t height,
           std::uint32_t stride,
           const void* ptr_bitmap,
           float quality/*=1.f*/)
{
    PKImageEncode* pEncoder;
    ERR err = PKCodecFactory_CreateCodec(&IID_PKImageWmpEncode, (void**)&pEncoder);

    CWMIStrCodecParam codec_parameters = {};
    //codec_parameters.guidPixFormat = GUID_PKPixelFormat24bppBGR;
    codec_parameters.bVerbose = FALSE;
    codec_parameters.cfColorFormat = YUV_444;
    //    args->bFlagRGB_BGR = FALSE; //default BGR
    codec_parameters.bdBitDepth = BD_LONG;
    codec_parameters.bfBitstreamFormat = FREQUENCY;
    codec_parameters.bProgressiveMode = TRUE;
    codec_parameters.olOverlap = OL_ONE;
    codec_parameters.cNumOfSliceMinus1H = codec_parameters.cNumOfSliceMinus1V = 0;
    codec_parameters.sbSubband = SB_ALL;
    codec_parameters.uAlphaMode = 0;
    codec_parameters.uiDefaultQPIndex = 1;

    codec_parameters.uiDefaultQPIndexAlpha = 1;

    //codec_parameters.fltImageQuality = 1.f;
    //codec_parameters.bOverlapSet = 0;
    //codec_parameters.bColorFormatSet = 0;


   // { &GUID_PKPixelFormat24bppBGR, 3, CF_RGB, BD_8, 24, PK_pixfmtBGR, 2, 3, 8, 1 },

    struct tagWMPStream* pEncodeStream = NULL;
    //CreateWS_File(&pEncodeStream, "C:\\temp\\test.jxr", "wb");
    CreateWS_HeapBackedWriteableStream(&pEncodeStream, 1024, 0);

    err = pEncoder->Initialize(pEncoder, pEncodeStream, &codec_parameters, sizeof(codec_parameters));

    if (quality < 1.f)
    {
        ApplyQuality(quality, pixel_format, width, pEncoder);
    }

    switch (pixel_format)
    {
    case PixelFormat::kBgr24:
        err = pEncoder->SetPixelFormat(pEncoder, GUID_PKPixelFormat24bppBGR);
        break;
    case PixelFormat::kGray8:
        err = pEncoder->SetPixelFormat(pEncoder, GUID_PKPixelFormat8bppGray);
        break;
    case PixelFormat::kBgr48:
        err = pEncoder->SetPixelFormat(pEncoder, GUID_PKPixelFormat48bppRGB);
        break;
    case PixelFormat::kGray16:
        err = pEncoder->SetPixelFormat(pEncoder, GUID_PKPixelFormat16bppGray);
        break;
    }
    //err = pEncoder->SetPixelFormat(pEncoder, GUID_PKPixelFormat24bppBGR);
    err = pEncoder->SetSize(pEncoder, width, height);
    err = pEncoder->SetResolution(pEncoder, 96.f, 96.f);

    pEncoder->WritePixels(pEncoder, height, (U8*)ptr_bitmap, stride);

    return CompressedData(pEncodeStream);
    //pEncodeStream->Close(&pEncodeStream);
}

void JxrDecode2::Decode(
           codecHandle h,
          // const WMPDECAPPARGS* decArgs,
           const void* ptrData,
           size_t size,
           const std::function<PixelFormat(PixelFormat, int width, int height)>& selectDestPixFmt,
           std::function<void(PixelFormat pixFmt, std::uint32_t  width, std::uint32_t  height, std::uint32_t linesCount, const void* ptrData, std::uint32_t stride)> deliverData)
{
    ERR err;
    /*PKFactory* pFactory = nullptr;
    err = PKCreateFactory(&pFactory, PK_SDK_VERSION);
    //if (Failed(err)) { ThrowError("PKCreateFactory failed", err); }
    std::unique_ptr<PKFactory, void(*)(PKFactory*)> upFactory(pFactory, [](PKFactory* p)->void {p->Release(&p); });
    */
    /*PKCodecFactory* pCodecFactory = NULL;
    err = PKCreateCodecFactory(&pCodecFactory, WMP_SDK_VERSION);
    //if (Failed(err)) { ThrowError("PKCreateCodecFactory failed", err); }
    std::unique_ptr<PKCodecFactory, void(*)(PKCodecFactory*)> upCodecFactory(pCodecFactory, [](PKCodecFactory* p)->void {p->Release(&p); });*/

    /*  const PKIID* pIID = NULL;

      GetImageDecodeIID((const char*)".jxr", &pIID);
      PKCodecFactory_CreateCodec(pIID, (void**)&pDecoder);
      std::unique_ptr<PKImageDecode, void(*)(PKImageDecode*)> upDecoder(pDecoder, [](PKImageDecode* p)->void {p->Release(&p); });*/

    WMPStream* pStream;
    err = CreateWS_Memory(&pStream, const_cast<void*>(ptrData), size);
    //unique_ptr<WMPStream, void(*)(WMPStream*)> upStream(pStream, [](WMPStream* p)->void {p->Close(p); p->Release(p); });
    unique_ptr<WMPStream, void(*)(WMPStream*)> upStream(pStream, [](WMPStream* p)->void {p->Close(&p);  });
    //err = pFactory->CreateStreamFromMemory(&pStream, const_cast<void*>(ptrData), size);

    PKImageDecode* pDecoder;
    //upCodecFactory->CreateDecoderFromStream(pStream, &pDecoder);
    PKCodecFactory_CreateDecoderFromStream(pStream, &pDecoder);
    std::unique_ptr<PKImageDecode, void(*)(PKImageDecode*)> upDecoder(pDecoder, [](PKImageDecode* p)->void {p->Release(&p); });


    PKPixelFormatGUID args_guidPixFormat;
    PKPixelInfo PI;

    // take decoder color format and try to look up better one
           // (e.g. 32bppBGR -> 24bppBGR etc.)
    PKPixelInfo newPI;
    newPI.pGUIDPixFmt = PI.pGUIDPixFmt = &upDecoder->guidPixFormat;
    PixelFormatLookup(&newPI, LOOKUP_FORWARD);
    PixelFormatLookup(&newPI, LOOKUP_BACKWARD_TIF);
    args_guidPixFormat = *newPI.pGUIDPixFmt;

    /*if (decArgs->pixFormat == PixelFormat::dontCare)
    {
        if (selectDestPixFmt)
        {
            PI.pGUIDPixFmt = &upDecoder->guidPixFormat;
            I32 w, h;
            upDecoder->GetSize(upDecoder.get(), &w, &h);
            PixelFormat pixFmtFromDecoder = PixelFormatFromPkPixelFormat(upDecoder->guidPixFormat);
            PixelFormat destFmtChosen = selectDestPixFmt(pixFmtFromDecoder, w, h);

            args_guidPixFormat = *PkPixelFormatFromPixelFormat(destFmtChosen);
            //args_guidPixFormat = selectDestPixFmt(*PI.pGUIDPixFmt);
        }
        else
        {
            // take decoder color format and try to look up better one
            // (e.g. 32bppBGR -> 24bppBGR etc.)
            PKPixelInfo newPI;
            newPI.pGUIDPixFmt = PI.pGUIDPixFmt = &upDecoder->guidPixFormat;
            PixelFormatLookup(&newPI, LOOKUP_FORWARD);
            PixelFormatLookup(&newPI, LOOKUP_BACKWARD_TIF);
            args_guidPixFormat = *newPI.pGUIDPixFmt;
        }
    }
    else
    {
        PI.pGUIDPixFmt = PkPixelFormatFromPixelFormat(decArgs->pixFormat);
        args_guidPixFormat = *PI.pGUIDPixFmt;
    }*/

    // == color transcoding,
    if (IsEqualGUID(args_guidPixFormat, GUID_PKPixelFormat8bppGray) || IsEqualGUID(args_guidPixFormat, GUID_PKPixelFormat16bppGray)) { // ** => Y transcoding
        upDecoder->guidPixFormat = args_guidPixFormat;
        upDecoder->WMP.wmiI.cfColorFormat = Y_ONLY;
    }
    else if (IsEqualGUID(args_guidPixFormat, GUID_PKPixelFormat24bppRGB) && upDecoder->WMP.wmiI.cfColorFormat == CMYK) { // CMYK = > RGB
        upDecoder->WMP.wmiI.cfColorFormat = CF_RGB;
        upDecoder->guidPixFormat = args_guidPixFormat;
        upDecoder->WMP.wmiI.bRGB = 1; //RGB
    }

    PixelFormatLookup(&PI, LOOKUP_FORWARD);

    /*std::uint8_t args_uAlphaMode = decArgs->uAlphaMode;
    if (255 == args_uAlphaMode)//user didn't set
    {
        if (!!(PI.grBit & PK_pixfmtHasAlpha))
            args_uAlphaMode = 2;//default is image & alpha for formats with alpha
        else
            args_uAlphaMode = 0;//otherwise, 0
    }*/

    upDecoder->WMP.wmiSCP.bfBitstreamFormat = BITSTREAMFORMAT::SPATIAL;// args.bfBitstreamFormat;   only used for transcoding?

    upDecoder->WMP.wmiSCP.uAlphaMode = 0;// args_uAlphaMode;

    upDecoder->WMP.wmiSCP.sbSubband = SB_ALL;// (SUBBAND)(std::underlying_type<Subband>::type)decArgs->sbSubband;
    upDecoder->WMP.bIgnoreOverlap = FALSE;// decArgs->bIgnoreOverlap ? 1 : 0;

    upDecoder->WMP.wmiI.cfColorFormat = PI.cfColorFormat;

    upDecoder->WMP.wmiI.bdBitDepth = PI.bdBitDepth;
    upDecoder->WMP.wmiI.cBitsPerUnit = PI.cbitUnit;

    //==== Validate thumbnail decode parameters =====
    upDecoder->WMP.wmiI.cThumbnailWidth = upDecoder->WMP.wmiI.cWidth;
    upDecoder->WMP.wmiI.cThumbnailHeight = upDecoder->WMP.wmiI.cHeight;
    upDecoder->WMP.wmiI.bSkipFlexbits = FALSE;
    /*if (args.tThumbnailFactor > 0 && args.tThumbnailFactor != SKIPFLEXBITS) {
        size_t tSize = ((size_t)1 << args.tThumbnailFactor);

        pDecoder->WMP.wmiI.cThumbnailWidth = (pDecoder->WMP.wmiI.cWidth + tSize - 1) / tSize;
        pDecoder->WMP.wmiI.cThumbnailHeight = (pDecoder->WMP.wmiI.cHeight + tSize - 1) / tSize;

        if (pDecoder->WMP.wmiI.cfColorFormat == YUV_420 || pDecoder->WMP.wmiI.cfColorFormat == YUV_422) { // unsupported thumbnail format
            pDecoder->WMP.wmiI.cfColorFormat = YUV_444;
        }
    }
    else if (args.tThumbnailFactor == SKIPFLEXBITS) {
        pDecoder->WMP.wmiI.bSkipFlexbits = TRUE;
    }*/

    /*  if (decArgs->rWidth == 0 || decArgs->rHeight == 0)
      { // no region decode
          upDecoder->WMP.wmiI.cROILeftX = 0;
          upDecoder->WMP.wmiI.cROITopY = 0;
          upDecoder->WMP.wmiI.cROIWidth = upDecoder->WMP.wmiI.cThumbnailWidth;
          upDecoder->WMP.wmiI.cROIHeight = upDecoder->WMP.wmiI.cThumbnailHeight;
      }
      else
      {
          upDecoder->WMP.wmiI.cROILeftX = decArgs->rLeftX;
          upDecoder->WMP.wmiI.cROITopY = decArgs->rTopY;
          upDecoder->WMP.wmiI.cROIWidth = decArgs->rWidth;
          upDecoder->WMP.wmiI.cROIHeight = decArgs->rHeight;
      }*/
    upDecoder->WMP.wmiI.cROILeftX = 0;
    upDecoder->WMP.wmiI.cROITopY = 0;
    upDecoder->WMP.wmiI.cROIWidth = pDecoder->WMP.wmiI.cWidth;
    upDecoder->WMP.wmiI.cROIHeight = upDecoder->WMP.wmiI.cWidth;

    upDecoder->WMP.wmiI.oOrientation = O_NONE;// static_cast<ORIENTATION>(decArgs->oOrientation);

    upDecoder->WMP.wmiI.cPostProcStrength = 0;// decArgs->cPostProcStrength;

    upDecoder->WMP.wmiSCP.bVerbose = 0;

    U32 cFrame;
    err = upDecoder->GetFrameCount(upDecoder.get(), &cFrame);
    //if (Failed(err)) { ThrowError("GetFrameCount failed", err); }
    if (cFrame != 1)
    {
        //throw std::logic_error("Not expecting to find more than one image here.");
    }

    I32 width, height;
    pDecoder->GetSize(pDecoder, &width, &height);
    size_t bytes_per_pixel = pDecoder->WMP.wmiI.cBitsPerUnit / 8;

    PKRect rc;
    rc.X = 0;
    rc.Y = 0;
    rc.Width = width;
    rc.Height = height;

    JxrDecode2::PixelFormat pixel_format = JxrPixelFormatGuidToEnum(pDecoder->guidPixFormat);

    void* pImage = malloc(width * height * bytes_per_pixel);
    upDecoder->Copy(upDecoder.get(), &rc, (U8*)pImage, width * bytes_per_pixel);

    deliverData(/*JxrDecode2::PixelFormat::_24bppBGR*/pixel_format, width, height, height, pImage, width * bytes_per_pixel);
    free(pImage);
}

JxrDecode2::PixelFormat JxrPixelFormatGuidToEnum(const GUID& guid)
{
    if (IsEqualGUID(guid, GUID_PKPixelFormat8bppGray))
    {
        return JxrDecode2::PixelFormat::kGray8;
    }
    else if (IsEqualGUID(guid, GUID_PKPixelFormat16bppGray))
    {
        return JxrDecode2::PixelFormat::kGray16;
    }
    else if (IsEqualGUID(guid, GUID_PKPixelFormat24bppBGR))
    {
        return JxrDecode2::PixelFormat::kBgr24;
    }
    else if (IsEqualGUID(guid, GUID_PKPixelFormat48bppRGB))
    {
        return JxrDecode2::PixelFormat::kBgr48;
    }

    return JxrDecode2::PixelFormat::kInvalid;
}

void ThrowError(const char* error_message, ERR error_code)
{
    ostringstream string_stream;
    if (error_message != nullptr)
    {
        string_stream << "Error in JXR-decoder -> \"" << error_message << "\" code:" << error_code << " (" << ERR_to_string(error_code) << ")";
    }
    else
    {
        string_stream << "Error in JXR-decoder -> " << error_message << " (" << ERR_to_string(error_code) << ")";
    }

    throw runtime_error(string_stream.str());
}

const char* ERR_to_string(ERR error_code)
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

    return "unknown";
}

JxrDecode2::CompressedData::~CompressedData()
{
    if (this->obj_handle_ != nullptr)
    {
        CloseWS_HeapBackedWriteableStream((struct tagWMPStream**)&this->obj_handle_);
    }
}

void* JxrDecode2::CompressedData::GetMemory()
{
    void* data = nullptr;
    GetWS_HeapBackedWriteableStreamBuffer(
        (struct tagWMPStream*)this->obj_handle_,
        &data,
        nullptr);
    return data;
}

size_t JxrDecode2::CompressedData::GetSize()
{
    size_t size = 0;
    GetWS_HeapBackedWriteableStreamBuffer(
        (struct tagWMPStream*)this->obj_handle_,
        nullptr,
        &size);
    return size;
}

/*static*/void ApplyQuality(float quality, JxrDecode2::PixelFormat pixel_format, uint32_t width, PKImageEncode* pEncoder)
{
    // this is resembling the code from https://github.com/ptahmose/jxrlib/blob/f7521879862b9085318e814c6157490dd9dbbdb4/jxrencoderdecoder/JxrEncApp.c#L677C1-L738C10
#if 1
    // optimized for PSNR
    static const int DPK_QPS_420[11][6] = {      // for 8 bit only
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

    static const int DPK_QPS_8[12][6] = {
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
    static const int DPK_QPS_420[11][6] = {      // for 8 bit only
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

    static const int DPK_QPS_8[12][6] = {
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

    static const int DPK_QPS_16[11][6] = {
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

    static const int DPK_QPS_16f[11][6] = {
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
    };

    static const int DPK_QPS_32f[11][6] = {
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

    //if (!args.bOverlapSet)
    //{
        // Image width must be at least 2 MB wide for subsampled chroma and two levels of overlap!
    if (quality >= 0.5F || /*rect.Width*/width < 2 * MB_WIDTH_PIXEL)
        pEncoder->WMP.wmiSCP.olOverlap = OL_ONE;
    else
        pEncoder->WMP.wmiSCP.olOverlap = OL_TWO;
    //}

    //if (!args.bColorFormatSet)
    //{
    if (quality >= 0.5F || /*PI.uBitsPerSample > 8*/(pixel_format == JxrDecode2::PixelFormat::kBgr48 || pixel_format == JxrDecode2::PixelFormat::kGray16))
        pEncoder->WMP.wmiSCP.cfColorFormat = YUV_444;
    else
        pEncoder->WMP.wmiSCP.cfColorFormat = YUV_420;
    //}

    //if (PI.bdBitDepth == BD_1)
    //{
    //    pEncoder->WMP.wmiSCP.uiDefaultQPIndex = (U8)(8 - 5.0F *
    //        quality/*args.fltImageQuality*/ + 0.5F);
    //}
    //else
    {
        // remap [0.8, 0.866, 0.933, 1.0] to [0.8, 0.9, 1.0, 1.1]
        // to use 8-bit DPK QP table (0.933 == Photoshop JPEG 100)
        int qi;
        float qf;
        const int* pQPs;
        if (/*args.fltImageQuality*/quality > 0.8f && /*PI.bdBitDepth == BD_8*/
            (pixel_format == JxrDecode2::PixelFormat::kBgr24 || pixel_format == JxrDecode2::PixelFormat::kGray8) &&
            pEncoder->WMP.wmiSCP.cfColorFormat != YUV_420 &&
            pEncoder->WMP.wmiSCP.cfColorFormat != YUV_422)
            quality/*args.fltImageQuality*/ = 0.8f + (quality/*args.fltImageQuality*/ - 0.8f) * 1.5f;

        qi = (int)(10.f * quality/*args.fltImageQuality*/);
        qf = 10.f * quality/*args.fltImageQuality*/ - (float)qi;

        /*pQPs =
            (pEncoder->WMP.wmiSCP.cfColorFormat == YUV_420 ||
             pEncoder->WMP.wmiSCP.cfColorFormat == YUV_422) ?
            DPK_QPS_420[qi] :
            (PI.bdBitDepth == BD_8 ? DPK_QPS_8[qi] :
            (PI.bdBitDepth == BD_16 ? DPK_QPS_16[qi] :
            (PI.bdBitDepth == BD_16F ? DPK_QPS_16f[qi] :
            DPK_QPS_32f[qi])));*/
        pQPs =
            (pEncoder->WMP.wmiSCP.cfColorFormat == YUV_420 ||
             pEncoder->WMP.wmiSCP.cfColorFormat == YUV_422) ?
            DPK_QPS_420[qi] :
            (pixel_format == JxrDecode2::PixelFormat::kBgr24 || pixel_format == JxrDecode2::PixelFormat::kGray8) ? DPK_QPS_8[qi] :
            ((pixel_format == JxrDecode2::PixelFormat::kBgr48 || pixel_format == JxrDecode2::PixelFormat::kGray16) ? DPK_QPS_16[qi] :
            (DPK_QPS_32f[qi]));

        pEncoder->WMP.wmiSCP.uiDefaultQPIndex = (U8)(0.5f +
                (float)pQPs[0] * (1.f - qf) + (float)(pQPs + 6)[0] * qf);
        pEncoder->WMP.wmiSCP.uiDefaultQPIndexU = (U8)(0.5f +
                (float)pQPs[1] * (1.f - qf) + (float)(pQPs + 6)[1] * qf);
        pEncoder->WMP.wmiSCP.uiDefaultQPIndexV = (U8)(0.5f +
                (float)pQPs[2] * (1.f - qf) + (float)(pQPs + 6)[2] * qf);
        pEncoder->WMP.wmiSCP.uiDefaultQPIndexYHP = (U8)(0.5f +
                (float)pQPs[3] * (1.f - qf) + (float)(pQPs + 6)[3] * qf);
        pEncoder->WMP.wmiSCP.uiDefaultQPIndexUHP = (U8)(0.5f +
                (float)pQPs[4] * (1.f - qf) + (float)(pQPs + 6)[4] * qf);
        pEncoder->WMP.wmiSCP.uiDefaultQPIndexVHP = (U8)(0.5f +
                (float)pQPs[5] * (1.f - qf) + (float)(pQPs + 6)[5] * qf);
    }
}