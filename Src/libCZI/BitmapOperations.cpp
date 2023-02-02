// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "stdafx.h"
#include <utility>
#include "BitmapOperations.h"
#include "MD5Sum.h"
#include "utilities.h"
#include "libCZI.h"
#include "Site.h"
#include "inc_libCZI_Config.h"

using namespace libCZI;
using namespace std;

/*static*/int CBitmapOperations::CalcMd5Sum(libCZI::IBitmapData* bm, std::uint8_t* ptrHash, int hashSize)
{
    if (ptrHash == nullptr) { return 16; }
    if (hashSize < 16)
    {
        throw invalid_argument("argument 'hashsize' must be >= 16");
    }

    ScopedBitmapLockerP lck{ bm };

    CMd5Sum md5sum;
    size_t lineLength = bm->GetWidth() * static_cast<size_t>(CziUtils::GetBytesPerPel(bm->GetPixelType()));
#if LIBCZI_ISBIGENDIANHOST
    if (CziUtils::IsPixelTypeEndianessAgnostic(bm->GetPixelType()))
    {
        for (uint32_t y = 0; y < bm->GetHeight(); ++y)
        {
            const std::uint8_t* ptr = ((const std::uint8_t*)lck.ptrDataRoi) + y * ((ptrdiff_t)lck.stride);
            md5sum.update(ptr, lineLength);
        }
    }
    else
    {
        // for compatibility with the little-endian version, we convert pixeltypes for which we have a different byte order - so that the
        //  hashcode gives the same result
        std::unique_ptr<void, decltype(free)*> pLineBuffer(malloc(lineLength), free);
        for (uint32_t y = 0; y < bm->GetHeight(); ++y)
        {
            const std::uint8_t* ptr = ((const std::uint8_t*)lck.ptrDataRoi) + y * ((ptrdiff_t)lck.stride);
            CBitmapOperations::CopyConvertBigEndian(bm->GetPixelType(), ptr, lck.stride, pLineBuffer.get(), lineLength, bm->GetWidth(), 1);
            md5sum.update(pLineBuffer.get(), lineLength);
        }
    }
#else
    for (uint32_t y = 0; y < bm->GetHeight(); ++y)
    {
        const std::uint8_t* ptr = ((const std::uint8_t*)lck.ptrDataRoi) + y * ((ptrdiff_t)lck.stride);
        md5sum.update(ptr, lineLength);
    }
#endif

    md5sum.complete();
    md5sum.getHash(reinterpret_cast<char*>(ptrHash));
    return 16;
}

/*static*/void CBitmapOperations::Copy(libCZI::PixelType srcPixelType, const void* srcPtr, int srcStride, libCZI::PixelType dstPixelType, void* dstPtr, int dstStride, int width, int height, bool drawTileBorder)
{
    switch (srcPixelType)
    {
    case PixelType::Gray8:
        switch (dstPixelType)
        {
        case PixelType::Gray8:
            Copy<PixelType::Gray8, PixelType::Gray8>(srcPtr, srcStride, dstPtr, dstStride, width, height, drawTileBorder);
            return;
        case PixelType::Gray16:
            Copy<PixelType::Gray8, PixelType::Gray16>(srcPtr, srcStride, dstPtr, dstStride, width, height, drawTileBorder);
            return;
        case PixelType::Gray32Float:
            Copy<PixelType::Gray8, PixelType::Gray32Float>(srcPtr, srcStride, dstPtr, dstStride, width, height, drawTileBorder);
            return;
        case PixelType::Bgr24:
            Copy<PixelType::Gray8, PixelType::Bgr24>(srcPtr, srcStride, dstPtr, dstStride, width, height, drawTileBorder);
            return;
        case PixelType::Bgr48:
            Copy<PixelType::Gray8, PixelType::Bgr48>(srcPtr, srcStride, dstPtr, dstStride, width, height, drawTileBorder);
            return;
        default:break;
        }
        break;

    case PixelType::Gray16:
        switch (dstPixelType)
        {
        case PixelType::Gray8:break;
        case PixelType::Gray16:
            Copy<PixelType::Gray16, PixelType::Gray16>(srcPtr, srcStride, dstPtr, dstStride, width, height, drawTileBorder);
            return;
        case PixelType::Gray32Float:break;
        case PixelType::Bgr24:break;
        case PixelType::Bgr48:break;
        default:break;
        }
        break;

    case PixelType::Gray32Float:
        switch (dstPixelType)
        {
        case PixelType::Gray8:break;
        case PixelType::Gray16:break;
        case PixelType::Gray32Float:
            Copy<PixelType::Gray32Float, PixelType::Gray32Float>(srcPtr, srcStride, dstPtr, dstStride, width, height, drawTileBorder);
            return;
        case PixelType::Bgr24:break;
        case PixelType::Bgr48:break;
        default:break;
        }
        break;

    case PixelType::Bgr24:
        switch (dstPixelType)
        {
        case PixelType::Gray8:
            Copy<PixelType::Bgr24, PixelType::Gray8>(srcPtr, srcStride, dstPtr, dstStride, width, height, drawTileBorder);
            return;
        case PixelType::Gray16:
            Copy<PixelType::Bgr24, PixelType::Gray16>(srcPtr, srcStride, dstPtr, dstStride, width, height, drawTileBorder);
            return;
        case PixelType::Gray32Float:
            Copy<PixelType::Bgr24, PixelType::Gray32Float>(srcPtr, srcStride, dstPtr, dstStride, width, height, drawTileBorder);
            return;
        case PixelType::Bgr24:
            Copy<PixelType::Bgr24, PixelType::Bgr24>(srcPtr, srcStride, dstPtr, dstStride, width, height, drawTileBorder);
            return;
        case PixelType::Bgr48:
            Copy<PixelType::Bgr24, PixelType::Bgr48>(srcPtr, srcStride, dstPtr, dstStride, width, height, drawTileBorder);
            return;
        default:break;
        }
        break;

    case PixelType::Bgr48:
        switch (dstPixelType)
        {
        case PixelType::Gray8:break;
        case PixelType::Gray16:break;
        case PixelType::Gray32Float:break;
        case PixelType::Bgr24:break;
        case PixelType::Bgr48:
            Copy<PixelType::Bgr48, PixelType::Bgr48>(srcPtr, srcStride, dstPtr, dstStride, width, height, drawTileBorder);
            return;
        default:break;
        }
        break;

    default:break;
    }

    throw std::logic_error("It seems that this conversion is not implemented...");
}

/*static*/void CBitmapOperations::CopyOffseted(const CopyOffsetedInfo& info)
{
    const IntRect srcRect = IntRect{ info.xOffset,info.yOffset,info.srcWidth,info.srcHeight };
    const IntRect dstRect = IntRect{ 0,0,info.dstWidth,info.dstHeight };
    const IntRect intersection = Utilities::Intersect(srcRect, dstRect);

    if (intersection.w == 0 || intersection.h == 0)
    {
        return;
    }

    void* ptrDestination = static_cast<char*>(info.dstPtr) + intersection.y * static_cast<size_t>(info.dstStride) + intersection.x * static_cast<size_t>(CziUtils::GetBytesPerPel(info.dstPixelType));
    const void* ptrSource = static_cast<const char*>(info.srcPtr) + (std::max)(-info.yOffset, 0) * static_cast<size_t>(info.srcStride) + (std::max)(-info.xOffset, 0) * static_cast<size_t>(CziUtils::GetBytesPerPel(info.srcPixelType));

    Copy(
        info.srcPixelType, ptrSource, info.srcStride,
        info.dstPixelType, ptrDestination, info.dstStride,
        intersection.w, intersection.h,
        info.drawTileBorder);
}

/*static*/void CBitmapOperations::NNResize(libCZI::IBitmapData* bmSrc, libCZI::IBitmapData* bmDest, const DblRect& roiSrc, const DblRect& roiDst)
{
    ScopedBitmapLockerP lckSrc{ bmSrc };
    ScopedBitmapLockerP lckDst{ bmDest };

    NNResizeInfo2Dbl resizeInfo;
    resizeInfo.srcPtr = lckSrc.ptrDataRoi;
    resizeInfo.srcStride = lckSrc.stride;
    resizeInfo.srcRoiX = roiSrc.x;
    resizeInfo.srcRoiY = roiSrc.y;
    resizeInfo.srcRoiW = roiSrc.w;
    resizeInfo.srcRoiH = roiSrc.h;
    resizeInfo.srcWidth = bmSrc->GetWidth();
    resizeInfo.srcHeight = bmSrc->GetHeight();
    resizeInfo.dstPtr = lckDst.ptrDataRoi;
    resizeInfo.dstStride = lckDst.stride;
    resizeInfo.dstRoiX = roiDst.x;
    resizeInfo.dstRoiY = roiDst.y;
    resizeInfo.dstRoiW = roiDst.w;
    resizeInfo.dstRoiH = roiDst.h;
    resizeInfo.dstWidth = bmDest->GetWidth();
    resizeInfo.dstHeight = bmDest->GetHeight();
    NNScale2(bmSrc->GetPixelType(), bmDest->GetPixelType(), resizeInfo);
}

/*static*/void CBitmapOperations::NNResize(libCZI::IBitmapData* bmSrc, libCZI::IBitmapData* bmDst)
{
    if (bmSrc->GetPixelType() != bmDst->GetPixelType())
    {
        throw runtime_error("Currently works only for source and destination having same pixeltype, sorry.");
    }

    ScopedBitmapLockerP lckSrc{ bmSrc };
    ScopedBitmapLockerP lckDst{ bmDst };

    NNResizeInfo2Dbl resizeInfo;
    resizeInfo.srcPtr = lckSrc.ptrDataRoi;
    resizeInfo.srcStride = lckSrc.stride;
    resizeInfo.srcRoiX = 0;
    resizeInfo.srcRoiY = 0;
    resizeInfo.srcRoiW = bmSrc->GetWidth();
    resizeInfo.srcRoiH = bmSrc->GetHeight();
    resizeInfo.srcWidth = bmSrc->GetWidth();
    resizeInfo.srcHeight = bmSrc->GetHeight();
    resizeInfo.dstPtr = lckDst.ptrDataRoi;
    resizeInfo.dstStride = lckDst.stride;
    resizeInfo.dstRoiX = 0;
    resizeInfo.dstRoiY = 0;
    resizeInfo.dstRoiW = bmDst->GetWidth();
    resizeInfo.dstRoiH = bmDst->GetHeight();
    resizeInfo.dstWidth = bmDst->GetWidth();
    resizeInfo.dstHeight = bmDst->GetHeight();

    NNScale2(bmSrc->GetPixelType(), bmDst->GetPixelType(), resizeInfo);
}

/*static*/void CBitmapOperations::Fill(libCZI::IBitmapData* bm, const libCZI::RgbFloatColor& floatColor)
{
    ScopedBitmapLockerP lck{ bm };

    switch (bm->GetPixelType())
    {
    case PixelType::Gray8:
        Fill_Gray8(bm->GetWidth(), bm->GetHeight(), lck.ptrDataRoi, lck.stride, Utilities::clampToByte((255 * (floatColor.r + floatColor.g + floatColor.b)) / 3));
        break;
    case PixelType::Gray16:
        Fill_Gray16(bm->GetWidth(), bm->GetHeight(), lck.ptrDataRoi, lck.stride, Utilities::clampToUShort((65535 * (floatColor.r + floatColor.g + floatColor.b)) / 3));
        break;
    case PixelType::Gray32Float:
        Fill_GrayFloat(bm->GetWidth(), bm->GetHeight(), lck.ptrDataRoi, lck.stride, (floatColor.r + floatColor.g + floatColor.b) / 3);
        break;
    case PixelType::Bgr24:
        Fill_Bgr24(bm->GetWidth(), bm->GetHeight(), lck.ptrDataRoi, lck.stride, Utilities::clampToByte(255 * floatColor.b), Utilities::clampToByte(255 * floatColor.g), Utilities::clampToByte(255 * floatColor.r));
        break;
    case PixelType::Bgra32:
        // Setting a fixed value of 255 to the alpha channel.
        Fill_Bgra32(bm->GetWidth(), bm->GetHeight(), lck.ptrDataRoi, lck.stride, Utilities::clampToByte(255 * floatColor.b), Utilities::clampToByte(255 * floatColor.g), Utilities::clampToByte(255 * floatColor.r), 255);
        break;
    case PixelType::Bgr48:
        Fill_Bgr48(bm->GetWidth(), bm->GetHeight(), lck.ptrDataRoi, lck.stride, Utilities::clampToUShort(65535 * floatColor.b), Utilities::clampToUShort(65535 * floatColor.g), Utilities::clampToUShort(65535 * floatColor.r));
        break;
    default:
        throw runtime_error("Sorry, this pixeltype isn't implemented yet.");
    }
}

/*static*/void CBitmapOperations::Fill_Gray8(int w, int h, void* ptr, int stride, std::uint8_t val)
{
    for (int y = 0; y < h; ++y)
    {
        void* p = static_cast<char*>(ptr) + (y * static_cast<ptrdiff_t>(stride));
        memset(p, val, stride);
    }
}

/*static*/void CBitmapOperations::Fill_Gray16(int w, int h, void* ptr, int stride, std::uint16_t val)
{
    for (int y = 0; y < h; ++y)
    {
        std::uint16_t* p = reinterpret_cast<std::uint16_t*>(static_cast<char*>(ptr) + (y * static_cast<ptrdiff_t>(stride)));
        for (int x = 0; x < w; ++x)
        {
            *(p + x) = val;
        }
    }
}

/*static*/void CBitmapOperations::Fill_Bgr24(int w, int h, void* ptr, int stride, std::uint8_t b, std::uint8_t g, std::uint8_t r)
{
    for (int y = 0; y < h; ++y)
    {
        std::uint8_t* p = reinterpret_cast<std::uint8_t*>(static_cast<char*>(ptr) + (y * static_cast<ptrdiff_t>(stride)));
        for (int x = 0; x < w; ++x)
        {
            *(p + x * 3 + 0) = b;
            *(p + x * 3 + 1) = g;
            *(p + x * 3 + 2) = r;
        }
    }
}

void CBitmapOperations::Fill_Bgra32(int w, int h, void* ptr, int stride, std::uint8_t b, std::uint8_t g, std::uint8_t r, std::uint8_t a)
{
    for (int y = 0; y < h; ++y)
    {
        std::uint8_t* p = reinterpret_cast<std::uint8_t*>(static_cast<char*>(ptr) + (y * static_cast<ptrdiff_t>(stride)));
        for (int x = 0; x < w; ++x)
        {
            *(p + x * 4 + 0) = b;
            *(p + x * 4 + 1) = g;
            *(p + x * 4 + 2) = r;
            *(p + x * 4 + 3) = a;
        }
    }
}

/*static*/void CBitmapOperations::Fill_Bgr48(int w, int h, void* ptr, int stride, std::uint16_t b, std::uint16_t g, std::uint16_t r)
{
    for (int y = 0; y < h; ++y)
    {
        std::uint16_t* p = reinterpret_cast<std::uint16_t*>(static_cast<char*>(ptr) + (y * static_cast<ptrdiff_t>(stride)));
        for (int x = 0; x < w; ++x)
        {
            *(p + x * 3 + 0) = b;
            *(p + x * 3 + 1) = g;
            *(p + x * 3 + 2) = r;
        }
    }
}

/*static*/void CBitmapOperations::Fill_GrayFloat(int w, int h, void* ptr, int stride, float v)
{
    for (int y = 0; y < h; ++y)
    {
        float* p = reinterpret_cast<float*>(static_cast<char*>(ptr) + (y * static_cast<ptrdiff_t>(stride)));
        for (int x = 0; x < w; ++x)
        {
            *(p + x) = v;
        }
    }
}

/*static*/void CBitmapOperations::RGB48ToBGR48(int w, int h, std::uint16_t* ptr, int stride)
{
    for (int y = 0; y < h; ++y)
    {
        std::uint16_t* ptrLine = reinterpret_cast<std::uint16_t*>(reinterpret_cast<char*>(ptr) + y * static_cast<ptrdiff_t>(stride));
        for (int x = 0; x < w; ++x)
        {
            std::swap(ptrLine[0], ptrLine[2]);
            ptrLine += 3;
        }
    }
}

/*static*/void CBitmapOperations::ThrowUnsupportedConversion(libCZI::PixelType srcPixelType, libCZI::PixelType dstPixelType)
{
    stringstream ss;
    ss << "Operation not implemented for source pixeltype='" << libCZI::Utils::PixelTypeToInformalString(srcPixelType) << "' and destination pixeltype='" << libCZI::Utils::PixelTypeToInformalString(dstPixelType) << "'.";
    throw LibCZIException(ss.str().c_str());
}

/*static*/std::shared_ptr<libCZI::IBitmapData> CBitmapOperations::ConvertToBigEndian(libCZI::IBitmapData* source)
{
    auto pxltype = source->GetPixelType();
    auto dst = GetSite()->CreateBitmap(pxltype, source->GetWidth(), source->GetHeight());

    ScopedBitmapLockerP lckSrc{ source };
    ScopedBitmapLockerSP lckDst{ dst };

    CBitmapOperations::CopyConvertBigEndian(pxltype, lckSrc.ptrDataRoi, lckSrc.stride, lckDst.ptrDataRoi, lckDst.stride, source->GetWidth(), source->GetHeight());

    return dst;
}

/*static*/void CBitmapOperations::CopyConvertBigEndian(libCZI::PixelType pixelType, const void* ptrSrc, int srcStride, void* ptrDst, int dstStride, uint32_t width, uint32_t height)
{
    switch (pixelType)
    {
    case PixelType::Gray16:
        for (uint32_t y = 0; y < height; ++y)
        {
            const uint16_t* pSrc = reinterpret_cast<const uint16_t*>(static_cast<const char*>(ptrSrc) + y * static_cast<ptrdiff_t>(srcStride));
            uint16_t* pDst = reinterpret_cast<uint16_t*>(static_cast<char*>(ptrDst) + y * static_cast<ptrdiff_t>(dstStride));
            for (uint32_t x = 0; x < width; ++x)
            {
                *(pDst + x) = ((*(pSrc + x)) << 8) | ((*(pSrc + x)) >> 8);
            }
        }

        break;
    case PixelType::Bgr48:
        for (uint32_t y = 0; y < height; ++y)
        {
            const uint16_t* pSrc = reinterpret_cast<const uint16_t*>(static_cast<const char*>(ptrSrc) + y * static_cast<ptrdiff_t>(srcStride));
            uint16_t* pDst = reinterpret_cast<uint16_t*>(static_cast<char*>(ptrDst) + y * static_cast<ptrdiff_t>(dstStride));
            for (uint32_t x = 0; x < 3 * width; ++x)
            {
                *(pDst + x) = ((*(pSrc + x)) << 8) | ((*(pSrc + x)) >> 8);
            }
        }

        break;
    default:
        throw std::logic_error("Function not yet implemented for the specified pixeltype.");
    }
}
