// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SingleChannelScalingTileAccessor.h"
#include "utilities.h"
#include "BitmapOperations.h"
#include "Site.h"

using namespace libCZI;
using namespace std;

CSingleChannelScalingTileAccessor::CSingleChannelScalingTileAccessor(const std::shared_ptr<ISubBlockRepository>& sbBlkRepository)
    : CSingleChannelAccessorBase(sbBlkRepository)
{
}

/*virtual*/libCZI::IntSize CSingleChannelScalingTileAccessor::CalcSize(const libCZI::IntRect& roi, float zoom) const
{
    return InternalCalcSize(roi, zoom);
}

/*virtual*/ std::shared_ptr<libCZI::IBitmapData> CSingleChannelScalingTileAccessor::Get(const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, float zoom, const libCZI::ISingleChannelScalingTileAccessor::Options* pOptions)
{
    if (pOptions == nullptr)
    {
        Options opt; opt.Clear();
        return this->Get(roi, planeCoordinate, zoom, &opt);
    }

    libCZI::PixelType pixelType;
    const bool b = this->TryGetPixelType(planeCoordinate, pixelType);
    if (b == false)
    {
        throw LibCZIAccessorException("Unable to determine the pixeltype.", LibCZIAccessorException::ErrorType::CouldntDeterminePixelType);
    }

    return this->Get(pixelType, roi, planeCoordinate, zoom, pOptions);
}

/*virtual*/std::shared_ptr<libCZI::IBitmapData> CSingleChannelScalingTileAccessor::Get(libCZI::PixelType pixeltype, const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, float zoom, const libCZI::ISingleChannelScalingTileAccessor::Options* pOptions)
{
    if (pOptions == nullptr)
    {
        Options opt; opt.Clear();
        return this->Get(pixeltype, roi, planeCoordinate, zoom, &opt);
    }

    const IntSize sizeOfBitmap = InternalCalcSize(roi, zoom);
    auto bmDest = GetSite()->CreateBitmap(pixeltype, sizeOfBitmap.w, sizeOfBitmap.h);
    this->InternalGet(bmDest.get(), roi, planeCoordinate, zoom, *pOptions);
    return bmDest;
}

/*virtual*/void CSingleChannelScalingTileAccessor::Get(libCZI::IBitmapData* pDest, const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, float zoom, const libCZI::ISingleChannelScalingTileAccessor::Options* pOptions)
{
    if (pOptions == nullptr)
    {
        Options opt; opt.Clear();
        return this->Get(pDest, roi, planeCoordinate, zoom, &opt);
    }

    const IntSize sizeOfBitmap = InternalCalcSize(roi, zoom);
    if (sizeOfBitmap.w != pDest->GetWidth() || sizeOfBitmap.h != pDest->GetHeight())
    {
        stringstream ss;
        ss << "The specified bitmap has a size of " << pDest->GetWidth() << "*" << pDest->GetHeight() << ", whereas the expected size is " << sizeOfBitmap.w << "*" << sizeOfBitmap.h << ".";
        throw invalid_argument(ss.str().c_str());
    }

    this->InternalGet(pDest, roi, planeCoordinate, zoom, *pOptions);
}

// ----------------------------------------------------------------------------------------------------------------------

/*static*/libCZI::IntSize CSingleChannelScalingTileAccessor::InternalCalcSize(const libCZI::IntRect& roi, float zoom)
{
    return IntSize{ static_cast<uint32_t>(roi.w * zoom),static_cast<uint32_t>(roi.h * zoom) };
}

void CSingleChannelScalingTileAccessor::ScaleBlt(libCZI::IBitmapData* bmDest, float zoom, const libCZI::IntRect& roi, const SbInfo& sbInfo, const libCZI::ISingleChannelScalingTileAccessor::Options& options)
{
    auto subblock_bitmap_data = CSingleChannelAccessorBase::GetSubBlockDataForSubBlockIndex(
        this->sbBlkRepository,
        options.subBlockCache,
        sbInfo.index,
        options.onlyUseSubBlockCacheForCompressedData);
    if (GetSite()->IsEnabled(LOGLEVEL_CHATTYINFORMATION))
    {
        stringstream ss;
        ss << "   bounds: " << Utils::DimCoordinateToString(&subblock_bitmap_data.subBlockInfo.coordinate) << " M=" << (Utils::IsValidMindex(subblock_bitmap_data.subBlockInfo.mIndex) ? to_string(subblock_bitmap_data.subBlockInfo.mIndex) : "invalid");
        GetSite()->Log(LOGLEVEL_CHATTYINFORMATION, ss);
    }

    const auto& source = subblock_bitmap_data.bitmap;

    // In order not to run into trouble with floating point precision, if the scale is exactly 1, we refrain from using the scaling operation
    //  and do instead a simple copy operation. This should ensure a pixel-accurate result if zoom is exactly 1.
    if (zoom == 1)
    {
        ScopedBitmapLockerSP srcLck{ source };
        ScopedBitmapLockerP dstLck{ bmDest };
        CBitmapOperations::CopyWithOffsetInfo info;
        info.xOffset = sbInfo.logicalRect.x - roi.x;
        info.yOffset = sbInfo.logicalRect.y - roi.y;
        info.srcPixelType = source->GetPixelType();
        info.srcPtr = srcLck.ptrDataRoi;
        info.srcStride = srcLck.stride;
        info.srcWidth = source->GetWidth();
        info.srcHeight = source->GetHeight();
        info.dstPixelType = bmDest->GetPixelType();
        info.dstPtr = dstLck.ptrDataRoi;
        info.dstStride = dstLck.stride;
        info.dstWidth = bmDest->GetWidth();
        info.dstHeight = bmDest->GetHeight();
        info.drawTileBorder = false;

        CBitmapOperations::CopyWithOffset(info);
    }
    else
    {
        // calculate the intersection of the subblock (logical rect) and the destination
        const auto intersect = Utilities::Intersect(sbInfo.logicalRect, roi);

        const double roiSrcTopLeftX = static_cast<double>(intersect.x - sbInfo.logicalRect.x) / sbInfo.logicalRect.w;
        const double roiSrcTopLeftY = static_cast<double>(intersect.y - sbInfo.logicalRect.y) / sbInfo.logicalRect.h;
        const double roiSrcBttmRightX = static_cast<double>(intersect.x + intersect.w - sbInfo.logicalRect.x) / sbInfo.logicalRect.w;
        const double roiSrcBttmRightY = static_cast<double>(intersect.y + intersect.h - sbInfo.logicalRect.y) / sbInfo.logicalRect.h;

        const double destTopLeftX = static_cast<double>(intersect.x - roi.x) / roi.w;
        const double destTopLeftY = static_cast<double>(intersect.y - roi.y) / roi.h;
        const double destBttmRightX = static_cast<double>(intersect.x + intersect.w - roi.x) / roi.w;
        const double destBttmRightY = static_cast<double>(intersect.y + intersect.h - roi.y) / roi.h;

        DblRect srcRoi{ roiSrcTopLeftX ,roiSrcTopLeftY,roiSrcBttmRightX - roiSrcTopLeftX ,roiSrcBttmRightY - roiSrcTopLeftY };
        DblRect dstRoi{ destTopLeftX ,destTopLeftY,destBttmRightX - destTopLeftX ,destBttmRightY - destTopLeftY };

        srcRoi.x *= sbInfo.physicalSize.w;
        srcRoi.y *= sbInfo.physicalSize.h;
        srcRoi.w *= sbInfo.physicalSize.w;
        srcRoi.h *= sbInfo.physicalSize.h;

        dstRoi.x *= bmDest->GetWidth();
        dstRoi.y *= bmDest->GetHeight();
        dstRoi.w *= bmDest->GetWidth();
        dstRoi.h *= bmDest->GetHeight();

        CBitmapOperations::NNResize(source.get(), bmDest, srcRoi, dstRoi);
    }
}

int CSingleChannelScalingTileAccessor::GetIdxOf1stSubBlockWithZoomGreater(const std::vector<SbInfo>& sbBlks, const std::vector<int>& byZoom, float zoom)
{
    // now, skip until the zoom of the subBlock is greater than the specified zoom
    for (size_t i = 0; i < byZoom.size(); ++i)
    {
        if (sbBlks.at(byZoom.at(i)).GetZoom() >= zoom)
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}

/// <summary>	
/// Create an vector with indices (into the specified vector with SubBlock-infos) so that the indices give 
/// the items sorted by their "zoom"-factor. (A zoom of "1" means that the subblock is on layer-0). Subblocks of
/// a higher pyramid-layer are at the end of the list.
/// </summary>
/// <param name="sbBlks">	The vector of subblock-infos for which to create the sorted indices. </param>
/// <param name="sortByM">	Whether to sort the subblocks by their zoom level AND the M-Index or only by zoom level. </param>
/// <returns>	A vector with indices which give the subblocks sorted by their zoom (biggest zoom first). </returns>
std::vector<int> CSingleChannelScalingTileAccessor::CreateSortByZoom(const std::vector<SbInfo>& sbBlks, bool sortByM)
{
    std::vector<int> byZoom;
    byZoom.reserve(sbBlks.size());
    for (size_t i = 0; i < sbBlks.size(); ++i)
    {
        byZoom.emplace_back(static_cast<int>(i));
    }

    if (sortByM)
    {
        std::sort(
            byZoom.begin(),
            byZoom.end(),
            [&](const int i1, const int i2)->bool
            {
                const auto& sb1 = sbBlks.at(i1);
                const auto& sb2 = sbBlks.at(i2);
                const auto zoom1 = sb1.GetZoom();
                const auto zoom2 = sb2.GetZoom();
                if (zoom1 < zoom2)
                {
                    return true;
                }
                else if (zoom1 > zoom2 ||
                            sb1.logicalRect.w != sb1.physicalSize.w ||  // if the logical rect is not the same as the physical size, then the subblock is not on layer-0
                            sb1.logicalRect.h != sb1.physicalSize.h ||  // and we want to apply the "sorting by M-index" only for layer-0
                            sb2.logicalRect.w != sb2.physicalSize.w ||
                            sb2.logicalRect.h != sb2.physicalSize.h)
                {
                    return false;
                }

                // an invalid mIndex should go before a valid one (just to have a deterministic sorting) - and "invalid mIndex" is represented by both maximum int and minimum int
                const int mIndex1 = Utils::IsValidMindex(sb1.mIndex) ? sb1.mIndex : (numeric_limits<int>::min)();
                const int mIndex2 = Utils::IsValidMindex(sb2.mIndex) ? sb2.mIndex : (numeric_limits<int>::min)();

                return mIndex1 < mIndex2;
            });
    }
    else
    {
        // Sort by zoom only - note that we use "stable_sort" here, because otherwise the order of subblocks with the same zoom-level would be arbitrary.
        // This would mean that the result is not idem-potent, i.e. if we call this function twice with the same input, we would get different results.
        // This is not a problem for the "sort by M-index" case, because there we have a deterministic sorting.
        // With "stable_sort" we ensure that the order of subblocks with the same zoom-level is preserved. This randomness was actually observed
        // in case of with stdlibc++ - with MSVC on Windows, the order was always the same.
        std::stable_sort(byZoom.begin(), byZoom.end(), [&](const int i1, const int i2)->bool {return sbBlks.at(i1).GetZoom() < sbBlks.at(i2).GetZoom(); });
    }
    return byZoom;
}

std::vector<CSingleChannelScalingTileAccessor::SbInfo> CSingleChannelScalingTileAccessor::GetSubSet(const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, const std::vector<int>* allowedScenes)
{
    std::vector<SbInfo> sblks;
    this->sbBlkRepository->EnumSubset(planeCoordinate, &roi, false,
        [&](int idx, const SubBlockInfo& info)->bool
        {
            if (allowedScenes != nullptr)
            {
                int sIndex;
                if (info.coordinate.TryGetPosition(DimensionIndex::S, &sIndex))
                {
                    if (find(allowedScenes->cbegin(), allowedScenes->cend(), sIndex) == allowedScenes->cend())
                    {
                        // if there is a set of "allowedScenes" given, and the subblock has a valid S-index, and it is not found in the
                        //  set of allowed scenes, then we need to discard this subblock (need to return true in order to continue the enumeration)
                        return true;
                    }
                }
            }

            SbInfo sbinfo;
            sbinfo.logicalRect = info.logicalRect;
            sbinfo.physicalSize = info.physicalSize;
            sbinfo.mIndex = info.mIndex;
            sbinfo.index = idx;
            sblks.push_back(sbinfo);
            return true;
        });

    return sblks;
}

void CSingleChannelScalingTileAccessor::InternalGet(libCZI::IBitmapData* bmDest, const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, float zoom, const libCZI::ISingleChannelScalingTileAccessor::Options& options)
{
    this->CheckPlaneCoordinates(planeCoordinate);
    Clear(bmDest, options.backGroundColor);
    std::vector<int> scenesInvolved = this->DetermineInvolvedScenes(roi, options.sceneFilter.get());

    if (GetSite()->IsEnabled(LOGLEVEL_CHATTYINFORMATION))
    {
        stringstream ss;
        ss << "SingleChannelScalingTileAccessor -> Plane: " << Utils::DimCoordinateToString(planeCoordinate) << " Requested ROI: " << roi << " Zoom: " << zoom;
        GetSite()->Log(LOGLEVEL_CHATTYINFORMATION, ss);
        std::stringstream().swap(ss);
        if (scenesInvolved.empty())
        {
            ss << " scenes involved: none found (=either no scenes in repository or no overlap at all)";
        }
        else
        {
            ss << " scenes involved: ";
            bool isFirst = true;
            for (const auto it : scenesInvolved)
            {
                if (!isFirst)
                {
                    ss << ", ";
                }

                ss << it;
                isFirst = false;
            }
        }

        GetSite()->Log(LOGLEVEL_CHATTYINFORMATION, ss);
    }


    if (scenesInvolved.size() <= 1)
    {
        // we only have to deal with a single scene (or: the document does not include a scene-dimension at all), in this
        //  case we do not have group by scene and save some cycles
        auto sbSetsortedByZoom = this->GetSubSetFilteredBySceneSortedByZoom(roi, planeCoordinate, scenesInvolved, options.sortByM);
        this->Paint(bmDest, roi, sbSetsortedByZoom, zoom, options/*.useVisibilityCheckOptimization*/);
    }
    else
    {
        const auto sbSetSortedByZoomPerScene = this->GetSubSetSortedByZoomPerScene(scenesInvolved, roi, planeCoordinate, options.sortByM);
        for (const auto& it : sbSetSortedByZoomPerScene)
        {
            this->Paint(bmDest, roi, get<1>(it), zoom, options/*.useVisibilityCheckOptimization*/);
        }
    }
}

void CSingleChannelScalingTileAccessor::Paint(libCZI::IBitmapData* bmDest, const libCZI::IntRect& roi, const SubSetSortedByZoom& sbSetSortedByZoom, float zoom, const libCZI::ISingleChannelScalingTileAccessor::Options& options)
{
    const int idxOf1stSubBlockOfZoomGreater = this->GetIdxOf1stSubBlockWithZoomGreater(sbSetSortedByZoom.subBlocks, sbSetSortedByZoom.sortedByZoom, zoom);
    if (idxOf1stSubBlockOfZoomGreater < 0)
    {
        // this means that we would need to overzoom (i.e. the requested zoom is less than the lowest level we find in the subblock-repository)
        // TODO: this requires special consideration, for the time being -> bail out
        // ...we end up here e. g. when lowest level does not cover all the range, so - this is not
        //    something where we want to throw an excpetion
        //throw LibCZIAccessorException("Overzoom not supported", LibCZIAccessorException::ErrorType::Unspecified);
        return;
    }

    // start_iterator points into the "sortedByZoom" vector, which contains indices into the "subBlocks" vector
    std::vector<int>::const_iterator start_iterator = sbSetSortedByZoom.sortedByZoom.cbegin();
    std::advance(start_iterator, idxOf1stSubBlockOfZoomGreater);

    // find the end_iterator - this is the first element in the sortedByZoom-vector which has a zoom-level that is about twice that of the first element
    const float startZoom = sbSetSortedByZoom.subBlocks.at(*start_iterator).GetZoom();
    auto end_iterator = start_iterator + 1;
    for (; end_iterator != sbSetSortedByZoom.sortedByZoom.cend(); ++end_iterator)
    {
        const SbInfo& sbInfo = sbSetSortedByZoom.subBlocks.at(*end_iterator);
        // as an interim solution (in fact... this seems to be a rather good solution...), stop when we arrive at subblocks with a zoom-level about twice that what we started with
        if (sbInfo.GetZoom() >= startZoom * 1.9f)
        {
            break;
        }
    }

    if (!options.useVisibilityCheckOptimization)
    {
        for (auto it = start_iterator; it != end_iterator; ++it)
        {
            const SbInfo& sbInfo = sbSetSortedByZoom.subBlocks.at(*it);

            if (GetSite()->IsEnabled(LOGLEVEL_CHATTYINFORMATION))
            {
                stringstream ss;
                ss << " Drawing subblock: idx=" << sbInfo.index << " Log.: " << sbInfo.logicalRect << " Phys.Size: " << sbInfo.physicalSize;
                GetSite()->Log(LOGLEVEL_CHATTYINFORMATION, ss);
            }

            this->ScaleBlt(bmDest, zoom, roi, sbInfo, options);
        }
    }
    else
    {
        const auto indices_of_visible_tiles = this->CheckForVisibility(
            roi,
            static_cast<int>(distance(start_iterator, end_iterator)),           // how many subblocks we have in the range [start_iterator, end_iterator)
            [&](int index)->int
            {
                // dereference the iterator (advanced by the index we get), this gives us an index into the 
                // subBlocks-vector, which we then use to get the subblock-index of the subblock
                return sbSetSortedByZoom.subBlocks[*(start_iterator + index)].index;
            });

        // Now, draw only the subblocks which are visible - the vector "indices_of_visible_tiles" contains the indices "as they were passed to the lambda".
        for (const auto i : indices_of_visible_tiles)
        {
            // dereference the iterator (advanced by the index from out loop variable), this gives us an index into the
            // subBlocks-vector
            const SbInfo& sbInfo = sbSetSortedByZoom.subBlocks.at(*(start_iterator + i));
            if (GetSite()->IsEnabled(LOGLEVEL_CHATTYINFORMATION))
            {
                stringstream ss;
                ss << " Drawing subblock: idx=" << sbInfo.index << " Log.: " << sbInfo.logicalRect << " Phys.Size: " << sbInfo.physicalSize;
                GetSite()->Log(LOGLEVEL_CHATTYINFORMATION, ss);
            }

            this->ScaleBlt(bmDest, zoom, roi, sbInfo, options);
        }
    }
}

/// <summary>	Using the specified ROI, determine the scenes it intersects with. If the
/// 			subblock-repository does not contain an "S-dimension" we return an empty result.</summary>
/// <param name="roi">	The ROI. </param>
/// <param name="pSceneIndexSet"> Set of the scenes which are "allowed". May be null, in which case all scenes are considered "allowed". </param>
/// <returns>	A vector with the scene indices that the specified ROI intersects with. </returns>
std::vector<int> CSingleChannelScalingTileAccessor::DetermineInvolvedScenes(const libCZI::IntRect& roi, const libCZI::IIndexSet* pSceneIndexSet)
{
    SubBlockStatistics statistics = this->sbBlkRepository->GetStatistics();
    if (statistics.sceneBoundingBoxes.empty())
    {
        return std::vector<int>();
    }

    std::vector<int> result;
    for (auto it = statistics.sceneBoundingBoxes.cbegin(); it != statistics.sceneBoundingBoxes.cend(); ++it)
    {
        // check if the scene is part of the "scene index set" (if this is specified)
        if (pSceneIndexSet == nullptr || pSceneIndexSet->IsContained(it->first))
        {
            if (it->second.boundingBox.IntersectsWith(roi))
            {
                result.push_back(it->first);
            }
        }
    }

    return result;
}

/// <summary>   Gets the subset of subblocks intersecting with the ROI, having the specified plane-coordinate
///             and where their scene-index is among the ones given. If the subblock has no scene-index, the 
///             filtering by-scene-index is not applied. </summary>
/// <param name="roi">              The region-of-interest rectangle. </param>
/// <param name="planeCoordinate">  The plane coordinate. </param>
/// <param name="allowedScenes">    The list of allowed scenes. </param>
/// <param name="sortByM">          Whether to sort the subblocks by their zoom level AND the M-Index or only by zoom level. </param>
/// <returns>   The subset of subblocks filtered by the specified conditions, sorted by their zoom. </returns>
CSingleChannelScalingTileAccessor::SubSetSortedByZoom CSingleChannelScalingTileAccessor::GetSubSetFilteredBySceneSortedByZoom(const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, const std::vector<int>& allowedScenes, bool sortByM)
{
    SubSetSortedByZoom result;
    result.subBlocks = this->GetSubSet(roi, planeCoordinate, &allowedScenes);
    result.sortedByZoom = this->CreateSortByZoom(result.subBlocks, sortByM);
    return result;
}

std::vector<std::tuple<int, CSingleChannelScalingTileAccessor::SubSetSortedByZoom>> CSingleChannelScalingTileAccessor::GetSubSetSortedByZoomPerScene(const vector<int>& scenes, const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, bool sortByM)
{
    std::vector<std::tuple<int, CSingleChannelScalingTileAccessor::SubSetSortedByZoom>> result;
    CDimCoordinate coord(planeCoordinate);
    for (const auto& sceneIdx : scenes)
    {
        SubSetSortedByZoom sbset;

        // we explicitly set the S-coordinate here so that we only get subblocks of this scene
        // TODO: we need to look into what is supposed to happen if the user passed in a scene-index
        //       I guess the natural thing would be to consider only the specified scene
        coord.Set(DimensionIndex::S, sceneIdx);
        sbset.subBlocks = this->GetSubSet(roi, &coord, nullptr);
        sbset.sortedByZoom = this->CreateSortByZoom(sbset.subBlocks, sortByM);
        result.emplace_back(sceneIdx, sbset);
    }

    return result;
}
