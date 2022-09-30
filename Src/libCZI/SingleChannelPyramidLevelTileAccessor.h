// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "CZIReader.h"
#include "libCZI.h"
#include "SingleChannelAccessorBase.h"

class CSingleChannelPyramidLevelTileAccessor : public CSingleChannelAccessorBase, public libCZI::ISingleChannelPyramidLayerTileAccessor
{
private:
	struct SbInfo
	{
		libCZI::IntRect			logicalRect;
		libCZI::IntSize			physicalSize;
		int						mIndex;
		int						index;
	};

	struct SbByLayer
	{
		std::vector<int> indices;
	};

public:
	explicit CSingleChannelPyramidLevelTileAccessor(std::shared_ptr<libCZI::ISubBlockRepository> sbBlkRepository);

public:	// interface ISingleChannelTileAccessor
	std::shared_ptr<libCZI::IBitmapData> Get(const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, const PyramidLayerInfo& pyramidInfo, const Options* pOptions) override;

	std::shared_ptr<libCZI::IBitmapData> Get(libCZI::PixelType pixeltype, const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, const PyramidLayerInfo& pyramidInfo, const libCZI::ISingleChannelPyramidLayerTileAccessor::Options* pOptions) override;

	void Get(libCZI::IBitmapData* pDest, int xPos, int yPos, const libCZI::IDimCoordinate* planeCoordinate, const PyramidLayerInfo& pyramidInfo, const Options* pOptions) override;
private:
	libCZI::IntRect CalcDestinationRectFromSourceRect(const libCZI::IntRect& roi, const PyramidLayerInfo& pyramidInfo);

	libCZI::IntRect NormalizePyramidRect(int x, int y, int w, int h, const PyramidLayerInfo& pyramidInfo);

	static int CalcSizeOfPixelOnLayer0(const PyramidLayerInfo& pyramidInfo);

	std::map<int,SbByLayer> CalcByLayer(const std::vector<SbInfo>& sbinfo, int minificationFactor);

	std::vector<SbInfo> GetSubBlocksSubset(const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, const PyramidLayerInfo& pyramidInfo,const libCZI::IIndexSet* sceneFilter);

	void GetAllSubBlocks(const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, const libCZI::IIndexSet* sceneFilter, const std::function<void(const SbInfo& info)>& appender) const;

	int CalcPyramidLayerNo(const libCZI::IntRect& logicalRect, const libCZI::IntSize& physicalSize, int minificationFactor);

	void ComposeTiles(libCZI::IBitmapData* bm, int xPos, int yPos, int sizeOfPixel, int bitmapCnt, const Options& options, std::function<SbInfo(int)> getSbInfo);

	void InternalGet(libCZI::IBitmapData* pDest, int xPos, int yPos, int sizeOfPixelOnLayer0, const libCZI::IDimCoordinate* planeCoordinate, const PyramidLayerInfo& pyramidInfo, const Options& options);
};
