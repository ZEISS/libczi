// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "stdafx.h"

#include "libCZI.h"
#include "Site.h"
#include "BitmapOperations.h"
#include "splines.h"
#include "IndexSet.h"
#include "MD5Sum.h"
#include "CziMetadataBuilder.h"
#include "utilities.h"

#include <cmath>
#include <string>
#include <regex>

using namespace libCZI;
using namespace std;

namespace
{
	bool _tryParseCompressionMode(const std::string& s, libCZI::CompressionMode* m)
	{
		static constexpr libCZI::CompressionMode AvailableCompressionModes[] = 
		{ 
			libCZI::CompressionMode::UnCompressed,
			libCZI::CompressionMode::Jpg,
			libCZI::CompressionMode::JpgXr,
			libCZI::CompressionMode::Zstd0,
			libCZI::CompressionMode::Zstd1
		};

		for (const auto& compressionMode : AvailableCompressionModes)
		{
			if (Utilities::icasecmp(Utils::CompressionModeToInformalString(compressionMode), s) == true)
			{
				if (m != nullptr)
				{
					*m = compressionMode;
				}

				return true;
			}
		}
		
		return false;
	}

	bool _tryParseCompressionOptions(const std::string& s, std::map<int, libCZI::CompressParameter>* map)
	{
		const std::regex compressionOptionsRegex(R"(^\s*([a-zA-Z0-9]*)\s*=\s*([a-zA-Z0-9.+-]*)\s*$)");
		istringstream stringStream(s);
		string part;
		while (getline(stringStream, part, ';'))
		{
			std::smatch pieces_match;
			if (std::regex_match(part, pieces_match, compressionOptionsRegex))
			{
				const string& key = pieces_match[1].str();
				const string& value = pieces_match[2].str();

				// strategy is (currently): anything we do not understand we ignore
				if (Utilities::icasecmp(key, Utils::KEY_COMPRESS_EXPLICIT_LEVEL))
				{
					size_t indexParsingStopped;
					try
					{
						int i = stoi(value, &indexParsingStopped);
						if (value[indexParsingStopped] != '\0')
						{
							// this means that parsing stopped before we reached the end of the string
							return false;
						}

						if (map != nullptr)
						{
							(*map)[static_cast<int>(libCZI::CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL)] = libCZI::CompressParameter(i);
						}
					}
					catch (invalid_argument&)
					{
						return false;
					}
					catch (out_of_range&)
					{
						return false;
					}
				}
				else if (Utilities::icasecmp(key, Utils::KEY_COMPRESS_PRE_PROCESS))
				{
					if (Utilities::icasecmp(value, Utils::VALUE_COMPRESS_HILO_BYTE_UNPACK))
					{
						if (map != nullptr)
						{
							(*map)[static_cast<int>(libCZI::CompressionParameterKey::ZSTD_PREPROCESS_DOLOHIBYTEPACKING)] = libCZI::CompressParameter(true);
						}
					}
				}
			}
		}

		return true;
	}
} // namespace

const char* const Utils::KEY_COMPRESS_EXPLICIT_LEVEL		= "ExplicitLevel";
const char* const Utils::KEY_COMPRESS_PRE_PROCESS			= "PreProcess";
const char* const Utils::VALUE_COMPRESS_HILO_BYTE_UNPACK	= "HiLoByteUnpack";

/*static*/char Utils::DimensionToChar(libCZI::DimensionIndex dim)
{
		switch (dim)
		{
		case	DimensionIndex::Z:return 'Z';
		case 	DimensionIndex::C:return 'C';
		case 	DimensionIndex::T:return 'T';
		case 	DimensionIndex::R:return 'R';
		case 	DimensionIndex::S:return 'S';
		case 	DimensionIndex::I:return 'I';
		case 	DimensionIndex::H:return 'H';
		case 	DimensionIndex::V:return 'V';
		case 	DimensionIndex::B:return 'B';
		default:return '?';
		}
}

/*static*/libCZI::DimensionIndex Utils::CharToDimension(char c)
{
		switch (c)
		{
		case 'z':case'Z':
				return	DimensionIndex::Z;
		case 'c':case'C':
				return DimensionIndex::C;
		case 't':case'T':
				return 	DimensionIndex::T;
		case 'r':case'R':
				return 	DimensionIndex::R;
		case 's':case'S':
				return 	DimensionIndex::S;
		case 'i':case'I':
				return 	DimensionIndex::I;
		case 'h':case'H':
				return 	DimensionIndex::H;
		case 'v':case'V':
				return 	DimensionIndex::V;
		case 'b':case'B':
				return 	DimensionIndex::B;
		default:
				return DimensionIndex::invalid;
		}
}

/*static*/int Utils::CalcMd5SumHash(libCZI::IBitmapData* bm, std::uint8_t* ptrHash, int hashSize)
{
		return CBitmapOperations::CalcMd5Sum(bm, ptrHash, hashSize);
}

/*static*/int Utils::CalcMd5SumHash(const void* ptrData, size_t sizeData, std::uint8_t* ptrHash, int hashSize)
{
		if (ptrHash == nullptr) { return 16; }
		if (hashSize < 16)
		{
				throw invalid_argument("argument 'hashsize' must be >= 16");
		}

		CMd5Sum md5sum;
		md5sum.update(ptrData, sizeData);
		md5sum.complete();
		md5sum.getHash((char*)ptrHash);
		return 16;
}

static double CalcSplineValue(double x, const std::vector<libCZI::IDisplaySettings::SplineData>& splineData)
{
		if (x < 0 || x>1)
		{
				throw invalid_argument("out of range");
		}

		size_t index = 0;
		for (size_t i = 0; i < splineData.size(); ++i)
		{
				if (x > splineData.at(i).xPos)
				{
						index = i;
				}
		}

		double xPosNormalized = x - splineData.at(index).xPos;

		return CSplines::CalculateSplineValue(xPosNormalized, splineData.at(index).coefficients);
}

/*static*/std::vector<std::uint8_t> Utils::Create8BitLookUpTableFromSplines(int tableElementCnt, float blackPoint, float whitePoint, const std::vector<libCZI::IDisplaySettings::SplineData>& splineData)
{
		std::vector<std::uint8_t> lut; lut.reserve(tableElementCnt);

		// TODO - look into rounding
		int blackVal = (int)(tableElementCnt * blackPoint);
		int whiteVal = (int)(tableElementCnt * whitePoint);

		for (int i = 0; i < blackVal; ++i)
		{
				lut.emplace_back(0);
		}

		for (int i = blackVal; i < whiteVal; ++i)
		{
				double x = (i - blackVal) / double(whiteVal - blackVal - 1);
				double s = CalcSplineValue(x, splineData);
				int is = (int)(s * 255);
				std::uint8_t value = (is < 0 ? 0 : is>255 ? 255 : (std::uint8_t)is);
				lut.emplace_back(value);
		}

		for (int i = whiteVal; i < tableElementCnt; ++i)
		{
				lut.emplace_back(255);
		}

		return lut;
}

/// <summary>
/// Gets the parameter for the toe slope adjustment function.
/// </summary>
/// <remarks>
/// The Toe Slope adjustment uses a slightly adjusted version of the gamma function to evaluate the display image.
/// The adjusted version has the advantage that its slope at the origin, i.e. for x = 0, doesn't equal infinity.
/// The formula for this looks like y = ((ax + 1)**G - 1) / ((a + 1)**G - 1), where the parameter "a" depends on the gamma value.
/// Additionally, we choose the slope of 1/(G**3) for x = 0. 
/// This yields the iteration formula that is used in the method:
/// a = ((a+1)**G - 1)/(G**4).
/// </remarks>
/// <param name="gamma">The gamma value.</param>
/// <returns>The parameter necessary for the toe slope adjustment function formula.</returns>
template <typename tFloat>
tFloat GetParameterForToeSlopeAdjustment(tFloat gamma)
{
		const double GammaTolerance = (tFloat)0.0001;
		if (abs(gamma - 0.5) < GammaTolerance)
		{
				return 224;
		}
		else if (abs(gamma - (tFloat)0.45) < GammaTolerance)
		{
				// Optimization for frequently used gamma value.
				return (tFloat)287.806332841221;
		}
		else
		{
				tFloat start = 224;
				tFloat result = start;

				tFloat gamma2 = gamma * gamma;
				tFloat factor = 1 / (gamma2 * gamma2);

				const tFloat ResultTolerance = (tFloat)0.000001;
				const int MaxIterationCount = 200;

				for (int i = 0; i < MaxIterationCount; i++)
				{
						start = result;
						result = factor * (pow(start + 1, gamma) - 1);

						if (abs(start - result) < ResultTolerance)
						{
								break;
						}
				}

				return result;
		}
}

template <typename tFloat>
std::vector<std::uint8_t> InternalCreate8BitLookUpTableFromGamma(int tableElementCnt, tFloat blackPoint, tFloat whitePoint, tFloat gamma)
{
		std::vector<std::uint8_t> lut; lut.reserve(tableElementCnt);

		int lowOut = (int)(blackPoint * tableElementCnt);
		int highOut = (int)(whitePoint * tableElementCnt);

		for (int i = 0; i < lowOut; ++i)
		{
				lut.emplace_back(0);
		}

		if (gamma < 1.0f)
		{
				// If gamma < 1, use toe slope to avoid slope of infinity for x = 0.
				// Toe slope adjustment formula: y = ((ax + 1)**G - 1) / ((a + 1)**G - 1)
				tFloat a = GetParameterForToeSlopeAdjustment(gamma);
				tFloat denumeratorToeSlope = (std::pow(a + 1, gamma) - 1);

				for (int i = lowOut; i < highOut; ++i)
				{
						tFloat x = (i - lowOut) / tFloat(highOut - lowOut - 1);

						tFloat val = 255 * (pow(a * x + 1, gamma) - 1) / denumeratorToeSlope;
						if (val > 255) { val = 255; }
						else if (val < 0) { val = 0; }

						std::uint8_t val8 = (std::uint8_t)val;
						lut.emplace_back(val8);
				}
		}
		else
		{
				for (int i = lowOut; i < highOut; ++i)
				{
						tFloat x = (i - lowOut) / tFloat(highOut - lowOut - 1);
						tFloat val = 255 * pow(x, gamma);
						if (val > 255) { val = 255; }
						else if (val < 0) { val = 0; }

						std::uint8_t val8 = (std::uint8_t)val;
						lut.emplace_back(val8);
				}
		}

		for (int i = highOut; i < tableElementCnt; ++i)
		{
				lut.emplace_back(255);
		}

		return lut;
}

/*static*/std::vector<std::uint8_t> Utils::Create8BitLookUpTableFromGamma(int tableElementCnt, float blackPoint, float whitePoint, float gamma)
{
		return InternalCreate8BitLookUpTableFromGamma(tableElementCnt, blackPoint, whitePoint, gamma);
}

/*static*/std::vector<libCZI::IDisplaySettings::SplineData> Utils::CalcSplineDataFromPoints(int pointCnt, std::function< std::tuple<double, double>(int idx)> getPoint)
{
		auto coeffs = CSplines::GetSplineCoefficients(
				pointCnt + 2,
				[&](int index, double* px, double* py)->void
				{
						if (index == 0)
						{
								if (px != nullptr) { *px = 0; }
								if (py != nullptr) { *py = 0; }
						}
						else if (index == pointCnt + 1)
						{
								if (px != nullptr) { *px = 1; }
								if (py != nullptr) { *py = 1; }
						}
						else
						{
								auto pt = getPoint(index - 1);
								if (px != nullptr) { *px = get<0>(pt); }
								if (py != nullptr) { *py = get<1>(pt); }
						}
				});

		std::vector<libCZI::IDisplaySettings::SplineData> splineData; splineData.reserve(coeffs.size());

		for (int i = 0; i < (int)coeffs.size(); ++i)
		{
				double xCoord = (i == 0) ? 0. : get<0>(getPoint(i - 1));
				IDisplaySettings::SplineData spD{ xCoord, coeffs.at(i) };
				splineData.push_back(spD);
		}

		return splineData;
}

/*static*/std::shared_ptr<libCZI::IBitmapData> Utils::NearestNeighborResize(libCZI::IBitmapData* bmSrc, int dstWidth, int dstHeight)
{
		auto bmDest = GetSite()->CreateBitmap(bmSrc->GetPixelType(), dstWidth, dstHeight);
		CBitmapOperations::NNResize(bmSrc, bmDest.get());
		return bmDest;
}

/*static*/std::shared_ptr<libCZI::IBitmapData > Utils::NearestNeighborResize(libCZI::IBitmapData* bmSrc, int dstWidth, int dstHeight, const DblRect& roiSrc, const DblRect& roiDest)
{
		auto bmDest = GetSite()->CreateBitmap(bmSrc->GetPixelType(), dstWidth, dstHeight);
		CBitmapOperations::Fill(bmDest.get(), { 0,0,0 });

		ScopedBitmapLockerSP lckDest{ bmDest };
		ScopedBitmapLockerP lckSrc{ bmSrc };
		CBitmapOperations::NNResizeInfo2Dbl resizeInfo;
		resizeInfo.srcPtr = lckSrc.ptrDataRoi;
		resizeInfo.srcStride = lckSrc.stride;
		resizeInfo.srcWidth = bmSrc->GetWidth();
		resizeInfo.srcHeight = bmSrc->GetHeight();
		resizeInfo.srcRoiX = roiSrc.x;
		resizeInfo.srcRoiY = roiSrc.y;
		resizeInfo.srcRoiW = roiSrc.w;
		resizeInfo.srcRoiH = roiSrc.h;
		resizeInfo.dstPtr = lckDest.ptrDataRoi;
		resizeInfo.dstStride = lckDest.stride;
		resizeInfo.dstWidth = bmDest->GetWidth();
		resizeInfo.dstHeight = bmDest->GetHeight();
		resizeInfo.dstRoiX = roiDest.x;
		resizeInfo.dstRoiY = roiDest.y;
		resizeInfo.dstRoiW = roiDest.w;
		resizeInfo.dstRoiH = roiDest.h;

		CBitmapOperations::NNScale2(bmSrc->GetPixelType(), bmDest->GetPixelType(), resizeInfo);

		return bmDest;
}

/*static*/const char* Utils::PixelTypeToInformalString(libCZI::PixelType pixeltype)
{
		switch (pixeltype)
		{
		case PixelType::Invalid:			return "invalid";
		case PixelType::Gray8:				return "gray8";
		case PixelType::Gray16:				return "gray16";
		case PixelType::Gray32Float:		return "gray32float";
		case PixelType::Bgr24:				return "bgr24";
		case PixelType::Bgr48:				return "bgr48";
		case PixelType::Bgr96Float:			return "bgr96float";
		case PixelType::Bgra32:				return "bgra32";
		case PixelType::Gray64ComplexFloat: return "gray64complexfloat";
		case PixelType::Bgr192ComplexFloat: return "bgr192complexfloat";
		case PixelType::Gray32:				return "gray32";
		case PixelType::Gray64Float:		return "gray64float";
		}

		return "illegal value";
}

/*static*/std::uint8_t Utils::GetBytesPerPixel(libCZI::PixelType pixelType)
{
		return CziUtils::GetBytesPerPel(pixelType);
}

/*static*/const char* Utils::CompressionModeToInformalString(libCZI::CompressionMode compressionMode)
{
		switch (compressionMode)
		{
		case CompressionMode::UnCompressed:
				return "uncompressed";
		case CompressionMode::Jpg:
				return "jpg";
		case CompressionMode::JpgXr:
				return "jpgxr";
		case CompressionMode::Zstd0:
				return "zstd0";
		case CompressionMode::Zstd1:
				return "zstd1";
		case CompressionMode::Invalid:
				return "invalid";
		}

		return "illegal value";
}

/*static*/std::string Utils::DimCoordinateToString(const libCZI::IDimCoordinate* coord)
{
		stringstream ss;
		for (int i = (int)(libCZI::DimensionIndex::MinDim); i <= (int)(libCZI::DimensionIndex::MaxDim); ++i)
		{
				int value;
				if (coord->TryGetPosition((libCZI::DimensionIndex)i, &value))
				{
						ss << DimensionToChar((libCZI::DimensionIndex)i) << value;
				}
		}

		return ss.str();
}

/*static*/bool Utils::StringToDimCoordinate(const char* sz, libCZI::CDimCoordinate* coord)
{
		try
		{
				if (coord != nullptr)
				{
						*coord = CDimCoordinate::Parse(sz);
				}
				else
				{
						CDimCoordinate::Parse(sz);
				}

				return true;
		}
		catch (LibCZIStringParseException& /*excp*/)
		{
				return false;
		}
}

/*static*/std::string Utils::DimBoundsToString(const libCZI::IDimBounds* bounds)
{
		stringstream ss;
		for (int i = (int)(libCZI::DimensionIndex::MinDim); i <= (int)(libCZI::DimensionIndex::MaxDim); ++i)
		{
				int start, size;
				if (bounds->TryGetInterval((libCZI::DimensionIndex)i, &start, &size))
				{
						ss << DimensionToChar((libCZI::DimensionIndex)i) << start << ':' << size;
				}
		}

		return ss.str();
}

/*static*/std::shared_ptr<libCZI::IIndexSet> Utils::IndexSetFromString(const std::wstring& s)
{
		return std::make_shared<CIndexSet>(s);
}

/*static*/libCZI::PixelType Utils::TryDeterminePixelTypeForChannel(libCZI::ISubBlockRepository* repository, int channelIdx)
{
		SubBlockInfo info;
		bool b = repository->TryGetSubBlockInfoOfArbitrarySubBlockInChannel(channelIdx, info);
		if (b == false)
		{
				return PixelType::Invalid;
		}

		return info.pixelType;
}

/*static*/int Utils::Compare(const IDimCoordinate* a, const IDimCoordinate* b)
{
		// algorithm:
		// We check all dimensions (in the ordinal order of the enums), then if 
		// coordinateA(dim) is valid and coordinateB(dim) is invalid -> a > b
		// coordinateA(dim) is invalid and coordinateB(dim) is valid -> a < b
		// if both are valid, then the coordinate-values are determined and compared
		for (auto i = static_cast<std::underlying_type<libCZI::DimensionIndex>::type>(libCZI::DimensionIndex::MinDim); i <= static_cast<std::underlying_type<libCZI::DimensionIndex>::type>(libCZI::DimensionIndex::MaxDim); ++i)
		{
				int coord1, coord2;
				bool valida = a->TryGetPosition((DimensionIndex)i, &coord1);
				bool validb = b->TryGetPosition((DimensionIndex)i, &coord2);
				if (valida == true && validb == true)
				{
						if (coord1 > coord2)
						{
								return 1;
						}
						else if (coord1 < coord2)
						{
								return -1;
						}
				}
				else if (valida == true && validb == false)
				{
						return 1;
				}
				else if (valida == false && validb == true)
				{
						return -1;
				}
		}

		return 0;
}

/*static*/bool Utils::HasSameDimensions(const IDimCoordinate* a, const IDimCoordinate* b)
{
		for (auto i = static_cast<std::underlying_type<libCZI::DimensionIndex>::type>(libCZI::DimensionIndex::MinDim); i <= static_cast<std::underlying_type<libCZI::DimensionIndex>::type>(libCZI::DimensionIndex::MaxDim); ++i)
		{
			    const bool valida = a->IsValid(static_cast<DimensionIndex>(i));
				const bool validb = b->IsValid(static_cast<DimensionIndex>(i));
				if (valida != validb)
				{
						return false;
				}
		}

		return true;
}

/*static*/std::shared_ptr<ICziMetadataBuilder> Utils::CreateSubBlockMetadata(const std::function<bool(int, std::tuple<std::string, std::string>&)>& tagsEnum)
{
		auto md = make_shared<CCZiMetadataBuilder>(L"METADATA");
		auto n = md->GetRootNode();
		if (tagsEnum)
		{
				std::tuple<std::string, std::string> nodeNameAndValue;
				for (int i = 0;; ++i)
				{
						if (!tagsEnum(i, nodeNameAndValue))
						{
								break;
						}

						std::string nodeName = "Tags/" + get<0>(nodeNameAndValue);
						auto n2 = n->GetOrCreateChildNode(nodeName.c_str());
						n2->SetValue(get<1>(nodeNameAndValue).c_str());
				}
		}

		return md;
}

/*static*/bool Utils::EnumAllCoordinates(const libCZI::CDimBounds& bounds, const std::function<bool(std::uint64_t, const libCZI::CDimCoordinate& coord)>& func)
{
		if (bounds.IsEmpty())
		{
				return true;
		}

		CDimCoordinate coord;
		std::vector<DimensionIndex> dims;
		bounds.EnumValidDimensions(
				[&](libCZI::DimensionIndex dim, int start, int size)->bool
				{
						coord.Set(dim, start);
						dims.push_back(dim);
						return true;
				});

		uint64_t coordNo = 0;
		for (;;)
		{
				if (!func(coordNo++, coord))
				{
						return false;
				}

				for (size_t i = 0; i < dims.size(); ++i)
				{
						int v;
						coord.TryGetPosition(dims[i], &v);

						int minVal, sizeVal;
						bounds.TryGetInterval(dims[i], &minVal, &sizeVal);

						if (v < minVal + sizeVal - 1)
						{
								++v;
								coord.Set(dims[i], v);
								break;
						}
						else
						{
								if (i == dims.size() - 1)
								{
										return true;
								}

								for (size_t j = 0; j <= i; j++)
								{
										bounds.TryGetInterval(dims[j], &minVal, nullptr);
										coord.Set(dims[j], minVal);
								}
						}
				}
		}
}

/*static*/void Utils::FillBitmap(libCZI::IBitmapData* bm, const libCZI::RgbFloatColor& floatColor)
{
		CBitmapOperations::Fill(bm, floatColor);
}

/*static*/CompressionMode Utils::CompressionModeFromRawCompressionIdentifier(std::int32_t m)
{
		return CziUtils::CompressionModeFromInt(m);
}

/*static*/std::int32_t Utils::CompressionModeToCompressionIdentifier(CompressionMode mode)
{
		return CziUtils::CompressionModeToInt(mode);
}

Utils::CompressionOption Utils::ParseCompressionOptions(const std::string& options)
{
	const std::regex compressionOptionsRegex(R"(^\s*([a-zA-Z0-9]+)\s*:\s*((?:\s*[a-zA-Z0-9]*\s*=\s*[a-zA-Z0-9.+-]*\s*[;])*(?:\s*[a-zA-Z0-9]*\s*=\s*[a-zA-Z0-9.+-]*)?)\s*$)");
	std::smatch pieces_match;

	Utils::CompressionOption result;

	if (std::regex_match(options, pieces_match, compressionOptionsRegex))
	{
		if (pieces_match.size() == 3 && pieces_match[0].matched == true && pieces_match[1].matched == true && pieces_match[2].matched == true)
		{
			const string& compressionMethod = pieces_match[1].str();
			const string& parameters = pieces_match[2].str();

			libCZI::CompressionMode compressionMode;
			if (!_tryParseCompressionMode(compressionMethod, &compressionMode))
			{
				stringstream ss;
				ss << "Error parsing the compression-options - unknown method \"" << compressionMethod << "\"";
				throw logic_error(ss.str());
			}

			auto compressParametersOnMap = make_shared<libCZI::CompressParametersOnMap>();
			if (!_tryParseCompressionOptions(parameters, &compressParametersOnMap->map))
			{
				stringstream ss;
				ss << "Error parsing the compression-options - parameters could not be parsed (\"" << parameters << "\")";
				throw logic_error(ss.str());
			}

			result.first = compressionMode;
			result.second = compressParametersOnMap;

			return result;
		}
	}

	throw logic_error("The specified string could not be processed.");
}

// ----------------------------------------------------------------------------
