// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"
#include "../libCZI/utilities.h"

using namespace libCZI;
using namespace std;

struct CoverageCoverageCalculatorFixture : public testing::TestWithParam<tuple<vector<IntRect>, __int64>> {};

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
