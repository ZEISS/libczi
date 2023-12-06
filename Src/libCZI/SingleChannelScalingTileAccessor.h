// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <tuple>
#include <vector>
#include <memory>
#include "CZIReader.h"
#include "libCZI.h"
#include "SingleChannelAccessorBase.h"

class CSingleChannelScalingTileAccessor : public CSingleChannelAccessorBase, public libCZI::ISingleChannelScalingTileAccessor
{
private:
    struct SbInfo
    {
        libCZI::IntRect			logicalRect;
        libCZI::IntSize			physicalSize;
        int						mIndex;
        int						index;

        float	GetZoom() const { return libCZI::Utils::CalcZoom(this->logicalRect, this->physicalSize); }
    };

public:
    explicit CSingleChannelScalingTileAccessor(const std::shared_ptr<libCZI::ISubBlockRepository>& sbBlkRepository);

public:	// interface ISingleChannelScalingTileAccessor
    libCZI::IntSize CalcSize(const libCZI::IntRect& roi, float zoom) const override;
    std::shared_ptr<libCZI::IBitmapData> Get(const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, float zoom, const libCZI::ISingleChannelScalingTileAccessor::Options* pOptions) override;
    std::shared_ptr<libCZI::IBitmapData> Get(libCZI::PixelType pixeltype, const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, float zoom, const libCZI::ISingleChannelScalingTileAccessor::Options* pOptions) override;
    void Get(libCZI::IBitmapData* pDest, const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, float zoom, const libCZI::ISingleChannelScalingTileAccessor::Options* pOptions) override;
private:
    static libCZI::IntSize InternalCalcSize(const libCZI::IntRect& roi, float zoom);

    std::vector<int> CreateSortByZoom(const std::vector<SbInfo>& sbBlks, bool sortByM);
    std::vector<SbInfo> GetSubSet(const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, const std::vector<int>* allowedScenes);
    int GetIdxOf1stSubBlockWithZoomGreater(const std::vector<SbInfo>& sbBlks, const std::vector<int>& byZoom, float zoom);
    void ScaleBlt(libCZI::IBitmapData* bmDest, float zoom, const libCZI::IntRect& roi, const SbInfo& sbInfo, const libCZI::ISingleChannelScalingTileAccessor::Options& options);

    void InternalGet(libCZI::IBitmapData* bmDest, const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, float zoom, const libCZI::ISingleChannelScalingTileAccessor::Options& options);

    std::vector<int> DetermineInvolvedScenes(const libCZI::IntRect& roi, const libCZI::IIndexSet* pSceneIndexSet);

    /// This struct contains a vector of subblocks, and a vector of indices into this vector which gives an ordering
    /// by zoom of the subblocks.
    struct SubSetSortedByZoom
    {
        std::vector<SbInfo> subBlocks;      ///< The vector containing the subblocks (which are in no particular order).
        std::vector<int>    sortedByZoom;   ///< Vector with indices (into the vector 'subBlocks') which gives the ordering by zoom.
    };

    SubSetSortedByZoom GetSubSetFilteredBySceneSortedByZoom(const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, const std::vector<int>& allowedScenes, bool sortByM);

    std::vector<std::tuple<int, SubSetSortedByZoom>> GetSubSetSortedByZoomPerScene(const std::vector<int>& scenes, const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, bool sortByM);
    void Paint(libCZI::IBitmapData* bmDest, const libCZI::IntRect& roi, const SubSetSortedByZoom& sbSetSortedByZoom, float zoom, const libCZI::ISingleChannelScalingTileAccessor::Options& options);
};
