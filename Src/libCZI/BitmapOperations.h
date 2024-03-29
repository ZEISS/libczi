// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <memory>
#include <algorithm>
#include "libCZI_Pixels.h"

class CBitmapOperations
{
public:
    static int CalcMd5Sum(libCZI::IBitmapData* bm, std::uint8_t* ptrHash, int hashSize);

    static void NNResize(libCZI::IBitmapData* bmSrc, libCZI::IBitmapData* bmDst);

    static void NNResize(libCZI::IBitmapData* bmSrc, libCZI::IBitmapData* bmDest, const libCZI::DblRect& roiSrc, const libCZI::DblRect& roiDst);

    template <typename tFlt>
    struct NNResizeInfo2
    {
        const void* srcPtr;
        int srcStride;
        int srcWidth, srcHeight;
        tFlt srcRoiX, srcRoiY, srcRoiW, srcRoiH;
        void* dstPtr;
        int dstStride;
        int dstWidth, dstHeight;
        tFlt dstRoiX, dstRoiY, dstRoiW, dstRoiH;
    };

    typedef NNResizeInfo2<float> NNResizeInfo2Flt;
    typedef NNResizeInfo2<double> NNResizeInfo2Dbl;

    template <typename tFlt>
    static void NNScale2(libCZI::PixelType tSrcPixelType, libCZI::PixelType tDstPixelType, const NNResizeInfo2<tFlt>& resizeInfo);

    /// This structure gathers the information needed to copy a source bitmap into
    /// a destination bitmap at a specified offset.
    struct CopyWithOffsetInfo
    {
        int xOffset;                        ///< The offset in x direction where the source bitmap is to be placed in the destination bitmap.
        int yOffset;                        ///< The offset in x direction where the source bitmap is to be placed in the destination bitmap.
        libCZI::PixelType srcPixelType;     ///< The pixel type of the source bitmap.
        const void* srcPtr;                 ///< Pointer to the source bitmap.
        int srcStride;                      ///< The stride of the source bitmap in bytes.
        int srcWidth;                       ///< The width of the source bitmap in pixels.
        int srcHeight;                      ///< The height of the source bitmap in pixels.
        libCZI::PixelType dstPixelType;     ///< The pixel type of the destination bitmap.
        void* dstPtr;                       ///< Pointer to the destination bitmap.
        int dstStride;                      ///< The stride of the destination bitmap in bytes.
        int dstWidth;                       ///< The width of the destination bitmap in pixels.
        int dstHeight;                      ///< The height of the destination bitmap in pixels.

        bool drawTileBorder;                ///< If true, a one-pixel wide border is drawn around the copied source bitmap.
    };

    static void CopyWithOffset(const CopyWithOffsetInfo& info);
    static void Copy(libCZI::PixelType srcPixelType, const void* srcPtr, int srcStride, libCZI::PixelType dstPixelType, void* dstPtr, int dstStride, int width, int height, bool drawTileBorder);

    template <libCZI::PixelType tSrcPixelType, libCZI::PixelType tDstPixelType>
    static void Copy(const void* srcPtr, int srcStride, void* dstPtr, int dstStride, int width, int height, bool drawTileBorder);

    template <libCZI::PixelType tSrcPixelType, libCZI::PixelType tDstPixelType, typename tPixelConverter>
    static void Copy(const tPixelConverter& conv, const void* srcPtr, int srcStride, void* dstPtr, int dstStride, int width, int height, bool drawTileBorder);

    template <libCZI::PixelType tSrcDstPixelType>
    static void CopySamePixelType(const void* srcPtr, int srcStride, void* dstPtr, int dstStride, int width, int height, bool drawTileBorder);

    static void Fill(libCZI::IBitmapData* bm, const libCZI::RgbFloatColor& floatColor);

    static void Fill_Gray8(int w, int h, void* ptr, int stride, std::uint8_t val);
    static void Fill_Gray16(int w, int h, void* ptr, int stride, std::uint16_t val);
    static void Fill_Bgr24(int w, int h, void* ptr, int stride, std::uint8_t b, std::uint8_t g, std::uint8_t r);
    static void Fill_Bgra32(int w, int h, void* ptr, int stride, std::uint8_t b, std::uint8_t g, std::uint8_t r, std::uint8_t a);
    static void Fill_Bgr48(int w, int h, void* ptr, int stride, std::uint16_t b, std::uint16_t g, std::uint16_t r);
    static void Fill_GrayFloat(int w, int h, void* ptr, int stride, float v);
    static void RGB48ToBGR48(int w, int h, std::uint16_t* ptr, int stride);

    static std::shared_ptr<libCZI::IBitmapData> ConvertToBigEndian(libCZI::IBitmapData* source);
    static void CopyConvertBigEndian(libCZI::PixelType pixelType, const void* ptrSrc, int srcStride, void* ptrDst, int dstStride, std::uint32_t width, std::uint32_t height);
private:

    template <libCZI::PixelType tSrcPixelType, libCZI::PixelType tDstPixelType, typename tPixelConverter, typename tFlt>
    static void InternalNNScale2(const tPixelConverter& conv, const NNResizeInfo2<tFlt>& resizeInfo);

    template <libCZI::PixelType tSrcPixelType, libCZI::PixelType tDstPixelType, typename tPixelConverter, typename tFlt>
    static void InternalNNScale2(const NNResizeInfo2<tFlt>& resizeInfo)
    {
        tPixelConverter conv;
        InternalNNScale2<tSrcPixelType, tDstPixelType, tPixelConverter>(conv, resizeInfo);
    }

    static void ThrowUnsupportedConversion(libCZI::PixelType srcPixelType, libCZI::PixelType dstPixelType);
};

#include "BitmapOperations.hpp"
