// SPDX-FileCopyrightText: 2017-2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "executeCompareImages.h"
#include "cmdlineoptions.h"
#include "executeBase.h"
#include "inc_libCZI.h"
#include "utils.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <deque>
#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

using namespace libCZI;
using namespace std;

namespace
{
/// Identifies a subblock for pairing across two CZIs. Omits pyramidType (legacy; writers may differ)
/// and raw compression id (decoded pixels are compared).
struct SubBlockLayoutKey
{
    int logicalX;
    int logicalY;
    int logicalW;
    int logicalH;
    int physicalW;
    int physicalH;
    PixelType pixelType;
    bool mIndexValid;
    int mIndex;
    std::string coordinateText;

    bool operator<(const SubBlockLayoutKey& o) const
    {
        if (logicalX != o.logicalX)
        {
            return logicalX < o.logicalX;
        }

        if (logicalY != o.logicalY)
        {
            return logicalY < o.logicalY;
        }

        if (logicalW != o.logicalW)
        {
            return logicalW < o.logicalW;
        }

        if (logicalH != o.logicalH)
        {
            return logicalH < o.logicalH;
        }

        if (physicalW != o.physicalW)
        {
            return physicalW < o.physicalW;
        }

        if (physicalH != o.physicalH)
        {
            return physicalH < o.physicalH;
        }

        if (pixelType != o.pixelType)
        {
            return static_cast<int>(pixelType) < static_cast<int>(o.pixelType);
        }

        if (mIndexValid != o.mIndexValid)
        {
            return mIndexValid < o.mIndexValid;
        }

        if (mIndex != o.mIndex)
        {
            return mIndex < o.mIndex;
        }

        return coordinateText < o.coordinateText;
    }
};

SubBlockLayoutKey MakeLayoutKey(const SubBlockInfo& s)
{
    SubBlockLayoutKey k{};
    k.logicalX = s.logicalRect.x;
    k.logicalY = s.logicalRect.y;
    k.logicalW = s.logicalRect.w;
    k.logicalH = s.logicalRect.h;
    k.physicalW = s.physicalSize.w;
    k.physicalH = s.physicalSize.h;
    k.pixelType = s.pixelType;
    k.mIndexValid = s.IsMindexValid();
    k.mIndex = s.mIndex;
    k.coordinateText = Utils::DimCoordinateToString(&s.coordinate);
    return k;
}

std::string DescribeSubBlockShort(const SubBlockInfo& s)
{
    stringstream ss;
    ss << "coord=" << Utils::DimCoordinateToString(&s.coordinate)
       << " rect=" << s.logicalRect.x << "," << s.logicalRect.y << "+" << s.logicalRect.w << "x" << s.logicalRect.h
       << " phys=" << s.physicalSize.w << "x" << s.physicalSize.h
       << " pixel=" << static_cast<int>(s.pixelType);
    if (s.IsMindexValid())
    {
        ss << " M=" << s.mIndex;
    }
    else
    {
        ss << " M=invalid";
    }

    return ss.str();
}

/// For each directory index i in A, sets partnerIndexOut[i] to the index in B with the same layout key.
void BuildSubBlockPartners(
    const ICZIReader& readerA,
    const ICZIReader& readerB,
    int subBlockCount,
    std::vector<int>* partnerIndexOut)
{
    partnerIndexOut->assign(static_cast<size_t>(subBlockCount), -1);
    std::map<SubBlockLayoutKey, std::deque<int>> poolB;
    for (int j = 0; j < subBlockCount; ++j)
    {
        SubBlockInfo infoB;
        if (!readerB.TryGetSubBlockInfo(j, &infoB))
        {
            stringstream ss;
            ss << "CompareImages: TryGetSubBlockInfo(B, " << j << ") failed.";
            throw runtime_error(ss.str());
        }

        poolB[MakeLayoutKey(infoB)].push_back(j);
    }

    for (int i = 0; i < subBlockCount; ++i)
    {
        SubBlockInfo infoA;
        if (!readerA.TryGetSubBlockInfo(i, &infoA))
        {
            stringstream ss;
            ss << "CompareImages: TryGetSubBlockInfo(A, " << i << ") failed.";
            throw runtime_error(ss.str());
        }

        const SubBlockLayoutKey keyA = MakeLayoutKey(infoA);
        auto it = poolB.find(keyA);
        if (it == poolB.end() || it->second.empty())
        {
            stringstream ss;
            ss << "CompareImages: no matching subblock in file B for A index " << i << " (" << DescribeSubBlockShort(infoA) << ").";
            throw runtime_error(ss.str());
        }

        (*partnerIndexOut)[static_cast<size_t>(i)] = it->second.front();
        it->second.pop_front();
    }

    for (const auto& e : poolB)
    {
        if (!e.second.empty())
        {
            SubBlockInfo leftover;
            readerB.TryGetSubBlockInfo(e.second.front(), &leftover);
            stringstream ss;
            ss << "CompareImages: " << static_cast<int>(e.second.size())
               << " subblock(s) in file B have no counterpart in file A (e.g. B index " << e.second.front()
               << ": " << DescribeSubBlockShort(leftover) << ").";
            throw runtime_error(ss.str());
        }
    }
}

float ReadF32(const uint8_t* p)
{
    float f;
    memcpy(&f, p, sizeof(f));
    return f;
}

double ReadF64(const uint8_t* p)
{
    double d;
    memcpy(&d, p, sizeof(d));
    return d;
}

uint16_t ReadU16LE(const uint8_t* p)
{
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

void AccumulatePixelDifference(
    PixelType pt,
    const uint8_t* baseA,
    size_t strideA,
    const uint8_t* baseB,
    size_t strideB,
    uint32_t w,
    uint32_t h,
    long double* sumSq,
    uint64_t* sampleCount,
    long double* maxAbs)
{
    switch (pt)
    {
    case PixelType::Gray8:
        for (uint32_t y = 0; y < h; ++y)
        {
            const uint8_t* ra = baseA + y * strideA;
            const uint8_t* rb = baseB + y * strideB;
            for (uint32_t x = 0; x < w; ++x)
            {
                const long double d = static_cast<long double>(ra[x]) - static_cast<long double>(rb[x]);
                *sumSq += d * d;
                ++*sampleCount;
                if (fabsl(d) > *maxAbs)
                {
                    *maxAbs = fabsl(d);
                }
            }
        }
        break;
    case PixelType::Gray16:
        for (uint32_t y = 0; y < h; ++y)
        {
            const uint8_t* ra = baseA + y * strideA;
            const uint8_t* rb = baseB + y * strideB;
            for (uint32_t x = 0; x < w; ++x)
            {
                const long double va = ReadU16LE(ra + 2 * x);
                const long double vb = ReadU16LE(rb + 2 * x);
                const long double d = va - vb;
                *sumSq += d * d;
                ++*sampleCount;
                if (fabsl(d) > *maxAbs)
                {
                    *maxAbs = fabsl(d);
                }
            }
        }
        break;
    case PixelType::Gray32Float:
        for (uint32_t y = 0; y < h; ++y)
        {
            const uint8_t* ra = baseA + y * strideA;
            const uint8_t* rb = baseB + y * strideB;
            for (uint32_t x = 0; x < w; ++x)
            {
                const long double va = ReadF32(ra + 4 * x);
                const long double vb = ReadF32(rb + 4 * x);
                const long double d = va - vb;
                *sumSq += d * d;
                ++*sampleCount;
                if (fabsl(d) > *maxAbs)
                {
                    *maxAbs = fabsl(d);
                }
            }
        }
        break;
    case PixelType::Bgr24:
        for (uint32_t y = 0; y < h; ++y)
        {
            const uint8_t* ra = baseA + y * strideA;
            const uint8_t* rb = baseB + y * strideB;
            for (uint32_t x = 0; x < w; ++x)
            {
                for (int c = 0; c < 3; ++c)
                {
                    const long double d = static_cast<long double>(ra[3 * x + c]) - static_cast<long double>(rb[3 * x + c]);
                    *sumSq += d * d;
                    ++*sampleCount;
                    if (fabsl(d) > *maxAbs)
                    {
                        *maxAbs = fabsl(d);
                    }
                }
            }
        }
        break;
    case PixelType::Bgr48:
        for (uint32_t y = 0; y < h; ++y)
        {
            const uint8_t* ra = baseA + y * strideA;
            const uint8_t* rb = baseB + y * strideB;
            for (uint32_t x = 0; x < w; ++x)
            {
                for (int c = 0; c < 3; ++c)
                {
                    const long double va = ReadU16LE(ra + 6 * x + 2 * c);
                    const long double vb = ReadU16LE(rb + 6 * x + 2 * c);
                    const long double d = va - vb;
                    *sumSq += d * d;
                    ++*sampleCount;
                    if (fabsl(d) > *maxAbs)
                    {
                        *maxAbs = fabsl(d);
                    }
                }
            }
        }
        break;
    case PixelType::Bgra32:
        for (uint32_t y = 0; y < h; ++y)
        {
            const uint8_t* ra = baseA + y * strideA;
            const uint8_t* rb = baseB + y * strideB;
            for (uint32_t x = 0; x < w; ++x)
            {
                for (int c = 0; c < 4; ++c)
                {
                    const long double d = static_cast<long double>(ra[4 * x + c]) - static_cast<long double>(rb[4 * x + c]);
                    *sumSq += d * d;
                    ++*sampleCount;
                    if (fabsl(d) > *maxAbs)
                    {
                        *maxAbs = fabsl(d);
                    }
                }
            }
        }
        break;
    case PixelType::Bgr96Float:
        for (uint32_t y = 0; y < h; ++y)
        {
            const uint8_t* ra = baseA + y * strideA;
            const uint8_t* rb = baseB + y * strideB;
            for (uint32_t x = 0; x < w; ++x)
            {
                for (int c = 0; c < 3; ++c)
                {
                    const long double va = ReadF32(ra + 12 * x + 4 * c);
                    const long double vb = ReadF32(rb + 12 * x + 4 * c);
                    const long double d = va - vb;
                    *sumSq += d * d;
                    ++*sampleCount;
                    if (fabsl(d) > *maxAbs)
                    {
                        *maxAbs = fabsl(d);
                    }
                }
            }
        }
        break;
    case PixelType::Gray32:
        for (uint32_t y = 0; y < h; ++y)
        {
            const uint8_t* ra = baseA + y * strideA;
            const uint8_t* rb = baseB + y * strideB;
            for (uint32_t x = 0; x < w; ++x)
            {
                int32_t ia, ib;
                memcpy(&ia, ra + 4 * x, sizeof(ia));
                memcpy(&ib, rb + 4 * x, sizeof(ib));
                const long double d = static_cast<long double>(ia) - static_cast<long double>(ib);
                *sumSq += d * d;
                ++*sampleCount;
                if (fabsl(d) > *maxAbs)
                {
                    *maxAbs = fabsl(d);
                }
            }
        }
        break;
    case PixelType::Gray64Float:
        for (uint32_t y = 0; y < h; ++y)
        {
            const uint8_t* ra = baseA + y * strideA;
            const uint8_t* rb = baseB + y * strideB;
            for (uint32_t x = 0; x < w; ++x)
            {
                const long double va = ReadF64(ra + 8 * x);
                const long double vb = ReadF64(rb + 8 * x);
                const long double d = va - vb;
                *sumSq += d * d;
                ++*sampleCount;
                if (fabsl(d) > *maxAbs)
                {
                    *maxAbs = fabsl(d);
                }
            }
        }
        break;
    case PixelType::Gray64ComplexFloat:
        for (uint32_t y = 0; y < h; ++y)
        {
            const uint8_t* ra = baseA + y * strideA;
            const uint8_t* rb = baseB + y * strideB;
            for (uint32_t x = 0; x < w; ++x)
            {
                for (int k = 0; k < 4; ++k)
                {
                    const long double va = ReadF32(ra + 16 * x + 4 * k);
                    const long double vb = ReadF32(rb + 16 * x + 4 * k);
                    const long double d = va - vb;
                    *sumSq += d * d;
                    ++*sampleCount;
                    if (fabsl(d) > *maxAbs)
                    {
                        *maxAbs = fabsl(d);
                    }
                }
            }
        }
        break;
    case PixelType::Bgr192ComplexFloat:
        for (uint32_t y = 0; y < h; ++y)
        {
            const uint8_t* ra = baseA + y * strideA;
            const uint8_t* rb = baseB + y * strideB;
            for (uint32_t x = 0; x < w; ++x)
            {
                for (int k = 0; k < 6; ++k)
                {
                    const long double va = ReadF32(ra + 24 * x + 4 * k);
                    const long double vb = ReadF32(rb + 24 * x + 4 * k);
                    const long double d = va - vb;
                    *sumSq += d * d;
                    ++*sampleCount;
                    if (fabsl(d) > *maxAbs)
                    {
                        *maxAbs = fabsl(d);
                    }
                }
            }
        }
        break;
    default:
        {
            stringstream ss;
            ss << "CompareImages: unsupported pixel type " << static_cast<int>(pt) << ".";
            throw runtime_error(ss.str());
        }
    }
}
} // namespace

class CExecuteCompareImages : CExecuteBase
{
public:
    static bool execute(const CCmdLineOptions& options);
};

bool CExecuteCompareImages::execute(const CCmdLineOptions& options)
{
    auto readerA = CreateAndOpenCziReader(options);
    auto readerB = CreateAndOpenCziReaderForPath(options.GetCompareCziFilename().c_str(), options);

    const SubBlockStatistics statsA = readerA->GetStatistics();
    const SubBlockStatistics statsB = readerB->GetStatistics();
    if (statsA.subBlockCount != statsB.subBlockCount)
    {
        stringstream ss;
        ss << "CompareImages: subblock count differs (A=" << statsA.subBlockCount << ", B=" << statsB.subBlockCount << ").";
        throw runtime_error(ss.str());
    }

    if (statsA.subBlockCount <= 0)
    {
        throw runtime_error("CompareImages: no subblocks in source A.");
    }

    vector<int> partnerBIndexByAIndex;
    BuildSubBlockPartners(*readerA, *readerB, statsA.subBlockCount, &partnerBIndexByAIndex);

    const shared_ptr<ILog> log = options.GetLog();
    bool reordered = false;
    for (int i = 0; i < statsA.subBlockCount; ++i)
    {
        if (partnerBIndexByAIndex[static_cast<size_t>(i)] != i)
        {
            reordered = true;
            break;
        }
    }

    if (reordered)
    {
        log->WriteLineStdOut("CompareImages: subblocks paired by tile layout (directory order differs between the two files).");
    }

    long double sumSqTotal = 0;
    uint64_t sampleTotal = 0;
    long double maxAbsGlobal = 0;
    int worstIndex = 0;
    long double worstBlockRms = -1;

    for (int i = 0; i < statsA.subBlockCount; ++i)
    {
        const int j = partnerBIndexByAIndex[static_cast<size_t>(i)];
        const auto sbA = readerA->ReadSubBlock(i);
        const auto sbB = readerB->ReadSubBlock(j);
        if (sbA == nullptr || sbB == nullptr)
        {
            stringstream ss;
            ss << "CompareImages: ReadSubBlock failed (A index " << i << ", B index " << j << ").";
            throw runtime_error(ss.str());
        }

        const shared_ptr<IBitmapData> bmA = sbA->CreateBitmap(nullptr);
        const shared_ptr<IBitmapData> bmB = sbB->CreateBitmap(nullptr);
        const ScopedBitmapLockerSP lockA(bmA);
        const ScopedBitmapLockerSP lockB(bmB);

        if (bmA->GetPixelType() != bmB->GetPixelType() || bmA->GetWidth() != bmB->GetWidth() || bmA->GetHeight() != bmB->GetHeight())
        {
            throw runtime_error("CompareImages: decoded bitmap shape or type mismatch.");
        }

        const PixelType pt = bmA->GetPixelType();
        const uint32_t w = bmA->GetWidth();
        const uint32_t h = bmA->GetHeight();
        const size_t bpp = Utils::GetBytesPerPixel(pt);
        const size_t minStride = static_cast<size_t>(w) * bpp;
        if (lockA.stride < minStride || lockB.stride < minStride)
        {
            throw runtime_error("CompareImages: decoded bitmap stride too small for width and pixel type.");
        }

        long double sumSqBlock = 0;
        uint64_t samplesBlock = 0;
        long double maxAbsBlock = 0;
        AccumulatePixelDifference(
            pt,
            static_cast<const uint8_t*>(lockA.ptrDataRoi),
            lockA.stride,
            static_cast<const uint8_t*>(lockB.ptrDataRoi),
            lockB.stride,
            w,
            h,
            &sumSqBlock,
            &samplesBlock,
            &maxAbsBlock);

        sumSqTotal += sumSqBlock;
        sampleTotal += samplesBlock;
        if (maxAbsBlock > maxAbsGlobal)
        {
            maxAbsGlobal = maxAbsBlock;
        }

        const long double blockRms = (samplesBlock > 0)
                                         ? sqrtl(static_cast<long double>(sumSqBlock) / static_cast<long double>(samplesBlock))
                                         : 0;
        if (options.IsLogLevelEnabled(4) || options.IsLogLevelEnabled(5))
        {
            stringstream line;
            line << "CompareImages: subblock A[" << i << "]<->B[" << j << "] RMSD=" << fixed << setprecision(8) << static_cast<double>(blockRms)
                 << " max_abs=" << static_cast<double>(maxAbsBlock);
            log->WriteLineStdOut(line.str());
        }

        if (worstBlockRms < 0 || blockRms > worstBlockRms)
        {
            worstBlockRms = blockRms;
            worstIndex = i;
        }
    }

    const long double overallRms = (sampleTotal > 0)
                                       ? sqrtl(static_cast<long double>(sumSqTotal) / static_cast<long double>(sampleTotal))
                                       : 0;

    stringstream summary;
    summary << "CompareImages: compared " << statsA.subBlockCount << " subblock(s), " << sampleTotal
            << " scalar sample(s)." << '\n';
    summary << "  Overall RMSD: " << fixed << setprecision(10) << static_cast<double>(overallRms) << '\n';
    summary << "  Max absolute sample difference: " << setprecision(10) << static_cast<double>(maxAbsGlobal) << '\n';
    summary << "  Largest per-subblock RMSD at index " << worstIndex << ": " << setprecision(10) << static_cast<double>(worstBlockRms) << '\n';
    summary << "  Source A: " << convertToUtf8(options.GetCZIFilename()) << '\n';
    summary << "  Source B: " << convertToUtf8(options.GetCompareCziFilename());
    log->WriteLineStdOut(summary.str());

    if (sumSqTotal == 0)
    {
        log->WriteLineStdOut("CompareImages: decoded pixels match exactly (RMSD is zero).");
    }

    return true;
}

bool executeCompareImages(const CCmdLineOptions& options)
{
    return CExecuteCompareImages::execute(options);
}
