// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <tuple>
#include <vector>
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
	explicit CSingleChannelScalingTileAccessor(std::shared_ptr<libCZI::ISubBlockRepository> sbBlkRepository);

public:	// interface ISingleChannelScalingTileAccessor
	libCZI::IntSize CalcSize(const libCZI::IntRect& roi, float zoom) const override;
	std::shared_ptr<libCZI::IBitmapData> Get(const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, float zoom, const libCZI::ISingleChannelScalingTileAccessor::Options* pOptions) override;
	std::shared_ptr<libCZI::IBitmapData> Get(libCZI::PixelType pixeltype, const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, float zoom, const libCZI::ISingleChannelScalingTileAccessor::Options* pOptions) override;
	void Get(libCZI::IBitmapData* pDest, const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, float zoom, const libCZI::ISingleChannelScalingTileAccessor::Options* pOptions) override;
private:
	static libCZI::IntSize InternalCalcSize(const libCZI::IntRect& roi, float zoom);

	std::vector<int> CreateSortByZoom(const std::vector<SbInfo>& sbBlks);
	std::vector<SbInfo> GetSubSet(const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, const std::vector<int>* allowedScenes);
	int GetIdxOf1stSubBlockWithZoomGreater(const std::vector<SbInfo>& sbBlks, const std::vector<int>& byZoom, float zoom);
	void ScaleBlt(libCZI::IBitmapData* bmDest, float zoom, const libCZI::IntRect&  roi, const SbInfo& sbInfo);

	void InternalGet(libCZI::IBitmapData* bmDest, const libCZI::IntRect&  roi, const libCZI::IDimCoordinate* planeCoordinate, float zoom, const libCZI::ISingleChannelScalingTileAccessor::Options& options);

	std::vector<int> DetermineInvolvedScenes(const libCZI::IntRect&  roi, const libCZI::IIndexSet* pSceneIndexSet);
	
	struct SubSetSortedByZoom
	{
		std::vector<SbInfo> subBlocks;
		std::vector<int>	sortedByZoom;
	};

	SubSetSortedByZoom GetSubSetFilteredBySceneSortedByZoom(const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate,const std::vector<int>& allowedScenes);

	std::vector<std::tuple<int, SubSetSortedByZoom>> GetSubSetSortedByZoomPerScene(const std::vector<int>& scenes, const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate);
	void Paint(libCZI::IBitmapData* bmDest, const libCZI::IntRect&  roi,const SubSetSortedByZoom& sbSetSortedByZoom, float zoom);
};
