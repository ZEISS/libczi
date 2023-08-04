#include "JxrDecode2.h"
#include <memory>
#include "jxrlib/jxrgluelib/JXRGlue.h"

#include "jxrlib/image/sys/windowsmediaphoto.h"

using namespace std;

static JxrDecode2::PixelFormat JxrPixelFormatGuidToEnum(const GUID& guid);

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
