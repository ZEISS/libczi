// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <cmath>
#include "SingleChannelPyramidLevelTileAccessor.h"
#include "utilities.h"
#include "Site.h"

using namespace libCZI;
using namespace std;

CSingleChannelPyramidLevelTileAccessor::CSingleChannelPyramidLevelTileAccessor(const std::shared_ptr<ISubBlockRepository>& sbBlkRepository)
    : CSingleChannelAccessorBase(sbBlkRepository)
{
}

/*virtual*/std::shared_ptr<libCZI::IBitmapData> CSingleChannelPyramidLevelTileAccessor::Get(const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, const PyramidLayerInfo& pyramidInfo, const ISingleChannelPyramidLayerTileAccessor::Options* pOptions)
{
    if (pOptions == nullptr)
    {
        Options opt;
        opt.Clear();
        return this->Get(roi, planeCoordinate, pyramidInfo, &opt);
    }

    libCZI::PixelType pixelType;
    const bool b = this->TryGetPixelType(planeCoordinate, pixelType);
    if (b == false)
    {
        throw LibCZIAccessorException("Unable to determine the pixeltype.", LibCZIAccessorException::ErrorType::CouldntDeterminePixelType);
    }

    return this->Get(pixelType, roi, planeCoordinate, pyramidInfo, pOptions);
}

/*virtual*/std::shared_ptr<libCZI::IBitmapData> CSingleChannelPyramidLevelTileAccessor::Get(libCZI::PixelType pixeltype, const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, const PyramidLayerInfo& pyramidInfo, const libCZI::ISingleChannelPyramidLayerTileAccessor::Options* pOptions)
{
    if (pOptions == nullptr)
    {
        Options opt;
        opt.Clear();
        return this->Get(pixeltype, roi, planeCoordinate, pyramidInfo, &opt);
    }

    const int sizeOfPixel = CalcSizeOfPixelOnLayer0(pyramidInfo);
    const IntSize sizeOfBitmap{ static_cast<std::uint32_t>(roi.w / sizeOfPixel),static_cast<std::uint32_t>(roi.h / sizeOfPixel) };
    if (sizeOfBitmap.w == 0 || sizeOfBitmap.h == 0)
    {
        // TODO
        throw runtime_error("error");
    }

    auto bmDest = GetSite()->CreateBitmap(pixeltype, sizeOfBitmap.w, sizeOfBitmap.h);
    this->InternalGet(bmDest.get(), roi.x, roi.y, sizeOfPixel, planeCoordinate, pyramidInfo, *pOptions);
    return bmDest;
}

/*virtual*/void CSingleChannelPyramidLevelTileAccessor::Get(libCZI::IBitmapData* pDest, int xPos, int yPos, const libCZI::IDimCoordinate* planeCoordinate, const PyramidLayerInfo& pyramidInfo, const Options* pOptions)
{
    if (pOptions == nullptr)
    {
        Options opt;
        opt.Clear();
        this->Get(pDest, xPos, yPos, planeCoordinate, pyramidInfo, &opt);
        return;
    }

    const int sizeOfPixel = CalcSizeOfPixelOnLayer0(pyramidInfo);
    this->InternalGet(pDest, xPos, yPos, sizeOfPixel, planeCoordinate, pyramidInfo, *pOptions);
}

void CSingleChannelPyramidLevelTileAccessor::InternalGet(libCZI::IBitmapData* pDest, int xPos, int yPos, int sizeOfPixelOnLayer0, const libCZI::IDimCoordinate* planeCoordinate, const PyramidLayerInfo& pyramidInfo, const Options& options)
{
    this->CheckPlaneCoordinates(planeCoordinate);
    Clear(pDest, options.backGroundColor);
    const auto sizeBitmap = pDest->GetSize();
    const auto subSet = GetSubBlocksSubset(IntRect{ xPos,yPos,static_cast<int>(sizeBitmap.w) * sizeOfPixelOnLayer0,static_cast<int>(sizeBitmap.h) * sizeOfPixelOnLayer0 }, planeCoordinate, pyramidInfo, options.sceneFilter.get(), options.sortByM);
    if (subSet.empty())
    {	// no subblocks were found in the requested plane/ROI, so there is nothing to do
        return;
    }

    const auto byLayer = CalcByLayer(subSet, pyramidInfo.minificationFactor);
    // ok, now we just have to look at our requested pyramid-layer
    const auto& indices = byLayer.at(pyramidInfo.pyramidLayerNo).indices;

    // and now... copy...
    this->ComposeTiles(pDest, xPos, yPos, sizeOfPixelOnLayer0, static_cast<int>(indices.size()), options,
        [&](int idx)->SbInfo
        {
            return subSet.at(indices.at(idx));
        });
}

void CSingleChannelPyramidLevelTileAccessor::ComposeTiles(libCZI::IBitmapData* bm, int xPos, int yPos, int sizeOfPixel, int bitmapCnt, const Options& options, const std::function<SbInfo(int)>& getSbInfo)
{
    Compositors::ComposeSingleTileOptions composeOptions; composeOptions.Clear();
    composeOptions.drawTileBorder = options.drawTileBorder;

    Compositors::ComposeSingleChannelTiles(
        [&](int index, std::shared_ptr<libCZI::IBitmapData>& spBm, int& xPosTile, int& yPosTile)->bool
        {
            if (index < bitmapCnt)
            {
                const SbInfo sbinfo = getSbInfo(index);
                const auto subblock_bitmap_data = CSingleChannelAccessorBase::GetSubBlockDataForSubBlockIndex(
                        this->sbBlkRepository,
                        options.subBlockCache,
                        sbinfo.index,
                        options.onlyUseSubBlockCacheForCompressedData);
                spBm = subblock_bitmap_data.bitmap;
                xPosTile = (subblock_bitmap_data.subBlockInfo.logicalRect.x - xPos) / sizeOfPixel;
                yPosTile = (subblock_bitmap_data.subBlockInfo.logicalRect.y - yPos) / sizeOfPixel;
                return true;
            }

            return false;
        },
        bm,
        0,
        0,
        &composeOptions);
}

libCZI::IntRect CSingleChannelPyramidLevelTileAccessor::CalcDestinationRectFromSourceRect(const libCZI::IntRect& roi, const PyramidLayerInfo& pyramidInfo)
{
    const int p = CalcSizeOfPixelOnLayer0(pyramidInfo);
    const int w = roi.w / p;
    const int h = roi.h / p;
    return IntRect{ roi.x, roi.y, w, h };
}

libCZI::IntRect CSingleChannelPyramidLevelTileAccessor::NormalizePyramidRect(int x, int y, int w, int h, const PyramidLayerInfo& pyramidInfo)
{
    const int p = CSingleChannelPyramidLevelTileAccessor::CalcSizeOfPixelOnLayer0(pyramidInfo);
    return IntRect{ x,y,w * p,h * p };
}

/// <summary>	For the specified pyramid layer (and the pyramid type), calculate the size of a pixel on this
/// 			layer as measured by pixels on layer 0. </summary>
/// <param name="pyramidInfo">	Information describing the pyramid and the requested pyramid-layer. </param>
/// <returns>	The calculated size of pixel (in units of pixels on pyramid layer 0). </returns>
/*static*/int CSingleChannelPyramidLevelTileAccessor::CalcSizeOfPixelOnLayer0(const PyramidLayerInfo& pyramidInfo)
{
    int f = 1;
    for (int i = 0; i < pyramidInfo.pyramidLayerNo; ++i)
    {
        f *= pyramidInfo.minificationFactor;
    }

    return f;
}

std::map<int, CSingleChannelPyramidLevelTileAccessor::SbByLayer> CSingleChannelPyramidLevelTileAccessor::CalcByLayer(const std::vector<SbInfo>& sbinfos, int minificationFactor)
{
    std::map<int, CSingleChannelPyramidLevelTileAccessor::SbByLayer> result;
    for (size_t i = 0; i < sbinfos.size(); ++i)
    {
        const SbInfo& sbinfo = sbinfos.at(i);
        int pyrLayer = this->CalcPyramidLayerNo(sbinfo.logicalRect, sbinfo.physicalSize, minificationFactor);
        result[pyrLayer].indices.push_back(static_cast<int>(i));
    }

    return result;
}

int CSingleChannelPyramidLevelTileAccessor::CalcPyramidLayerNo(const libCZI::IntRect& logicalRect, const libCZI::IntSize& physicalSize, int minificationFactorPerLayer)
{
    double minFactor;
    if (physicalSize.w > physicalSize.h)
    {
        minFactor = static_cast<double>(logicalRect.w) / physicalSize.w;
    }
    else
    {
        minFactor = static_cast<double>(logicalRect.h) / physicalSize.h;
    }

    const int minFactorInt = static_cast<int>(round(minFactor));
    int f = 1;
    int layerNo = -1;
    for (int layer = 0;; layer++)
    {
        if (f >= minFactorInt)
        {
            layerNo = layer;
            break;
        }

        f *= minificationFactorPerLayer;
    }

    if (GetSite()->IsEnabled(LOGLEVEL_CHATTYINFORMATION))
    {
        stringstream ss;
        ss << "Logical=(" << logicalRect.x << "," << logicalRect.y << "," << logicalRect.w << "," << logicalRect.h << ") size=(" <<
            physicalSize.w << "," << physicalSize.h << ") minFactorPerLayer=" << minificationFactorPerLayer <<
            " minFact=" << minFactor << "[" << minFactorInt << "]" << " -> Layer:" << layerNo;
        GetSite()->Log(LOGLEVEL_CHATTYINFORMATION, ss);
    }

    return layerNo;
}

std::vector<CSingleChannelPyramidLevelTileAccessor::SbInfo> CSingleChannelPyramidLevelTileAccessor::GetSubBlocksSubset(const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, const PyramidLayerInfo& pyramidInfo, const libCZI::IIndexSet* sceneFilter, bool sortByM)
{
    std::vector<CSingleChannelPyramidLevelTileAccessor::SbInfo> sblks;
    this->GetAllSubBlocks(roi, planeCoordinate, sceneFilter,
        [&](const SbInfo& info)->void
        {
            sblks.emplace_back(info);
        });
    if (sortByM)
    {
        // sort ascending-by-M-index (-> lowest M-index first, highest last)
        std::sort(sblks.begin(), sblks.end(), [](const CSingleChannelPyramidLevelTileAccessor::SbInfo& i1, const CSingleChannelPyramidLevelTileAccessor::SbInfo& i2)->bool
            {
                // an invalid mIndex should go before a valid one (just to have a deterministic sorting) - and "invalid mIndex" is represented by both maximum int and minimum int
                const int mIndex1 = Utils::IsValidMindex(i1.mIndex) ? i1.mIndex : (numeric_limits<int>::min)();
                const int mIndex2 = Utils::IsValidMindex(i2.mIndex) ? i2.mIndex : (numeric_limits<int>::min)();
                return mIndex1 < mIndex2;
            });
    }
    return sblks;
}

/// Enumerate all sub blocks on the specified plane which intersect with the specified ROI - irrespective of their zoom.
///
/// \param roi									   The region-of-interest.
/// \param planeCoordinate						   The plane coordinate.
/// \param sceneFilter							   An optional filter selecting scenes.
/// \param appender								   A functor which will called passing in subblocks matching the conditions.
void CSingleChannelPyramidLevelTileAccessor::GetAllSubBlocks(const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, const libCZI::IIndexSet* sceneFilter, const std::function<void(const SbInfo&)>& appender) const
{
    this->sbBlkRepository->EnumSubset(planeCoordinate, nullptr, false,
        [&](int idx, const SubBlockInfo& info)->bool
        {
            if (sceneFilter != nullptr)
            {
                int indexS;
                if (info.coordinate.TryGetPosition(DimensionIndex::S, &indexS) == true)
                {
                    if (!sceneFilter->IsContained(indexS))
                    {
                        return true;
                    }
                }
            }

            if (Utilities::DoIntersect(roi, info.logicalRect))
            {
                SbInfo sbinfo;
                sbinfo.logicalRect = info.logicalRect;
                sbinfo.physicalSize = info.physicalSize;
                sbinfo.mIndex = info.mIndex;
                sbinfo.index = idx;
                appender(sbinfo);
            }

            return true;
        });
}
