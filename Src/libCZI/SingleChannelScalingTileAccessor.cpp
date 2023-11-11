// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SingleChannelScalingTileAccessor.h"

#include <chrono>

#include "utilities.h"
#include "BitmapOperations.h"
#include "Site.h"

using namespace libCZI;
using namespace std;

CSingleChannelScalingTileAccessor::CSingleChannelScalingTileAccessor(std::shared_ptr<ISubBlockRepository> sbBlkRepository)
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

void CSingleChannelScalingTileAccessor::ScaleBlt(libCZI::IBitmapData* bmDest, float zoom, const libCZI::IntRect& roi, const SbInfo& sbInfo)
{
    auto start = std::chrono::high_resolution_clock::now();
    const auto sb = this->sbBlkRepository->ReadSubBlock(sbInfo.index);
    if (GetSite()->IsEnabled(LOGLEVEL_CHATTYINFORMATION))
    {
        stringstream ss;
        ss << "   bounds: " << Utils::DimCoordinateToString(&sb->GetSubBlockInfo().coordinate) << " M=" << (Utils::IsValidMindex(sbInfo.mIndex) ? to_string(sbInfo.mIndex) : "invalid");
        GetSite()->Log(LOGLEVEL_CHATTYINFORMATION, ss);
    }

    const auto source = sb->CreateBitmap();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    if (GetSite()->IsEnabled(LOGLEVEL_CHATTYINFORMATION))
    {
        IntSize extent = source->GetSize();
        int64_t data_size = static_cast<int64_t>(extent.w) * extent.h * CziUtils::GetBytesPerPel(source->GetPixelType());
        std::stringstream ss;
        double elapsed_time = elapsed_seconds.count();
        ss << "ReadSubblock&CreateBitmap: " << elapsed_time * 1000.0 << "ms, size: " << data_size / 1e6 << "MB -> " << (data_size / elapsed_time) / 1e6 << "MB/s";
        GetSite()->Log(LOGLEVEL_CHATTYINFORMATION, ss);
    }

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

        const double roiSrcTopLeftX = double(intersect.x - sbInfo.logicalRect.x) / sbInfo.logicalRect.w;
        const double roiSrcTopLeftY = double(intersect.y - sbInfo.logicalRect.y) / sbInfo.logicalRect.h;
        const double roiSrcBttmRightX = double(intersect.x + intersect.w - sbInfo.logicalRect.x) / sbInfo.logicalRect.w;
        const double roiSrcBttmRightY = double(intersect.y + intersect.h - sbInfo.logicalRect.y) / sbInfo.logicalRect.h;

        const double destTopLeftX = double(intersect.x - roi.x) / roi.w;
        const double destTopLeftY = double(intersect.y - roi.y) / roi.h;
        const double destBttmRightX = double(intersect.x + intersect.w - roi.x) / roi.w;
        const double destBttmRightY = double(intersect.y + intersect.h - roi.y) / roi.h;

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
        std::sort(byZoom.begin(), byZoom.end(), [&](const int i1, const int i2)->bool {return sbBlks.at(i1).GetZoom() < sbBlks.at(i2).GetZoom(); });
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
        this->Paint(bmDest, roi, sbSetsortedByZoom, zoom, options.useVisibilityCheckOptimization);
    }
    else
    {
        const auto sbSetSortedByZoomPerScene = this->GetSubSetSortedByZoomPerScene(scenesInvolved, roi, planeCoordinate, options.sortByM);
        for (const auto& it : sbSetSortedByZoomPerScene)
        {
            this->Paint(bmDest, roi, get<1>(it), zoom, options.useVisibilityCheckOptimization);
        }
    }
}

void CSingleChannelScalingTileAccessor::Paint(libCZI::IBitmapData* bmDest, const libCZI::IntRect& roi, const SubSetSortedByZoom& sbSetSortedByZoom, float zoom, bool useCoverageOptimization)
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

    std::vector<int>::const_iterator start_iterator = sbSetSortedByZoom.sortedByZoom.cbegin();
    std::advance(start_iterator, idxOf1stSubBlockOfZoomGreater);

    const float startZoom = sbSetSortedByZoom.subBlocks.at(*start_iterator).GetZoom();
    auto end_iterator = start_iterator;
    for (; end_iterator != sbSetSortedByZoom.sortedByZoom.cend(); ++end_iterator)
    {
        const SbInfo& sbInfo = sbSetSortedByZoom.subBlocks.at(*end_iterator);
        // as an interim solution (in fact... this seems to be a rather good solution...), stop when we arrive at subblocks with a zoom-level about twice that what we started with
        if (sbInfo.GetZoom() >= startZoom * 1.9f)
        {
            break;
        }
    }

    if (!useCoverageOptimization)
    {
        for (auto it = start_iterator; it != end_iterator; ++it)
        {
            const SbInfo& sbInfo = sbSetSortedByZoom.subBlocks.at(*it);
            /*
            // as an interim solution (in fact... this seems to be a rather good solution...), stop when we arrive at subblocks with a zoom-level about twice that what we started with
            if (sbInfo.GetZoom() >= startZoom * 1.9f)
            {
                break;
            }
            */
            if (GetSite()->IsEnabled(LOGLEVEL_CHATTYINFORMATION))
            {
                stringstream ss;
                ss << " Drawing subblock: idx=" << sbInfo.index << " Log.: " << sbInfo.logicalRect << " Phys.Size: " << sbInfo.physicalSize;
                GetSite()->Log(LOGLEVEL_CHATTYINFORMATION, ss);
            }

            this->ScaleBlt(bmDest, zoom, roi, sbInfo);
        }
    }
    else
    {
        const auto indices_of_visible_tiles = this->CheckForVisibility(
           roi,
           static_cast<int>(distance(start_iterator, end_iterator)),
           [&](int index)->int
           {
               const auto element = end_iterator - 1 - index;
               return sbSetSortedByZoom.subBlocks.at(*element).index;
           });

        for (const auto it : indices_of_visible_tiles)
        {
            const SbInfo& sbInfo = sbSetSortedByZoom.subBlocks.at(*(start_iterator + it));
            if (GetSite()->IsEnabled(LOGLEVEL_CHATTYINFORMATION))
            {
                stringstream ss;
                ss << " Drawing subblock: idx=" << sbInfo.index << " Log.: " << sbInfo.logicalRect << " Phys.Size: " << sbInfo.physicalSize;
                GetSite()->Log(LOGLEVEL_CHATTYINFORMATION, ss);
            }

            this->ScaleBlt(bmDest, zoom, roi, sbInfo);
        }
    }

#if false
    const int idxOf1stSubBlockOfZoomGreater = this->GetIdxOf1stSubBlockWithZoomGreater(sbSetSortedByZoom.subBlocks, sbSetSortedByZoom.sortedByZoom, zoom);
    if (idxOf1stSubBlockOfZoomGreater < 0)
    {
        // this means that we would need to overzoom (i.e. the requested zoom is less than the lowest level we find in the subblock-repository)
        // TODO: this requires special consideration, for the time being -> bail out
        // ...we end up here e. g. when lowest level does not cover all the range, so - this is not
        //    something where we want to throw an exception
        //throw LibCZIAccessorException("Overzoom not supported", LibCZIAccessorException::ErrorType::Unspecified);
        return;
    }

    std::vector<int>::const_iterator it = sbSetSortedByZoom.sortedByZoom.cbegin();
    std::advance(it, idxOf1stSubBlockOfZoomGreater);

    const float startZoom = sbSetSortedByZoom.subBlocks.at(*it).GetZoom();

    // determine the end
    std::vector<int>::const_iterator itEnd = sbSetSortedByZoom.sortedByZoom.cend();
    if (useVisibilityCheckOptimization)
    {
        RectangleCoverageCalculator coverageCalculator;
        for (auto it2 = it; it2 != sbSetSortedByZoom.sortedByZoom.cend(); ++it2)
        {
            const SbInfo& sbInfo = sbSetSortedByZoom.subBlocks.at(*it2);
            coverageCalculator.AddRectangle(sbInfo.logicalRect);
            if (coverageCalculator.CalcAreaOfIntersectionWithRectangle(roi) == roi.w * roi.h)
            {
                itEnd = ++it2;
                break;
            }
        }
    }

    for (; it != itEnd; ++it)
    {
        const SbInfo& sbInfo = sbSetSortedByZoom.subBlocks.at(*it);

        // as an interim solution (in fact... this seems to be a rather good solution...), stop when we arrive at subblocks with a zoom-level about twice that what we started with
        if (sbInfo.GetZoom() >= startZoom * 1.9f)
        {
            break;
        }

        if (GetSite()->IsEnabled(LOGLEVEL_CHATTYINFORMATION))
        {
            stringstream ss;
            ss << " Drawing subblock: idx=" << sbInfo.index << " Log.: " << sbInfo.logicalRect << " Phys.Size: " << sbInfo.physicalSize;
            GetSite()->Log(LOGLEVEL_CHATTYINFORMATION, ss);
        }

        this->ScaleBlt(bmDest, zoom, roi, sbInfo);
    }
    /*
    for (; it != sbSetSortedByZoom.sortedByZoom.cend(); ++it)
    {
        const SbInfo& sbInfo = sbSetSortedByZoom.subBlocks.at(*it);

        // as an interim solution (in fact... this seems to be a rather good solution...), stop when we arrive at subblocks with a zoom-level about twice that what we started with
        if (sbInfo.GetZoom() >= startZoom * 1.9f)
        {
            break;
        }

        if (GetSite()->IsEnabled(LOGLEVEL_CHATTYINFORMATION))
        {
            stringstream ss;
            ss << " Drawing subblock: idx=" << sbInfo.index << " Log.: " << sbInfo.logicalRect << " Phys.Size: " << sbInfo.physicalSize;
            GetSite()->Log(LOGLEVEL_CHATTYINFORMATION, ss);
        }

        this->ScaleBlt(bmDest, zoom, roi, sbInfo);
    }
    */
    /*
    RectangleCoverageCalculator coverageCalculator;
    for (; it != sbSetSortedByZoom.sortedByZoom.cend(); ++it)
    {
        const SbInfo& sbInfo = sbSetSortedByZoom.subBlocks.at(*it);

        coverageCalculator.AddRectangle(sbInfo.logicalRect);

        // as an interim solution (in fact... this seems to be a rather good solution...), stop when we arrive at subblocks with a zoom-level about twice that what we started with
        if (sbInfo.GetZoom() >= startZoom * 1.9f)
        {
            break;
        }

        if (GetSite()->IsEnabled(LOGLEVEL_CHATTYINFORMATION))
        {
            stringstream ss;
            ss << " Drawing subblock: idx=" << sbInfo.index << " Log.: " << sbInfo.logicalRect << " Phys.Size: " << sbInfo.physicalSize;
            GetSite()->Log(LOGLEVEL_CHATTYINFORMATION, ss);
        }

        this->ScaleBlt(bmDest, zoom, roi, sbInfo);

        if (coverageCalculator.CalcAreaOfIntersectionWithRectangle(roi) == roi.w*roi.h)
        {
            break;
        }
    }
    */
#endif
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
        result.emplace_back(make_tuple(sceneIdx, sbset));
    }

    return result;
}
