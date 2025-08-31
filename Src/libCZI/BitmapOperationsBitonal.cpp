#include "BitmapOperationsBitonal.h"

#include <cstdint>
#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "inc_libCZI_Config.h"
#include "BitmapOperations.h"
#include <libCZI_Utilities.h>
#include "utilities.h"

using namespace std;
using namespace libCZI;

namespace
{
    // use this on a little-endian machine (Intel, ARM, ...)
    class CLittleEndianConverter
    {
    public:
        static std::uint32_t BswapDWORD(std::uint32_t dw)
        {
#if LIBCZI_HAS_BUILTIN_BSWAP32
            return __builtin_bswap32(dw);
#elif LIBCZI_HAS_BYTESWAP_IN_STDLIB
            return _byteswap_ulong(dw);
#elif LIBCZI_HAS_BSWAP_LONG_IN_SYS_ENDIAN
            return bswap_32(dw);
#else
            return (((dw & 0xff000000u) >> 24) |
                    ((dw & 0x00ff0000u) >> 8) |
                    ((dw & 0x0000ff00u) << 8) |
                    ((dw & 0x000000ffu) << 24));
#endif
        }
        static std::uint16_t BswapUSHORT(std::uint16_t us)
        {
#if LIBCZI_HAS_BUILTIN_BSWAP32
            return __builtin_bswap16(us);
#elif LIBCZI_HAS_BYTESWAP_IN_STDLIB
            return _byteswap_ushort(us);
#elif LIBCZI_HAS_BSWAP_LONG_IN_SYS_ENDIAN
            return bswap_16(us);
#else
            return (((us & 0xff00u) >> 8) |
                    ((us & 0x00ffu) << 8));
#endif
        }
    };

    // use this on a big-endian machine
    class CBigEndianConverter
    {
    public:
        static std::uint32_t BswapDWORD(std::uint32_t dw) { return dw; }
        static std::uint16_t BswapUSHORT(std::uint16_t us) { return us; }
    };

    typedef
#if LIBCZI_ISBIGENDIANHOST
        CBigEndianConverter
#else
        CLittleEndianConverter
#endif
        EndianConverterForHost;

    template <int regionSize>
    class DecimateHelpers
    {
    protected:
        static std::uint64_t Filter(std::uint64_t value);
        static std::uint32_t GetDword(int y, int height, const void* ptrSrc, int pitchSrc);
        static std::uint32_t GetDwordPartial(int y, int height, const void* ptrSrc, int bitCount, int pitchSrc);
    private:
        static std::uint32_t ReadPartial(const void* ptr, int bitcount);
    };

    template <int tregionSize, class CEndianessConv = EndianConverterForHost>
    class DecimateBitonal : DecimateHelpers<tregionSize>
    {
    public:
        static void Decimate(const void* ptrSrc, int strideSrc, int widthSrc, int heightSrc, void* ptrDest, int strideDest, int widthDest, int heightDest);
    private:
        static const std::uint8_t DecimateBitonalTable[256];
        static void DecimateLine(int y, int height, int regionSize, const std::uint8_t* ptrSrc, int widthSrc, int strideSrc, std::uint8_t* ptrDest, int widthDest, int strideDest);
        static std::uint8_t GetByteAfter(const void* ptr, int x, int width);
        static std::uint16_t FilterDword(std::uint32_t dw, uint8_t byteBefore, uint8_t byteAfter);
    };

    template <int regionSize, class CEndianessConv>
    /*static*/const uint8_t DecimateBitonal<regionSize, CEndianessConv>::DecimateBitonalTable[256] =
    {
        0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3,
        0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3,
        4, 4, 5, 5, 4, 4, 5, 5, 6, 6, 7, 7, 6, 6, 7, 7,
        4, 4, 5, 5, 4, 4, 5, 5, 6, 6, 7, 7, 6, 6, 7, 7,
        0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3,
        0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3,
        4, 4, 5, 5, 4, 4, 5, 5, 6, 6, 7, 7, 6, 6, 7, 7,
        4, 4, 5, 5, 4, 4, 5, 5, 6, 6, 7, 7, 6, 6, 7, 7,
        8, 8, 9, 9, 8, 8, 9, 9, 10, 10, 11, 11, 10, 10, 11, 11,
        8, 8, 9, 9, 8, 8, 9, 9, 10, 10, 11, 11, 10, 10, 11, 11,
        12, 12, 13, 13, 12, 12, 13, 13, 14, 14, 15, 15, 14, 14, 15, 15,
        12, 12, 13, 13, 12, 12, 13, 13, 14, 14, 15, 15, 14, 14, 15, 15,
        8, 8, 9, 9, 8, 8, 9, 9, 10, 10, 11, 11, 10, 10, 11, 11,
        8, 8, 9, 9, 8, 8, 9, 9, 10, 10, 11, 11, 10, 10, 11, 11,
        12, 12, 13, 13, 12, 12, 13, 13, 14, 14, 15, 15, 14, 14, 15, 15,
        12, 12, 13, 13, 12, 12, 13, 13, 14, 14, 15, 15, 14, 14, 15, 15
    };


    template <int tregionSize, class CEndianessConv>
    /*static*/void DecimateBitonal<tregionSize, CEndianessConv>::Decimate(const void* ptrSrc, int strideSrc, int widthSrc, int heightSrc, void* ptrDest, int strideDest, int widthDest, int heightDest)
    {
        for (int y = 0; y < heightDest; ++y)
        {
            DecimateLine(
                y * 2,
                heightSrc,
                1,
                ((const std::uint8_t*)ptrSrc) + strideSrc * y * 2,
                widthSrc,
                strideSrc,
                ((std::uint8_t*)ptrDest) + strideDest * y,
                widthDest,
                strideDest);
        }
    }

    template <int tregionSize, class CEndianessConv>
    /*static*/std::uint16_t DecimateBitonal<tregionSize, CEndianessConv>::FilterDword(std::uint32_t dw, std::uint8_t byteBefore, std::uint8_t byteAfter)
    {
        const std::uint64_t v0 = byteAfter | (((std::uint64_t)dw) << 8) | (((std::uint64_t)byteBefore) << 40);
        const std::uint64_t v = DecimateHelpers<tregionSize>::Filter(v0);
        const std::uint16_t b1 = DecimateBitonalTable[(std::uint8_t)(v >> 8)];
        const std::uint16_t b2 = DecimateBitonalTable[(std::uint8_t)(v >> 16)];
        const std::uint16_t b3 = DecimateBitonalTable[(std::uint8_t)(v >> 24)];
        const std::uint16_t b4 = DecimateBitonalTable[(std::uint8_t)(v >> 32)];
        const std::uint16_t dest = b1 | (b2 << 4) | (b3 << 8) | (b4 << 12);
        return dest;
    }

    template <int tregionSize, class CEndianessConv>
    /*static*/void DecimateBitonal<tregionSize, CEndianessConv>::DecimateLine(int y, int height, int regionSize, const uint8_t* ptrSrc, int widthSrc, int strideSrc, uint8_t* ptrDest, int widthDest, int strideDest)
    {
        uint8_t byteBefore = 0xff;

        int numberOfDwords = widthSrc / 32;

        for (int x = 0; x < numberOfDwords; ++x)
        {
            uint8_t byteAfter = GetByteAfter(ptrSrc, (x * 4) + 4, widthSrc);
            uint32_t dw = CEndianessConv::BswapDWORD(DecimateHelpers<tregionSize>::GetDword(y, height, (void*)(((uintptr_t)ptrSrc) + x * 4), strideSrc)); ///< The double-word
            uint16_t dest = FilterDword(dw, byteBefore, byteAfter);
            byteBefore = (uint8_t)(dw);
            *((uint16_t*)(((uintptr_t)ptrDest) + 2 * x)) = CEndianessConv::BswapUSHORT(dest);
        }

        int bitsRemaining = widthSrc - numberOfDwords * 32;
        if (bitsRemaining > 0)
        {
            // we continue to operate DWORD-wise, we are just careful...
            uint32_t dw = CEndianessConv::BswapDWORD(DecimateHelpers<tregionSize>::GetDwordPartial(y, height, (void*)(((uintptr_t)ptrSrc) + numberOfDwords * 4), bitsRemaining, strideSrc));

            // the filter the DWORD as usually
            uint16_t dest = FilterDword(dw, byteBefore, 0xff);

            if (bitsRemaining <= 16)
            {
                *((uint8_t*)(((uintptr_t)ptrDest) + 2 * numberOfDwords)) = (uint8_t)CEndianessConv::BswapUSHORT(dest);
            }
            else
            {
                *((uint16_t*)(((uintptr_t)ptrDest) + 2 * numberOfDwords)) = CEndianessConv::BswapUSHORT(dest);
            }
        }
    }

    template <int tregionSize, class CEndianessConv>
    uint8_t DecimateBitonal<tregionSize, CEndianessConv>::GetByteAfter(const void* ptr, int x, int width)
    {
        if (x < width)
        {
            return *(((const uint8_t*)ptr) + x);
        }

        return 0xff;
    }

    template <>
    /*static*/uint64_t DecimateHelpers<0>::Filter(uint64_t value)
    {
        return value;
    }

    template <>
    /*static*/uint64_t DecimateHelpers<1>::Filter(uint64_t value)
    {
        return value & (value << 1) & (value >> 1);
    }

    template <>
    /*static*/uint64_t DecimateHelpers<2>::Filter(uint64_t value)
    {
        return value & (value << 1) & (value << 2) & (value >> 1) & (value >> 2);
    }

    template <>
    /*static*/uint64_t DecimateHelpers<3>::Filter(uint64_t value)
    {
        return value & (value << 1) & (value << 2) & (value << 3) & (value >> 1) & (value >> 2) & (value >> 3);
    }

    template <>
    /*static*/uint64_t DecimateHelpers<4>::Filter(uint64_t value)
    {
        return value & (value << 1) & (value << 2) & (value << 3) & (value << 4) & (value >> 1) & (value >> 2) & (value >> 3) & (value >> 4);
    }

    template <>
    /*static*/uint64_t DecimateHelpers<5>::Filter(uint64_t value)
    {
        return value & (value << 1) & (value << 2) & (value << 3) & (value << 4) & (value << 5) & (value >> 1) & (value >> 2) & (value >> 3) & (value >> 4) & (value >> 5);
    }

    template <>
    /*static*/uint64_t DecimateHelpers<6>::Filter(uint64_t value)
    {
        return value & (value << 1) & (value << 2) & (value << 3) & (value << 4) & (value << 5) & (value << 6) & (value >> 1) & (value >> 2) & (value >> 3) & (value >> 4) & (value >> 5) & (value >> 6);
    }

    template <>
    /*static*/uint64_t DecimateHelpers<7>::Filter(uint64_t value)
    {
        return value & (value << 1) & (value << 2) & (value << 3) & (value << 4) & (value << 5) & (value << 6) & (value << 7) & (value >> 1) & (value >> 2) & (value >> 3) & (value >> 4) & (value >> 5) & (value >> 6) & (value >> 7);
    }

    template <>
    /*static*/uint32_t DecimateHelpers<0>::GetDword(int y, int height, const void* ptrSrc, int pitchSrc)
    {
        return *((const uint32_t*)ptrSrc);
    }

    template <>
    /*static*/uint32_t DecimateHelpers<1>::GetDword(int y, int height, const void* ptrSrc, int pitchSrc)
    {
        uint32_t dw = *((const uint32_t*)ptrSrc);
        if (y > 0)
        {
            dw &= *((const uint32_t*)(((uintptr_t)ptrSrc) - pitchSrc));
        }

        if (y < height - 1)
        {
            dw &= *((const uint32_t*)(((uintptr_t)ptrSrc) + pitchSrc));
        }

        return dw;
    }

    template <int regionSize>
    /*static*/uint32_t DecimateHelpers<regionSize>::GetDword(int y, int height, const void* ptrSrc, int pitchSrc)
    {
        uint32_t dw = 0xffffffff;

        int start = (std::max)(y - regionSize, 0);
        int end = (std::min)(y + regionSize, height - 1);

        const uint32_t* ptr = (const uint32_t*)(((uintptr_t)ptrSrc) - ((y - start) * pitchSrc));

        for (int i = start; i <= end; ++i)
        {
            dw &= *ptr;
            ptr = (const uint32_t*)(((uintptr_t)ptr) + pitchSrc);
        }

        return dw;
    }

    template <int regionSize>
    /*static*/uint32_t DecimateHelpers<regionSize>::GetDwordPartial(int y, int height, const void* ptrSrc, int bitCount, int pitchSrc)
    {
        uint32_t dw = 0xffffffff;

        int start = (std::max)(y - regionSize, 0);
        int end = (std::min)(y + regionSize, height - 1);

        const uint32_t* ptr = (const uint32_t*)(((uintptr_t)ptrSrc) - ((y - start) * pitchSrc));

        for (int i = start; i <= end; ++i)
        {
            dw &= ReadPartial(ptr, bitCount);
            ptr = (const uint32_t*)(((uintptr_t)ptr) + pitchSrc);
        }

        return dw;
    }

    template <int regionSize>
    /*static*/uint32_t DecimateHelpers<regionSize>::ReadPartial(const void* ptr, int bitcount)
    {
        if (bitcount <= 8)
        {
            uint8_t b = *((const uint8_t*)ptr);
            b |= (0xff >> (bitcount));
            return 0xffffff00 | b;
        }
        else if (bitcount <= 16)
        {
            uint8_t b1 = *((const uint8_t*)(((uintptr_t)ptr) + 0));
            uint8_t b2 = *((const uint8_t*)(((uintptr_t)ptr) + 1));
            b2 |= (0xff >> (bitcount - 8));
            return 0xffff0000 | (((uint32_t)b2) << 8) | b1;
        }
        else if (bitcount <= 24)
        {
            uint8_t b1 = *((const uint8_t*)(((uintptr_t)ptr) + 0));
            uint8_t b2 = *((const uint8_t*)(((uintptr_t)ptr) + 1));
            uint8_t b3 = *((const uint8_t*)(((uintptr_t)ptr) + 2));
            b3 |= (0xff >> (bitcount - 16));

            return 0xff000000 | (((uint32_t)b3) << 16) | (((uint32_t)b2) << 8) | b1;
        }
        else
        {
            uint32_t dw = *((const uint32_t*)ptr);
            uint8_t m = 0xff >> (bitcount - 24);
            dw |= (((uint32_t)m) << 24);
            return dw;
        }
    }
}

void BitmapOperationsBitonal::BitonalDecimate(
                                        int region_size,
                                        const std::uint8_t* mask_src,
                                        int stride_src,
                                        int width_src,
                                        int height_src,
                                        std::uint8_t* mask_dest,
                                        int stride_dest,
                                        int width_dest,
                                        int height_dest)
{
    // TODO: check arguments!

    switch (region_size)
    {
    case 0:
        DecimateBitonal<0>::Decimate(mask_src, stride_src, width_src, height_src, mask_dest, stride_dest, width_dest, height_dest);
        break;
    case 1:
        DecimateBitonal<1>::Decimate(mask_src, stride_src, width_src, height_src, mask_dest, stride_dest, width_dest, height_dest);
        break;
    case 2:
        DecimateBitonal<2>::Decimate(mask_src, stride_src, width_src, height_src, mask_dest, stride_dest, width_dest, height_dest);
        break;
    case 3:
        DecimateBitonal<3>::Decimate(mask_src, stride_src, width_src, height_src, mask_dest, stride_dest, width_dest, height_dest);
        break;
    case 4:
        DecimateBitonal<4>::Decimate(mask_src, stride_src, width_src, height_src, mask_dest, stride_dest, width_dest, height_dest);
        break;
    case 5:
        DecimateBitonal<5>::Decimate(mask_src, stride_src, width_src, height_src, mask_dest, stride_dest, width_dest, height_dest);
        break;
    case 6:
        DecimateBitonal<6>::Decimate(mask_src, stride_src, width_src, height_src, mask_dest, stride_dest, width_dest, height_dest);
        break;
    case 7:
        DecimateBitonal<7>::Decimate(mask_src, stride_src, width_src, height_src, mask_dest, stride_dest, width_dest, height_dest);
        break;
    default:
        throw invalid_argument("Invalid region size");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace
{
    template <typename tFlt>
    struct NNResizeMaskAwareInfo2
    {
        const void* srcPtr;
        int srcStride;
        int srcWidth, srcHeight;
        const void* srcMaskPtr;
        int srcMaskStride;
        tFlt srcRoiX, srcRoiY, srcRoiW, srcRoiH;
        void* dstPtr;
        int dstStride;
        int dstWidth, dstHeight;
        tFlt dstRoiX, dstRoiY, dstRoiW, dstRoiH;
    };

    typedef NNResizeMaskAwareInfo2<float> NNResizeMaskAwareInfo2Flt;
    typedef NNResizeMaskAwareInfo2<double> NNResizeMaskAwareInfo2Dbl;

    template <libCZI::PixelType tSrcPixelType, libCZI::PixelType tDstPixelType, typename tPixelConverter, typename tFlt>
    void InternalNNScaleMaskAware2(const tPixelConverter& conv, const NNResizeMaskAwareInfo2<tFlt>& resizeInfo)
    {
        constexpr uint8_t bytesPerPelSrc = CziUtils::BytesPerPel<tSrcPixelType>();
        constexpr uint8_t bytesPerPelDest = CziUtils::BytesPerPel<tDstPixelType>();

        const int dstXStart = (std::max)(static_cast<int>(resizeInfo.dstRoiX), 0);
        const int dstXEnd = (std::min)(static_cast<int>(resizeInfo.dstRoiX + resizeInfo.dstRoiW), resizeInfo.dstWidth - 1);

        const int dstYStart = (std::max)(static_cast<int>(resizeInfo.dstRoiY), 0);
        const int dstYEnd = (std::min)(static_cast<int>(resizeInfo.dstRoiY + resizeInfo.dstRoiH), resizeInfo.dstHeight - 1);

        auto yMin = ((0 - resizeInfo.srcRoiY) * resizeInfo.dstRoiH) / (resizeInfo.srcRoiH) + resizeInfo.dstRoiY;
        auto yMax = ((resizeInfo.srcHeight - 1 - resizeInfo.srcRoiY) * (resizeInfo.dstRoiH)) / resizeInfo.srcRoiH + resizeInfo.dstRoiY;
        auto xMin = ((0 - resizeInfo.srcRoiX) * resizeInfo.dstRoiW) / (resizeInfo.srcRoiW) + resizeInfo.dstRoiX;
        auto xMax = ((resizeInfo.srcWidth - 1 - resizeInfo.srcRoiX) * (resizeInfo.dstRoiW)) / resizeInfo.srcRoiW + resizeInfo.dstRoiX;

        const int dstXStartClipped = (std::max)(static_cast<int>(std::ceil(xMin)), dstXStart);
        const int dstXEndClipped = (std::min)(static_cast<int>(std::ceil(xMax)), dstXEnd);
        const int dstYStartClipped = (std::max)(static_cast<int>(std::ceil(yMin)), dstYStart);
        const int dstYEndClipped = (std::min)(static_cast<int>(std::ceil(yMax)), dstYEnd);

        const auto srcWidthOverDstWidth = resizeInfo.srcRoiW / resizeInfo.dstRoiW;
        const auto srcHeightOverDstHeight = resizeInfo.srcRoiH / resizeInfo.dstRoiH;

        for (int y = dstYStartClipped; y <= dstYEndClipped; ++y)
        {
            tFlt srcY = (y - resizeInfo.dstRoiY) * srcHeightOverDstHeight + resizeInfo.srcRoiY;
            long srcYInt = lround(srcY);
            if (srcYInt < 0)
            {
                srcYInt = 0;
            }
            else if (srcYInt >= resizeInfo.srcHeight)
            {
                srcYInt = resizeInfo.srcHeight - 1;
            }

            const char* pSrcLine = (static_cast<const char*>(resizeInfo.srcPtr) + srcYInt * static_cast<size_t>(resizeInfo.srcStride));
            char* pDstLine = static_cast<char*>(resizeInfo.dstPtr) + y * static_cast<size_t>(resizeInfo.dstStride);
            for (int x = dstXStartClipped; x <= dstXEndClipped; ++x)
            {
                // now transform this pixel into the source-ROI
                tFlt srcX = (x - resizeInfo.dstRoiX) * srcWidthOverDstWidth + resizeInfo.srcRoiX;
                long srcXInt = lround(srcX);
                if (srcXInt < 0)
                {
                    srcXInt = 0;
                }
                else if (srcXInt >= resizeInfo.srcWidth)
                {
                    srcXInt = resizeInfo.srcWidth - 1;
                }

                if (BitmapOperationsBitonal::GetPixelFromBitonalUnchecked(srcXInt, srcYInt, resizeInfo.srcMaskPtr, resizeInfo.srcMaskStride))
                {
                    const char* pSrc = pSrcLine + srcXInt * static_cast<size_t>(bytesPerPelSrc);
                    char* pDst = pDstLine + x * static_cast<size_t>(bytesPerPelDest);
                    conv.ConvertPixel(pDst, pSrc);
                }
            }
        }
    }

    template <libCZI::PixelType tSrcPixelType, libCZI::PixelType tDstPixelType, typename tPixelConverter, typename tFlt>
    void InternalNNScaleMaskAware2(const NNResizeMaskAwareInfo2<tFlt>& resizeInfo)
    {
        tPixelConverter conv;
        InternalNNScaleMaskAware2<tSrcPixelType, tDstPixelType, tPixelConverter>(conv, resizeInfo);
    }

    [[noreturn]] void ThrowUnsupportedConversion(libCZI::PixelType srcPixelType, libCZI::PixelType dstPixelType)
    {
        stringstream ss;
        ss << "Operation not implemented for source pixeltype='" << libCZI::Utils::PixelTypeToInformalString(srcPixelType) << "' and destination pixeltype='" << libCZI::Utils::PixelTypeToInformalString(dstPixelType) << "'.";
        throw LibCZIException(ss.str().c_str());
    }

    template <typename tFlt>
    void NNScaleMaskAware2(libCZI::PixelType srcPixelType, libCZI::PixelType dstPixelType, const NNResizeMaskAwareInfo2<tFlt>& resizeInfo)
    {
        switch (srcPixelType)
        {
        case libCZI::PixelType::Gray8:
            switch (dstPixelType)
            {
            case libCZI::PixelType::Gray8:
                InternalNNScaleMaskAware2<libCZI::PixelType::Gray8, libCZI::PixelType::Gray8, CConvGray8ToGray8>(resizeInfo);
                break;
            case libCZI::PixelType::Gray16:
                InternalNNScaleMaskAware2<libCZI::PixelType::Gray8, libCZI::PixelType::Gray16, CConvGray8ToGray16>(resizeInfo);
                break;
            case libCZI::PixelType::Bgr24:
                InternalNNScaleMaskAware2<libCZI::PixelType::Gray8, libCZI::PixelType::Bgr24, CConvGray8ToBgr24>(resizeInfo);
                break;
            case libCZI::PixelType::Bgr48:
                InternalNNScaleMaskAware2<libCZI::PixelType::Gray8, libCZI::PixelType::Bgr48, CConvGray8ToBgr48>(resizeInfo);
                break;
            case libCZI::PixelType::Gray32Float:
                InternalNNScaleMaskAware2<libCZI::PixelType::Gray8, libCZI::PixelType::Gray32Float, CConvGray8ToGray32Float>(resizeInfo);
                break;
            default:
                ThrowUnsupportedConversion(srcPixelType, dstPixelType);
            }

            break;
        case libCZI::PixelType::Bgr24:
            switch (dstPixelType)
            {
            case libCZI::PixelType::Gray8:
                InternalNNScaleMaskAware2<libCZI::PixelType::Bgr24, libCZI::PixelType::Gray8, CConvBgr24ToGray8>(resizeInfo);
                break;
            case libCZI::PixelType::Gray16:
                InternalNNScaleMaskAware2<libCZI::PixelType::Bgr24, libCZI::PixelType::Gray16, CConvBgr24ToGray16>(resizeInfo);
                break;
            case libCZI::PixelType::Gray32Float:
                InternalNNScaleMaskAware2<libCZI::PixelType::Bgr24, libCZI::PixelType::Gray32Float, CConvBgr24ToGray32Float>(resizeInfo);
                break;
            case libCZI::PixelType::Bgr24:
                InternalNNScaleMaskAware2<libCZI::PixelType::Bgr24, libCZI::PixelType::Bgr24, CConvBgr24ToBgr24>(resizeInfo);
                break;
            case libCZI::PixelType::Bgr48:
                InternalNNScaleMaskAware2<libCZI::PixelType::Bgr24, libCZI::PixelType::Bgr48, CConvBgr24ToBgr48>(resizeInfo);
                break;
            default:
                ThrowUnsupportedConversion(srcPixelType, dstPixelType);
            }

            break;
        case libCZI::PixelType::Bgra32:
            switch (dstPixelType)
            {
            case libCZI::PixelType::Bgra32:
                InternalNNScaleMaskAware2<libCZI::PixelType::Bgra32, libCZI::PixelType::Bgra32, CConvBgra32ToBgra32>(resizeInfo);
                break;
            default:
                ThrowUnsupportedConversion(srcPixelType, dstPixelType);
            }

            break;
        case libCZI::PixelType::Gray16:
            switch (dstPixelType)
            {
            case libCZI::PixelType::Gray8:
                InternalNNScaleMaskAware2<libCZI::PixelType::Gray16, libCZI::PixelType::Gray8, CConvGray16ToGray8>(resizeInfo);
                break;
            case libCZI::PixelType::Gray16:
                InternalNNScaleMaskAware2<libCZI::PixelType::Gray16, libCZI::PixelType::Gray16, CConvGray16ToGray16>(resizeInfo);
                break;
            case libCZI::PixelType::Gray32Float:
                InternalNNScaleMaskAware2<libCZI::PixelType::Gray16, libCZI::PixelType::Gray32Float, CConvGray16ToGray32Float>(resizeInfo);
                break;
            case libCZI::PixelType::Bgr24:
                InternalNNScaleMaskAware2<libCZI::PixelType::Gray16, libCZI::PixelType::Bgr24, CConvGray16ToBgr24>(resizeInfo);
                break;
            case libCZI::PixelType::Bgr48:
                InternalNNScaleMaskAware2<libCZI::PixelType::Gray16, libCZI::PixelType::Bgr48, CConvGray16ToBgr48>(resizeInfo);
                break;
            default:
                ThrowUnsupportedConversion(srcPixelType, dstPixelType);
            }

            break;
        case libCZI::PixelType::Bgr48:
            switch (dstPixelType)
            {
            case libCZI::PixelType::Gray8:
                InternalNNScaleMaskAware2<libCZI::PixelType::Bgr48, libCZI::PixelType::Gray8, CConvBgr48ToGray8>(resizeInfo);
                break;
            case libCZI::PixelType::Gray16:
                InternalNNScaleMaskAware2<libCZI::PixelType::Bgr48, libCZI::PixelType::Gray16, CConvBgr48ToGray16>(resizeInfo);
                break;
            case libCZI::PixelType::Gray32Float:
                InternalNNScaleMaskAware2<libCZI::PixelType::Bgr48, libCZI::PixelType::Gray32Float, CConvBgr48ToGray32Float>(resizeInfo);
                break;
            case libCZI::PixelType::Bgr24:
                InternalNNScaleMaskAware2<libCZI::PixelType::Bgr48, libCZI::PixelType::Bgr24, CConvBgr48ToBgr24>(resizeInfo);
                break;
            case libCZI::PixelType::Bgr48:
                InternalNNScaleMaskAware2<libCZI::PixelType::Bgr48, libCZI::PixelType::Bgr48, CConvBgr48ToBgr48>(resizeInfo);
                break;
            default:
                ThrowUnsupportedConversion(srcPixelType, dstPixelType);
            }

            break;
        case libCZI::PixelType::Gray32Float:
            switch (dstPixelType)
            {
            case libCZI::PixelType::Gray32Float:
                InternalNNScaleMaskAware2<libCZI::PixelType::Gray32Float, libCZI::PixelType::Gray32Float, CConvGray32FloatToGray32Float>(resizeInfo);
                break;
            default:
                ThrowUnsupportedConversion(srcPixelType, dstPixelType);
            }

            break;
        default:
            ThrowUnsupportedConversion(srcPixelType, dstPixelType);
        }
    }
}

namespace
{
    struct CopyParameters
    {
        const void* srcPtr;
        int srcStride;
        void* dstPtr;
        int dstStride;
        int width;
        int height;
        const void* srcMaskPtr;
        int srcMaskStride;
        int maskOffsetX;
        int maskOffsetY;
        bool drawTileBorder;
    };

    template <libCZI::PixelType tSrcDstPixelType>
    void CopySamePixelTypeWithMask(const CopyParameters& parameters)
    {
        constexpr auto bytes_per_pel = CziUtils::BytesPerPel<tSrcDstPixelType>();
        const int bytes_to_copy = parameters.width * bytes_per_pel;

        if (parameters.drawTileBorder == false)
        {
            for (int y = 0; y < parameters.height; ++y)
            {
                char* dest = static_cast<char*>(parameters.dstPtr) + y * static_cast<std::ptrdiff_t>(parameters.dstStride);
                const char* src = static_cast<const char*>(parameters.srcPtr) + y * static_cast<std::ptrdiff_t>(parameters.srcStride);

                for (int x = 0; x < parameters.width; ++x)
                {
                    if (BitmapOperationsBitonal::GetPixelFromBitonalUnchecked(x + parameters.maskOffsetX, y + parameters.maskOffsetY, parameters.srcMaskPtr, parameters.srcMaskStride))
                    {
                        char* destPixel = dest + x * bytes_per_pel;
                        const char* srcPixel = src + x * bytes_per_pel;
                        memcpy(destPixel, srcPixel, bytes_per_pel);
                    }
                }
            }
        }
        else
        {
            memset(parameters.dstPtr, 0, bytes_to_copy);
            for (int y = 1; y < parameters.height - 1; ++y)
            {
                char* dest = static_cast<char*>(parameters.dstPtr) + y * static_cast<std::ptrdiff_t>(parameters.dstStride);
                const char* src = static_cast<const char*>(parameters.srcPtr) + y * static_cast<std::ptrdiff_t>(parameters.srcStride);
                memset(dest, 0, bytes_per_pel);
                for (int x = 1; x < parameters.width - 1; ++x)
                {
                    if (BitmapOperationsBitonal::GetPixelFromBitonalUnchecked(x + parameters.maskOffsetX, y + parameters.maskOffsetY, parameters.srcMaskPtr, parameters.srcMaskStride))
                    {
                        char* dest_pixel = dest + x * bytes_per_pel;
                        const char* src_pixel = src + x * bytes_per_pel;
                        memcpy(dest_pixel, src_pixel, bytes_per_pel);
                    }
                }

                memset(dest + bytes_to_copy - bytes_per_pel, 0, bytes_per_pel);
            }

            memset(static_cast<char*>(parameters.dstPtr) + (parameters.height - 1) * static_cast<size_t>(parameters.dstStride), 0, bytes_to_copy);
        }
    }

    template <libCZI::PixelType tSrcPixelType, libCZI::PixelType tDstPixelType, typename tPixelConverter>
    void CopyWithMask(const tPixelConverter& conv, const CopyParameters& parameters)
    {
        // TODO: -implement "drawBorder"
        for (int y = 0; y < parameters.height; ++y)
        {
            char* dest = static_cast<char*>(parameters.dstPtr) + y * static_cast<std::ptrdiff_t>(parameters.dstStride);
            const char* src = static_cast<const char*>(parameters.srcPtr) + y * static_cast<std::ptrdiff_t>(parameters.srcStride);
            for (int x = 0; x < parameters.width; ++x)
            {
                if (BitmapOperationsBitonal::GetPixelFromBitonalUnchecked(x + parameters.maskOffsetX, y + parameters.maskOffsetY, parameters.srcMaskPtr, parameters.srcMaskStride))
                {
                    conv.ConvertPixel(dest, src);
                }
            }

            dest += CziUtils::BytesPerPel<tDstPixelType>();;
            src += CziUtils::BytesPerPel<tSrcPixelType>();;
        }
    }

    template <libCZI::PixelType tSrcPixelType, libCZI::PixelType tDstPixelType> void CopyWithMask(const CopyParameters& parameters);

    template <> void CopyWithMask<libCZI::PixelType::Gray8, libCZI::PixelType::Gray8>(const CopyParameters& parameters)
    {
        CopySamePixelTypeWithMask<libCZI::PixelType::Gray8>(parameters);
    }
    template <> void CopyWithMask<libCZI::PixelType::Gray16, libCZI::PixelType::Gray16>(const CopyParameters& parameters)
    {
        CopySamePixelTypeWithMask<libCZI::PixelType::Gray16>(parameters);
    }
    template <> void CopyWithMask<libCZI::PixelType::Gray32Float, libCZI::PixelType::Gray32Float>(const CopyParameters& parameters)
    {
        CopySamePixelTypeWithMask<libCZI::PixelType::Gray32Float>(parameters);
    }
    template <> void CopyWithMask<libCZI::PixelType::Bgr24, libCZI::PixelType::Bgr24>(const CopyParameters& parameters)
    {
        CopySamePixelTypeWithMask<libCZI::PixelType::Bgr24>(parameters);
    }
    template <> void CopyWithMask<libCZI::PixelType::Bgr48, libCZI::PixelType::Bgr48>(const CopyParameters& parameters)
    {
        CopySamePixelTypeWithMask<libCZI::PixelType::Bgr48>(parameters);
    }
    template <> void CopyWithMask<libCZI::PixelType::Bgr24, libCZI::PixelType::Gray8>(const CopyParameters& parameters)
    {
        CopyWithMask<libCZI::PixelType::Bgr24, libCZI::PixelType::Gray8, CConvBgr24ToGray8>(CConvBgr24ToGray8(), parameters);
    }
    template <> void CopyWithMask<libCZI::PixelType::Gray8, libCZI::PixelType::Bgr24>(const CopyParameters& parameters)
    {
        CopyWithMask<libCZI::PixelType::Gray8, libCZI::PixelType::Bgr24, CConvGray8ToBgr24>(CConvGray8ToBgr24(), parameters);
    }
    template <> void CopyWithMask<libCZI::PixelType::Gray8, libCZI::PixelType::Bgr48>(const CopyParameters& parameters)
    {
        CopyWithMask<libCZI::PixelType::Gray8, libCZI::PixelType::Bgr48, CConvGray8ToBgr48>(CConvGray8ToBgr48(), parameters);
    }
    template <> void CopyWithMask<libCZI::PixelType::Gray8, libCZI::PixelType::Gray16>(const CopyParameters& parameters)
    {
        CopyWithMask<libCZI::PixelType::Gray8, libCZI::PixelType::Gray16, CConvGray8ToGray16>(CConvGray8ToGray16(), parameters);
    }
    template <> void CopyWithMask<libCZI::PixelType::Gray8, libCZI::PixelType::Gray32Float>(const CopyParameters& parameters)
    {
        CopyWithMask<libCZI::PixelType::Gray8, libCZI::PixelType::Gray32Float, CConvGray8ToGray32Float>(CConvGray8ToGray32Float(), parameters);
    }
    template <> void CopyWithMask<libCZI::PixelType::Bgr24, libCZI::PixelType::Gray32Float>(const CopyParameters& parameters)
    {
        CopyWithMask<libCZI::PixelType::Bgr24, libCZI::PixelType::Gray32Float, CConvBgr24ToGray32Float>(CConvBgr24ToGray32Float(), parameters);
    }
    template <> void CopyWithMask<libCZI::PixelType::Bgr24, libCZI::PixelType::Gray16>(const CopyParameters& parameters)
    {
        CopyWithMask<libCZI::PixelType::Bgr24, libCZI::PixelType::Gray16, CConvBgr24ToGray16>(CConvBgr24ToGray16(), parameters);
    }
    template <> void CopyWithMask<libCZI::PixelType::Bgr24, libCZI::PixelType::Bgr48>(const CopyParameters& parameters)
    {
        CopyWithMask<libCZI::PixelType::Bgr24, libCZI::PixelType::Bgr48, CConvBgr24ToBgr48>(CConvBgr24ToBgr48(), parameters);
    }
    template <> void CopyWithMask<libCZI::PixelType::Gray16, libCZI::PixelType::Gray8>(const CopyParameters& parameters)
    {
        CopyWithMask<libCZI::PixelType::Gray16, libCZI::PixelType::Gray8, CConvGray16ToGray8>(CConvGray16ToGray8(), parameters);
    }
    template <> void CopyWithMask<libCZI::PixelType::Gray16, libCZI::PixelType::Gray32Float>(const CopyParameters& parameters)
    {
        CopyWithMask<libCZI::PixelType::Gray16, libCZI::PixelType::Gray32Float, CConvGray16ToGray32Float>(CConvGray16ToGray32Float(), parameters);
    }
    template <> void CopyWithMask<libCZI::PixelType::Gray16, libCZI::PixelType::Bgr24>(const CopyParameters& parameters)
    {
        CopyWithMask<libCZI::PixelType::Gray16, libCZI::PixelType::Bgr24, CConvGray16ToBgr24>(CConvGray16ToBgr24(), parameters);
    }
    template <> void CopyWithMask<libCZI::PixelType::Gray16, libCZI::PixelType::Bgr48>(const CopyParameters& parameters)
    {
        CopyWithMask<libCZI::PixelType::Gray16, libCZI::PixelType::Bgr48, CConvGray16ToBgr48>(CConvGray16ToBgr48(), parameters);
    }
    template <> void CopyWithMask<libCZI::PixelType::Bgr48, libCZI::PixelType::Gray8>(const CopyParameters& parameters)
    {
        CopyWithMask<libCZI::PixelType::Bgr48, libCZI::PixelType::Gray8, CConvBgr48ToGray8>(CConvBgr48ToGray8(), parameters);
    }
    template <> void CopyWithMask<libCZI::PixelType::Bgr48, libCZI::PixelType::Gray16>(const CopyParameters& parameters)
    {
        CopyWithMask<libCZI::PixelType::Bgr48, libCZI::PixelType::Gray16, CConvBgr48ToGray16>(CConvBgr48ToGray16(), parameters);
    }
    template <> void CopyWithMask<libCZI::PixelType::Bgr48, libCZI::PixelType::Gray32Float>(const CopyParameters& parameters)
    {
        CopyWithMask<libCZI::PixelType::Bgr48, libCZI::PixelType::Gray32Float, CConvBgr48ToGray32Float>(CConvBgr48ToGray32Float(), parameters);
    }
    template <> void CopyWithMask<libCZI::PixelType::Bgr48, libCZI::PixelType::Bgr24>(const CopyParameters& parameters)
    {
        CopyWithMask<libCZI::PixelType::Bgr48, libCZI::PixelType::Bgr24, CConvBgr48ToBgr24>(CConvBgr48ToBgr24(), parameters);
    }

    void CopyWithMask(libCZI::PixelType source_pixel_type, libCZI::PixelType destination_pixel_type, const CopyParameters& parameters)
    {
        switch (source_pixel_type)
        {
        case PixelType::Gray8:
            switch (destination_pixel_type)
            {
            case PixelType::Gray8:
                CopyWithMask<PixelType::Gray8, PixelType::Gray8>(parameters);
                return;
            case PixelType::Gray16:
                CopyWithMask<PixelType::Gray8, PixelType::Gray16>(parameters);
                return;
            case PixelType::Gray32Float:
                CopyWithMask<PixelType::Gray8, PixelType::Gray32Float>(parameters);
                return;
            case PixelType::Bgr24:
                CopyWithMask<PixelType::Gray8, PixelType::Bgr24>(parameters);
                return;
            case PixelType::Bgr48:
                CopyWithMask<PixelType::Gray8, PixelType::Bgr48>(parameters);
                return;
            default:break;
            }
            break;

        case PixelType::Gray16:
            switch (destination_pixel_type)
            {
            case PixelType::Gray8:
                CopyWithMask<PixelType::Gray16, PixelType::Gray8>(parameters);
                return;
            case PixelType::Gray16:
                CopyWithMask<PixelType::Gray16, PixelType::Gray16>(parameters);
                return;
            case PixelType::Gray32Float:
                CopyWithMask<PixelType::Gray16, PixelType::Gray32Float>(parameters);
                return;
            case PixelType::Bgr24:
                CopyWithMask<PixelType::Gray16, PixelType::Bgr24>(parameters);
                return;
            case PixelType::Bgr48:
                CopyWithMask<PixelType::Gray16, PixelType::Bgr48>(parameters);
                return;
            default:break;
            }
            break;

        case PixelType::Gray32Float:
            switch (destination_pixel_type)
            {
            case PixelType::Gray8:break;
            case PixelType::Gray16:break;
            case PixelType::Gray32Float:
                CopyWithMask<PixelType::Gray32Float, PixelType::Gray32Float>(parameters);
                return;
            case PixelType::Bgr24:break;
            case PixelType::Bgr48:break;
            default:break;
            }
            break;

        case PixelType::Bgr24:
            switch (destination_pixel_type)
            {
            case PixelType::Gray8:
                CopyWithMask<PixelType::Bgr24, PixelType::Gray8>(parameters);
                return;
            case PixelType::Gray16:
                CopyWithMask<PixelType::Bgr24, PixelType::Gray16>(parameters);
                return;
            case PixelType::Gray32Float:
                CopyWithMask<PixelType::Bgr24, PixelType::Gray32Float>(parameters);
                return;
            case PixelType::Bgr24:
                CopyWithMask<PixelType::Bgr24, PixelType::Bgr24>(parameters);
                return;
            case PixelType::Bgr48:
                CopyWithMask<PixelType::Bgr24, PixelType::Bgr48>(parameters);
                return;
            default:break;
            }
            break;

        case PixelType::Bgr48:
            switch (destination_pixel_type)
            {
            case PixelType::Gray8:
                CopyWithMask<PixelType::Bgr48, PixelType::Gray8>(parameters);
                return;
            case PixelType::Gray16:
                CopyWithMask<PixelType::Bgr48, PixelType::Gray16>(parameters);
                return;
            case PixelType::Gray32Float:
                CopyWithMask<PixelType::Bgr48, PixelType::Gray32Float>(parameters);
                return;
            case PixelType::Bgr24:
                CopyWithMask<PixelType::Bgr48, PixelType::Bgr24>(parameters);
                return;
            case PixelType::Bgr48:
                CopyWithMask<PixelType::Bgr48, PixelType::Bgr48>(parameters);
                return;
            default:break;
            }
            break;
        default:break;
        }

        throw std::logic_error("It seems that this conversion is not implemented...");
    }
}

/*static*/void BitmapOperationsBitonal::NNResizeMaskAware(
    libCZI::IBitmapData* bmSrc,
    libCZI::IBitonalBitmapData* bmSrcMask,
    libCZI::IBitmapData* bmDest,
    const libCZI::DblRect& roiSrc,
    const libCZI::DblRect& roiDst)
{
    // Implement the nearest neighbor resizing with mask awareness 
    ScopedBitmapLockerP lckSrc{ bmSrc };
    ScopedBitmapLockerP lckDst{ bmDest };
    ScopedBitonalBitmapLockerP lckSrcMask{ bmSrcMask };

    NNResizeMaskAwareInfo2Dbl resizeInfo;
    resizeInfo.srcPtr = lckSrc.ptrDataRoi;
    resizeInfo.srcStride = lckSrc.stride;
    resizeInfo.srcMaskPtr = lckSrcMask.ptrData;
    resizeInfo.srcMaskStride = lckSrcMask.stride;
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
    NNScaleMaskAware2<double>(bmSrc->GetPixelType(), bmDest->GetPixelType(), resizeInfo);
}

/*static*/void BitmapOperationsBitonal::CopyWithOffsetAndMask(const CopyWithOffsetAndMaskInfo& info)
{
    if (info.maskPtr == nullptr)
    {
        CBitmapOperations::CopyWithOffset(info);
        return;
    }

    // If we reach here, it means we have a valid maskPtr
    const IntRect src_rect = IntRect{ info.xOffset,info.yOffset,info.srcWidth,info.srcHeight };
    const IntRect dst_rect = IntRect{ 0,0,info.dstWidth,info.dstHeight };
    const IntRect intersection = Utilities::Intersect(src_rect, dst_rect);

    if (intersection.w == 0 || intersection.h == 0)
    {
        return;
    }

    const IntPoint top_left_src_bitmap = IntPoint{ (std::max)(-info.xOffset, 0), (std::max)(-info.yOffset, 0) };
    void* ptr_destination = static_cast<char*>(info.dstPtr) + intersection.y * static_cast<size_t>(info.dstStride) + intersection.x * static_cast<size_t>(CziUtils::GetBytesPerPel(info.dstPixelType));
    const void* ptr_source = static_cast<const char*>(info.srcPtr) + top_left_src_bitmap.y * static_cast<size_t>(info.srcStride) + top_left_src_bitmap.x * static_cast<size_t>(CziUtils::GetBytesPerPel(info.srcPixelType));

    CopyParameters copy_parameters;
    copy_parameters.srcPtr = ptr_source;
    copy_parameters.srcStride = info.srcStride;
    copy_parameters.dstPtr = ptr_destination;
    copy_parameters.dstStride = info.dstStride;
    copy_parameters.width = intersection.w;
    copy_parameters.height = intersection.h;
    copy_parameters.srcMaskPtr = info.maskPtr;
    copy_parameters.srcMaskStride = info.maskStride;
    copy_parameters.maskOffsetX = top_left_src_bitmap.x;
    copy_parameters.maskOffsetY = top_left_src_bitmap.y;
    copy_parameters.drawTileBorder = info.drawTileBorder;

    CopyWithMask(info.srcPixelType, info.dstPixelType, copy_parameters);
}
