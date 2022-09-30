// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "libCZI_Pixels.h"
#include "libCZI_DimCoordinate.h"
#include <functional>
#include <stdint.h>

enum class CompareResult
{
	Equal,
	NotEqual,
	Ambigious
};

class CziUtils
{
public:
	static libCZI::PixelType PixelTypeFromInt(int i);
	static int IntFromPixelType(libCZI::PixelType);
	static libCZI::CompressionMode CompressionModeFromInt(int i);
	static std::int32_t CompressionModeToInt(libCZI::CompressionMode m);
	static std::uint8_t GetBytesPerPel(libCZI::PixelType pixelType);
	static bool CompareCoordinate(const libCZI::IDimCoordinate* coord1, const libCZI::IDimCoordinate* coord2);
	static void EnumAllCoordinateDimensions(std::function<bool(libCZI::DimensionIndex)> func);

	static double CalculateMinificationFactor(int logicalSizeWidth, int logicalSizeHeight, int physicalSizeWidth, int physicalSizeHeight);

	static bool IsPixelTypeEndianessAgnostic(libCZI::PixelType);

	template <libCZI::PixelType tPixelType>
	static constexpr std::uint8_t BytesPerPel();// { /*throw std::logic_error("invalid pixeltype");*/return 0; }
};

template <>	constexpr std::uint8_t CziUtils::BytesPerPel<libCZI::PixelType::Gray8>() { return 1; }
template <>	constexpr std::uint8_t CziUtils::BytesPerPel<libCZI::PixelType::Gray16>() { return 2; }
template <>	constexpr std::uint8_t CziUtils::BytesPerPel<libCZI::PixelType::Bgr24>() { return 3; }
template <>	constexpr std::uint8_t CziUtils::BytesPerPel<libCZI::PixelType::Bgr48>() { return 6; }
template <>	constexpr std::uint8_t CziUtils::BytesPerPel<libCZI::PixelType::Gray32Float>() { return 4; }
template <>	constexpr std::uint8_t CziUtils::BytesPerPel<libCZI::PixelType::Gray64Float>() { return 8; }
template <>	constexpr std::uint8_t CziUtils::BytesPerPel<libCZI::PixelType::Bgra32>() { return 4; }
template <>	constexpr std::uint8_t CziUtils::BytesPerPel<libCZI::PixelType::Gray32>() { return 4; }
template <>	constexpr std::uint8_t CziUtils::BytesPerPel<libCZI::PixelType::Bgr96Float>() { return 3 * 4; }
template <>	constexpr std::uint8_t CziUtils::BytesPerPel<libCZI::PixelType::Gray64ComplexFloat>() { return 2 * 8; }
template <>	constexpr std::uint8_t CziUtils::BytesPerPel<libCZI::PixelType::Bgr192ComplexFloat>() { return 48; }
