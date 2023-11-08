// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"
#include "../libCZI/utilities.h"
#include <random>

using namespace libCZI;
using namespace std;

/// A simplistic reference implementation of a rectangle coverage calculator. We calculate
/// what area of the query rectangle is covered by the given vector of rectangles.
///
/// \param  rectangles  The vector of rectangles.
/// \param  queryRect   The query rectangle.
/// \returns            The area of the query rectangle being covered by the rectangles of the rectangles vector.
static int64_t CalcAreaOfIntersectionWithRectangleReference(const vector<IntRect>& rectangles, const IntRect& queryRect)
{
    // what we do here is:
    // - we create a bitmap of the size of the query rectangle
    // - we then fill the bitmap with 0xff in the areas that are covered by the rectangles
    // - then we count how many pixels are set to 0xff
    const auto bitmap = CStdBitmapData::Create(PixelType::Gray8, queryRect.w, queryRect.h);
    const ScopedBitmapLockerSP bitmap_locked{ bitmap };
    CBitmapOperations::Fill_Gray8(bitmap->GetWidth(), bitmap->GetHeight(), bitmap_locked.ptrDataRoi, bitmap_locked.stride, 0);
    for (const auto& rect : rectangles)
    {
        const auto intersection = rect.Intersect(queryRect);
        if (!intersection.IsValid())
        {
            continue;
        }

        CBitmapOperations::Fill_Gray8(
            intersection.w,
            intersection.h,
            static_cast<uint8_t*>(bitmap_locked.ptrDataRoi) + intersection.x + intersection.y * static_cast<size_t>(bitmap_locked.stride),
            bitmap_locked.stride,
            0xff);
    }

    // now, we simply have to count the number of pixels that are set to 0xff
    int64_t set_pixel_count = 0;
    for (uint32_t y = 0; y < bitmap->GetHeight(); ++y)
    {
        const uint8_t* ptr = static_cast<const uint8_t*>(bitmap_locked.ptrDataRoi) + y * static_cast<size_t>(bitmap_locked.stride);
        for (uint32_t x = 0; x < bitmap->GetWidth(); ++x)
        {
            if (*ptr++ == 0xff)
            {
                ++set_pixel_count;
            }
        }
    }

    return set_pixel_count;
}

TEST(CoverageCalculator, RandomRectanglesCompareWithReferenceImplementation)
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<int> distribution(0, 99); // distribution in range [0, 99]

    static constexpr IntRect kQueryRect{ 0, 0, 100, 100 };

    for (int repeat = 0; repeat < 10; repeat++)
    {
        const int number_of_rectangles = 1 + distribution(rng);

        vector<IntRect> rectangles;
        rectangles.reserve(number_of_rectangles);
        for (int i = 0; i < number_of_rectangles; ++i)
        {
            rectangles.emplace_back(IntRect{ distribution(rng), distribution(rng), 1 + distribution(rng), 1 + distribution(rng) });
        }

        const int64_t reference_result_for_covered_area = CalcAreaOfIntersectionWithRectangleReference(rectangles, kQueryRect);

        RectangleCoverageCalculator calculator;
        calculator.AddRectangles(rectangles.cbegin(), rectangles.cend());
        const int64_t total_covered_area = calculator.CalcAreaOfIntersectionWithRectangle(kQueryRect);
        EXPECT_EQ(reference_result_for_covered_area, total_covered_area);
    }
}

struct CoverageCoverageCalculatorFixture : public testing::TestWithParam<tuple<vector<IntRect>, int64_t>> {};

TEST_P(CoverageCoverageCalculatorFixture, CreateDocumentAndUseSingleChannelScalingTileAccessorWithSortByMAndCheckResult)
{
    const auto parameters = GetParam();

    RectangleCoverageCalculator calculator;
    calculator.AddRectangles(get<0>(parameters).cbegin(), get<0>(parameters).cend());

    const int64_t totalCoveredArea = calculator.CalcAreaOfIntersectionWithRectangle({ 0, 0, 100, 100 });
    EXPECT_EQ(totalCoveredArea, get<1>(parameters));
}

INSTANTIATE_TEST_SUITE_P(
    CoverageCalculator,
    CoverageCoverageCalculatorFixture,
    testing::Values(
    //  Non-Overlapping Rectangles
    make_tuple(vector<IntRect>{ IntRect{ 10, 10, 20, 20 }, IntRect{ 40, 40, 20, 20 }, IntRect{ 70, 70, 20, 20 } }, 1200),
    // Partially Overlapping Rectangles
    make_tuple(vector<IntRect>{ IntRect{ 10, 10, 30, 30 }, IntRect{ 20, 20, 30, 30 }, IntRect{ 30, 30, 30, 30 } }, 1900),
    // Fully Overlapping Rectangles
    make_tuple(vector<IntRect>{ IntRect{ 10, 10, 30, 30 }, IntRect{ 10, 10, 30, 30 }}, 900),
    //  Rectangles Completely Outside the Query Rectangle
    make_tuple(vector<IntRect>{ IntRect{ -40, -50, 30, 30 }, IntRect{ 110, 110, 30, 30 }}, 0),
    // Rectangles Partially Outside the Main Rectangle
    make_tuple(vector<IntRect>{ IntRect{ 90, 90, 20, 20 }, IntRect{ -10, 0, 20, 100 }}, 1100),
    // Combination of Overlapping and Non-Overlapping Rectangles
    make_tuple(vector<IntRect>{ IntRect{ 10, 10, 30, 30 }, IntRect{ 40, 40, 30, 30 }, IntRect{ 20, 20, 50, 50 } }, 3000),
    // FullyOverlappingRectangles
    make_tuple(vector<IntRect>{ IntRect{ 10, 10, 20, 20 }, IntRect{ 10, 10, 20, 20 }, IntRect{ 10, 10, 20, 20 } }, 400),
    // Partially Overlapping Rectangles
    make_tuple(vector<IntRect>{ IntRect{ 10, 10, 40, 40 }, IntRect{ 30, 30, 30, 30 }, IntRect{ 65, 65, 25, 25 } }, 2725)
));
