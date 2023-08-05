#include "JxrDecode2.h"
#include <memory>
#include <stdexcept> 
#include <sstream>
#include "jxrlib/jxrgluelib/JXRGlue.h"

#include "jxrlib/image/sys/windowsmediaphoto.h"

using namespace std;

static JxrDecode2::PixelFormat JxrPixelFormatGuidToEnum(const GUID& guid);
static void ThrowError(const char* error_message, ERR error_code);
static const char* ERR_to_string(ERR error_code);

void JxrDecode2::Decode(
           const void* ptrData,
           size_t size,
           const std::function<std::tuple<JxrDecode2::PixelFormat, std::uint32_t, void*>(PixelFormat pixel_format, std::uint32_t  width, std::uint32_t  height)>& get_destination_func)
{
    WMPStream* pStream;
    ERR err = CreateWS_Memory(&pStream, const_cast<void*>(ptrData), size);
    if (Failed(err)) { ThrowError("CreateWS_Memory failed", err); }
    unique_ptr<WMPStream, void(*)(WMPStream*)> upStream(pStream, [](WMPStream* p)->void {p->Close(&p);});

    PKImageDecode* pDecoder;
    err = PKCodecFactory_CreateDecoderFromStream(pStream, &pDecoder);
    if (Failed(err)) { ThrowError("PKCodecFactory_CreateDecoderFromStream failed", err); }
    std::unique_ptr<PKImageDecode, void(*)(PKImageDecode*)> upDecoder(pDecoder, [](PKImageDecode* p)->void {p->Release(&p); });

    U32 cFrame;
    err = PKImageDecode_GetFrameCount(upDecoder.get(), &cFrame);

    I32 width, height;
    PKImageDecode_GetSize(upDecoder.get(), &width, &height);

    PKPixelFormatGUID pixel_format_of_decoder;
    PKImageDecode_GetPixelFormat(upDecoder.get(), &pixel_format_of_decoder);

    const auto decode_info = get_destination_func(
        JxrPixelFormatGuidToEnum(pixel_format_of_decoder), 
        width, 
        height);

    PKRect rc;
    rc.X = 0;
    rc.Y = 0;
    rc.Width = width;
    rc.Height = height;
    err = upDecoder->Copy(
        upDecoder.get(),
        &rc,
        static_cast<U8*>(get<2>(decode_info)),
        get<1>(decode_info));
    if (Failed(err)) { ThrowError("PKImageDecode_Copy failed", err); }
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
    std::stringstream ss;
    if (error_message != nullptr)
    {
        ss << "Error in JXR-decoder -> \"" << error_message << "\" code:" << error_code << " (" << ERR_to_string(error_code) << ")";
    }
    else
    {
        ss << "Error in JXR-decoder -> " << error_message << " (" << ERR_to_string(error_code) << ")";
    }

    throw std::runtime_error(ss.str());
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
