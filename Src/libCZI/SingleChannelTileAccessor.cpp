// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SingleChannelTileAccessor.h"
#include "CziUtils.h"
#include "utilities.h"
#include "SingleChannelTileCompositor.h"
#include "Site.h"
#include "bitmapData.h"

using namespace libCZI;
using namespace libCZI::detail;
using namespace std;

CSingleChannelTileAccessor::CSingleChannelTileAccessor(const std::shared_ptr<ISubBlockRepository>& sbBlkRepository)
    : CSingleChannelAccessorBase(sbBlkRepository)
{
}

/*virtual*/std::shared_ptr<libCZI::IBitmapData> CSingleChannelTileAccessor::Get(const libCZI::IntRectAndFrameOfReference& roi, const IDimCoordinate* planeCoordinate, const Options* pOptions)
{
    // first, we need to determine the pixeltype, which we do from the repository
    libCZI::PixelType pixelType;
    const bool b = this->TryGetPixelType(planeCoordinate, pixelType);
    if (b == false)
    {
        throw LibCZIAccessorException("Unable to determine the pixeltype.", LibCZIAccessorException::ErrorType::CouldntDeterminePixelType);
    }

    return this->Get(pixelType, roi, planeCoordinate, pOptions);
}

/*virtual*/std::shared_ptr<libCZI::IBitmapData> CSingleChannelTileAccessor::Get(libCZI::PixelType pixeltype, const  libCZI::IntRectAndFrameOfReference& roi, const IDimCoordinate* planeCoordinate, const Options* pOptions)
{
    const IntRect roi_raw_sub_block_cs = this->sbBlkRepository->TransformRectangle(roi, CZIFrameOfReference::RawSubBlockCoordinateSystem).rectangle;

    // A whole-slide reader commonly asks for exactly one layer-zero stored
    // tile.  Returning that decoded bitmap directly avoids a destination
    // allocation and a compositor copy.  Edge/padded tiles, overlays, and
    // pixel-type conversions retain the existing general compositor path.
    int matchingIndex = -1;
    int matchingCount = 0;
    bool hasOtherSubblock = false;
    this->sbBlkRepository->EnumSubset(
        planeCoordinate, &roi_raw_sub_block_cs, true,
        [&](int index, const SubBlockInfo& info)->bool
        {
            if (pOptions != nullptr && pOptions->sceneFilter != nullptr)
            {
                int sceneIndex;
                if (info.coordinate.TryGetPosition(DimensionIndex::S, &sceneIndex) &&
                    !pOptions->sceneFilter->IsContained(sceneIndex))
                {
                    return true;
                }
            }

            const bool isExactLayerZeroTile =
                info.logicalRect.x == roi_raw_sub_block_cs.x &&
                info.logicalRect.y == roi_raw_sub_block_cs.y &&
                info.logicalRect.w == roi_raw_sub_block_cs.w &&
                info.logicalRect.h == roi_raw_sub_block_cs.h &&
                // A minified pyramid subblock has the same logical rectangle
                // but fewer stored pixels.  It must go through the compositor
                // so the returned bitmap retains the requested ROI dimensions.
                info.physicalSize.w == static_cast<std::uint32_t>(roi_raw_sub_block_cs.w) &&
                info.physicalSize.h == static_cast<std::uint32_t>(roi_raw_sub_block_cs.h);
            if (isExactLayerZeroTile)
            {
                matchingIndex = index;
                ++matchingCount;
            }
            else
            {
                // A partially overlapping tile, another pyramid level, or a
                // different stored representation requires compositing.
                hasOtherSubblock = true;
            }

            return matchingCount < 2 && !hasOtherSubblock;
        });

    if (matchingCount == 1 && !hasOtherSubblock &&
        !(pOptions != nullptr && (pOptions->maskAware || pOptions->drawTileBorder)))
    {
        const auto subblockData = CSingleChannelAccessorBase::GetSubBlockDataIncludingMaskForSubBlockIndex(
            this->sbBlkRepository,
            pOptions ? pOptions->subBlockCache : nullptr,
            matchingIndex,
            pOptions ? pOptions->onlyUseSubBlockCacheForCompressedData : true,
            false);
        if (subblockData.bitmap->GetPixelType() == pixeltype)
        {
            return subblockData.bitmap;
        }
    }

    auto bmDest = GetSite()->CreateBitmap(pixeltype, roi_raw_sub_block_cs.w, roi_raw_sub_block_cs.h);
    this->InternalGet(roi_raw_sub_block_cs.x, roi_raw_sub_block_cs.y, bmDest.get(), planeCoordinate, pOptions);
    return bmDest;
}

/*virtual*/void CSingleChannelTileAccessor::Get(libCZI::IBitmapData* pDest, const IntPointAndFrameOfReference& position, const IDimCoordinate* planeCoordinate, const Options* pOptions)
{
    const IntPoint point_raw_sub_block_cs = this->sbBlkRepository->TransformPoint(position, CZIFrameOfReference::RawSubBlockCoordinateSystem).point;
    this->InternalGet(point_raw_sub_block_cs.x, point_raw_sub_block_cs.y, pDest, planeCoordinate, pOptions);
}

void CSingleChannelTileAccessor::ComposeTiles(libCZI::IBitmapData* pBm, int xPos, int yPos, const std::vector<IndexAndM>& subBlocksSet, const ISingleChannelTileAccessor::Options& options)
{
    Compositors::ComposeSingleTileOptions composeOptions;
    composeOptions.Clear();
    composeOptions.drawTileBorder = options.drawTileBorder;

    if (options.useVisibilityCheckOptimization)
    {
        // Try to reduce the number of subblocks to be rendered by doing a visibility check, and only rendering those which are visible.
        // We report the subblocks in the order as they are given in the vector 'subBlocksSet', the lambda will be called with the
        // argument 'index' counting down from subBlocksSet.size()-1 to 0. The subblock index we report for 'index=0' is the first one
        // to be rendered, and 'index=subBlocksSet.size()-1' is the last one to be rendered (on top of all the others).
        // We get a vector with the indices of the subblocks to be rendered, and then render them in the order as given in this vector 
        // (index here means - the number as passed to the lambda).
        const auto indices_of_visible_tiles = this->CheckForVisibility(
            { xPos, yPos, static_cast<int>(pBm->GetWidth()), static_cast<int>(pBm->GetHeight()) },
            static_cast<int>(subBlocksSet.size()),
            [&](int index)->int
            {
                return subBlocksSet[index].index;
            });


        Compositors::ComposeSingleChannelTilesMaskAware(
                [&](int index, std::shared_ptr<libCZI::IBitmapData>& spBm, std::shared_ptr<libCZI::IBitonalBitmapData>& spMask, int& xPosTile, int& yPosTile)->bool
                {
                    if (index < static_cast<int>(indices_of_visible_tiles.size()))
                    {
                        const auto subblock_data = CSingleChannelAccessorBase::GetSubBlockDataIncludingMaskForSubBlockIndex(
                            this->sbBlkRepository,
                            options.subBlockCache,
                            subBlocksSet[indices_of_visible_tiles[index]].index,
                            options.onlyUseSubBlockCacheForCompressedData,
                            options.maskAware);
                        spBm = subblock_data.bitmap;
                        spMask = subblock_data.mask;
                        xPosTile = subblock_data.subBlockInfo.logicalRect.x;
                        yPosTile = subblock_data.subBlockInfo.logicalRect.y;
                        return true;
                    }

                    return false;
                },
                pBm,
                xPos,
                yPos,
                &composeOptions);
    }
    else
    {
        Compositors::ComposeSingleChannelTilesMaskAware(
            [&](int index, std::shared_ptr<libCZI::IBitmapData>& spBm, std::shared_ptr<libCZI::IBitonalBitmapData>& spMask, int& xPosTile, int& yPosTile)->bool
            {
                if (index < static_cast<int>(subBlocksSet.size()))
                {
                    const auto subblock_data = CSingleChannelAccessorBase::GetSubBlockDataIncludingMaskForSubBlockIndex(
                        this->sbBlkRepository,
                        options.subBlockCache,
                        subBlocksSet[index].index,
                        options.onlyUseSubBlockCacheForCompressedData,
                        options.maskAware);
                    spBm = subblock_data.bitmap;
                    spMask = subblock_data.mask;
                    xPosTile = subblock_data.subBlockInfo.logicalRect.x;
                    yPosTile = subblock_data.subBlockInfo.logicalRect.y;
                    return true;
                }

                return false;
            },
            pBm,
            xPos,
            yPos,
            &composeOptions);
    }
}

void CSingleChannelTileAccessor::InternalGet(int xPos, int yPos, libCZI::IBitmapData* pBm, const IDimCoordinate* planeCoordinate, const ISingleChannelTileAccessor::Options* pOptions)
{
    if (pOptions == nullptr)
    {
        ISingleChannelTileAccessor::Options options; options.Clear();
        this->InternalGet(xPos, yPos, pBm, planeCoordinate, &options);
        return;
    }

    this->CheckPlaneCoordinates(planeCoordinate);
    Clear(pBm, pOptions->backGroundColor);
    const IntSize sizeBm = pBm->GetSize();
    const IntRect roi{ xPos,yPos,static_cast<int>(sizeBm.w),static_cast<int>(sizeBm.h) };
    const std::vector<IndexAndM> subBlocksSet = this->GetSubBlocksSubset(roi, planeCoordinate, pOptions->sceneFilter.get(), pOptions->sortByM);

    this->ComposeTiles(pBm, xPos, yPos, subBlocksSet, *pOptions);
}

std::vector<CSingleChannelTileAccessor::IndexAndM> CSingleChannelTileAccessor::GetSubBlocksSubset(const IntRect& roi, const IDimCoordinate* planeCoordinate, const IIndexSet* sceneFilter, bool sortByM)
{
    // ok... for a first tentative, experimental and quick-n-dirty implementation, simply
    // get all subblocks by enumerating all
    std::vector<IndexAndM> subBlocksSet;
    this->GetAllSubBlocks(roi, planeCoordinate, sceneFilter, [&](int index, int mIndex)->void {subBlocksSet.emplace_back(IndexAndM{ index,mIndex }); });
    if (sortByM == true)
    {
        // sort ascending-by-M-index (-> lowest M-index first, highest last)
        std::sort(subBlocksSet.begin(), subBlocksSet.end(), [](const IndexAndM& i1, const IndexAndM& i2)->bool
            {
                // an invalid mIndex should go before a valid one (just to have a deterministic sorting) - and "invalid mIndex" is represented by both maximum int and minimum int
                const int mIndex1 = Utils::IsValidMindex(i1.mIndex) ? i1.mIndex : (numeric_limits<int>::min)();
                const int mIndex2 = Utils::IsValidMindex(i2.mIndex) ? i2.mIndex : (numeric_limits<int>::min)();
                return mIndex1 < mIndex2;
            });
    }

    return subBlocksSet;
}

void CSingleChannelTileAccessor::GetAllSubBlocks(const IntRect& roi, const IDimCoordinate* planeCoordinate, const IIndexSet* sceneFilter, const std::function<void(int index, int mIndex)>& appender) const
{
    const auto callback = [&](int idx, const SubBlockInfo& info)->bool
    {
        if (sceneFilter != nullptr)
        {
            int sceneIndex;
            if (info.coordinate.TryGetPosition(DimensionIndex::S, &sceneIndex) &&
                !sceneFilter->IsContained(sceneIndex))
            {
                return true;
            }
        }
        if (Utilities::DoIntersect(roi, info.logicalRect))
        {
            appender(idx, info.mIndex);
        }
        return true;
    };
    auto optimizedRepository = dynamic_cast<ISubBlockRepositorySubsetEx*>(this->sbBlkRepository.get());
    if (optimizedRepository != nullptr)
    {
        optimizedRepository->EnumSubsetEx(planeCoordinate, &roi, true, sceneFilter, callback);
    }
    else
    {
        this->sbBlkRepository->EnumSubset(planeCoordinate, &roi, true, callback);
    }
}
