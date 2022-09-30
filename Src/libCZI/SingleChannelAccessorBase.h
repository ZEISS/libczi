// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "libCZI.h"

class CSingleChannelAccessorBase
{
protected:
	std::shared_ptr<libCZI::ISubBlockRepository> sbBlkRepository;

	explicit CSingleChannelAccessorBase(std::shared_ptr<libCZI::ISubBlockRepository> sbBlkRepository)
		: sbBlkRepository(sbBlkRepository)
	{}

	bool TryGetPixelType(const libCZI::IDimCoordinate* planeCoordinate, libCZI::PixelType& pixeltype);

	static void Clear(libCZI::IBitmapData* bm, const libCZI::RgbFloatColor& floatColor);

	void CheckPlaneCoordinates(const libCZI::IDimCoordinate* planeCoordinate) const;
};
