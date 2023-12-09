// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SingleChannelAccessorBase.h"
#include "BitmapOperations.h"
#include "utilities.h"

using namespace std;
using namespace libCZI;

bool CSingleChannelAccessorBase::TryGetPixelType(const libCZI::IDimCoordinate* planeCoordinate, libCZI::PixelType& pixeltype)
{
    int c = (numeric_limits<int>::min)();
    planeCoordinate->TryGetPosition(libCZI::DimensionIndex::C, &c);

    // the idea is: for the cornerstone-case where we do not have a C-index, the call to "TryGetSubBlockInfoOfArbitrarySubBlockInChannel"
    // will ignore the specified index _if_ there are no C-indices at all
    pixeltype = Utils::TryDeterminePixelTypeForChannel(this->sbBlkRepository.get(), c);
    return (pixeltype != PixelType::Invalid) ? true : false;
}

/*static*/void CSingleChannelAccessorBase::Clear(libCZI::IBitmapData* bm, const libCZI::RgbFloatColor& floatColor)
{
    if (!isnan(floatColor.r) && !isnan(floatColor.g) && !isnan(floatColor.b))
    {
        CBitmapOperations::Fill(bm, floatColor);
    }
}

void CSingleChannelAccessorBase::CheckPlaneCoordinates(const libCZI::IDimCoordinate* planeCoordinate) const
{
    // planeCoordinate must not contain S
    if (planeCoordinate->IsValid(DimensionIndex::S))
    {
        throw LibCZIInvalidPlaneCoordinateException("S-dimension is illegal for a plane.", LibCZIInvalidPlaneCoordinateException::ErrorCode::InvalidDimension);
    }

    static constexpr DimensionIndex DimensionsToCheck[] =
    { DimensionIndex::Z,DimensionIndex::C,DimensionIndex::T,DimensionIndex::R,DimensionIndex::I,DimensionIndex::H,DimensionIndex::V,DimensionIndex::B };

    SubBlockStatistics statistics = this->sbBlkRepository->GetStatistics();

    for (size_t i = 0; i < sizeof(DimensionsToCheck) / sizeof(DimensionsToCheck[0]); ++i)
    {
        auto d = DimensionsToCheck[i];
        int start, size;
        if (statistics.dimBounds.TryGetInterval(d, &start, &size))
        {
            // if the dimension is present in the dim-bounds, then it must be also given
            // in the plane-coordinate - with the sole exception that it can be absent if size is 1
            int co;
            if (planeCoordinate->TryGetPosition(d, &co) == false)
            {
                if (size > 1)
                {
                    stringstream ss;
                    ss << "Coordinate for dimension '" << Utils::DimensionToChar(d) << "' not given.";
                    throw LibCZIInvalidPlaneCoordinateException(ss.str().c_str(), LibCZIInvalidPlaneCoordinateException::ErrorCode::MissingDimension);
                }
            }
            else
            {
                if (co < start || co >= start + size)
                {
                    stringstream ss;
                    ss << "Coordinate for dimension '" << Utils::DimensionToChar(d) << "' is out-of-range.";
                    throw LibCZIInvalidPlaneCoordinateException(ss.str().c_str(), LibCZIInvalidPlaneCoordinateException::ErrorCode::CoordinateOutOfRange);
                }
            }
        }
        else
        {
            // if the dimension is not present in the dim-bounds, it must not be given in the plane-coordinate
            if (planeCoordinate->IsValid(d))
            {
                stringstream ss;
                ss << "Coordinate for dimension '" << Utils::DimensionToChar(d) << "' is not expected.";
                throw LibCZIInvalidPlaneCoordinateException(ss.str().c_str(), LibCZIInvalidPlaneCoordinateException::ErrorCode::SurplusDimension);
            }
        }
    }
}

std::vector<int> CSingleChannelAccessorBase::CheckForVisibility(const libCZI::IntRect& roi, int count, const std::function<int(int)>& get_subblock_index) const
{
    return CSingleChannelAccessorBase::CheckForVisibilityCore(
        roi,
        count,
        get_subblock_index,
        [this](int subblock_index) -> IntRect
            {
                SubBlockInfo subblock_info;
                bool b = this->sbBlkRepository->TryGetSubBlockInfo(subblock_index, &subblock_info);
                return subblock_info.logicalRect;
            });
}

/*static*/std::vector<int> CSingleChannelAccessorBase::CheckForVisibilityCore(const libCZI::IntRect& roi, int count, const std::function<int(int)>& get_subblock_index, const std::function<libCZI::IntRect(int)>& get_rect_of_subblock)
{
    std::vector<int> result;

    // handle the trivial cases
    if (count == 0 || !roi.IsNonEmpty())
    {
        return result;
    }

    const int64_t total_pixel_count = static_cast<int64_t>(roi.w) * roi.h;
    result.reserve(count);
    RectangleCoverageCalculator coverage_calculator;
    int64_t covered_pixel_count = 0;
    for (int i = count - 1; i >= 0; --i) // we start at the end, because that is the subblock which is rendered last (and thus is on top)
    {
        const int subblock_index = get_subblock_index(i);
        coverage_calculator.AddRectangle(get_rect_of_subblock(subblock_index));
        const int64_t new_covered_pixel_count = coverage_calculator.CalcAreaOfIntersectionWithRectangle(roi);
        if (new_covered_pixel_count > covered_pixel_count)  // if the covered pixel count has increased, it means that this subblock covers some new pixels,
        {                                                   //  some pixels which were not overdrawn by all the previous ones
            // this means - when drawing this subblock, some new pixels will be covered which were not covered before,
            //  so we need to draw this subblock, therefore we add it to our result vector
            result.push_back(i);

            covered_pixel_count = new_covered_pixel_count;
            if (new_covered_pixel_count == total_pixel_count)
            {
                // if the whole ROI is covered now, then we are done
                break;
            }
        }
    }

    // now, reverse the result vector, so that the subblocks are in the order in which they are to be rendered
    std::reverse(result.begin(), result.end());
    return result;
}

/*static*/CSingleChannelAccessorBase::SubBlockData CSingleChannelAccessorBase::GetSubBlockDataForSubBlockIndex(
    const std::shared_ptr<libCZI::ISubBlockRepository>& sbBlkRepository,
    const std::shared_ptr<libCZI::ISubBlockCacheOperation>& cache,
    int subBlockIndex,
    bool onlyAddCompressedSubBlockToCache)
{
    SubBlockData result;

    // if no cache-object is given, then we simply read the subblock and create a bitmap from it
    if (!cache)
    {
        const auto subblock = sbBlkRepository->ReadSubBlock(subBlockIndex);
        result.bitmap = subblock->CreateBitmap();
        result.subBlockInfo = subblock->GetSubBlockInfo();
    }
    else
    {
        const auto bitmap_from_cache = cache->Get(subBlockIndex);
        if (bitmap_from_cache)
        {
            const bool b = sbBlkRepository->TryGetSubBlockInfo(subBlockIndex, &result.subBlockInfo);
            if (!b)
            {
                stringstream ss;
                ss << "SubBlockInfo not found in repository for subblock index " << subBlockIndex << ".";
                throw logic_error(ss.str());
            }

            result.bitmap = bitmap_from_cache;
        }
        else
        {
            const auto subblock = sbBlkRepository->ReadSubBlock(subBlockIndex);
            result.bitmap = subblock->CreateBitmap();
            result.subBlockInfo = subblock->GetSubBlockInfo();
            if (!onlyAddCompressedSubBlockToCache || result.subBlockInfo.GetCompressionMode() != CompressionMode::UnCompressed)
            {
                cache->Add(subBlockIndex, result.bitmap);
            }
        }
    }

    return result;
}
