// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "CZIReader.h"
#include "libCZI.h"
#include "SingleChannelAccessorBase.h"

class CSingleChannelTileAccessor : public CSingleChannelAccessorBase, public libCZI::ISingleChannelTileAccessor
{
public:
	explicit CSingleChannelTileAccessor(std::shared_ptr<libCZI::ISubBlockRepository> sbBlkRepository);

public:	// interface ISingleChannelTileAccessor
	std::shared_ptr<libCZI::IBitmapData> Get(const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, const libCZI::ISingleChannelTileAccessor::Options* pOptions) override;
	std::shared_ptr<libCZI::IBitmapData> Get(libCZI::PixelType pixeltype, const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, const Options* pOptions) override;
	void Get(libCZI::IBitmapData* pDest, int xPos, int yPos, const libCZI::IDimCoordinate* planeCoordinate, const Options* pOptions) override;
private:
	void InternalGet(int xPos, int yPos, libCZI::IBitmapData* pBm, const libCZI::IDimCoordinate* planeCoordinate, const libCZI::ISingleChannelTileAccessor::Options* pOptions);
	//std::shared_ptr<libCZI::IBitmapData> InternalGet(const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, const ISingleChannelTileAccessor::Options* pOptions);
	void GetAllSubBlocks(const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, std::function<void(int index, int mIndex)> appender/*, libCZI::PixelType* pPixelTypeOfFirstFoundSubBlock*/);

	struct IndexAndM
	{
		int index;
		int mIndex;
	};

	std::vector<CSingleChannelTileAccessor::IndexAndM> GetSubBlocksSubset(const libCZI::IntRect& roi, const libCZI::IDimCoordinate* planeCoordinate, bool sortByM/*, libCZI::PixelType* pPixelTypeOfFirstFoundSubBlock = nullptr*/);
	void ComposeTiles(libCZI::IBitmapData* pBm, int xPos, int yPos, const std::vector<IndexAndM>& subBlocksSet, const libCZI::ISingleChannelTileAccessor::Options& options);
};
