// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "pch.h"
#include "inc_libCZI.h"

using namespace libCZI;

TEST(Splines, Splines1)
{
    static const double Points[] = { 0,0,0.362559241706161,0.876190476190476 ,0.554502369668246,0.561904761904762 ,1,1 };

    auto result = CSplines::GetSplineCoefficients(
        sizeof(Points) / sizeof(Points[0]) / 2,
        [](int idx, double* pX, double* pY)->void
        {
            idx *= 2;
            if (pX != nullptr) { *pX = Points[idx]; }
            if (pY != nullptr) { *pY = Points[idx + 1]; }
        });

    EXPECT_EQ(result.size(), (size_t)3) << "Incorrect result";

    EXPECT_NEAR(result[0].a, -11.360115103033465, 1e-6) << "Incorrect result";
    EXPECT_NEAR(result[0].b, 0, 1e-6) << "Incorrect result";
    EXPECT_NEAR(result[0].c, 3.9099603132098, 1e-6) << "Incorrect result";
    EXPECT_NEAR(result[0].d, 0, 1e-6) << "Incorrect result";

    EXPECT_NEAR(result[1].a, 35.39860240958761, 1e-6) << "Incorrect result";
    EXPECT_NEAR(result[1].b, -12.356144152351561, 1e-6) << "Incorrect result";
    EXPECT_NEAR(result[1].c, -0.5698739410787983, 1e-6) << "Incorrect result";
    EXPECT_NEAR(result[1].d, 0.876190476190476, 1e-6) << "Incorrect result";

    EXPECT_NEAR(result[2].a, -6.0063254490025031, 1e-6) << "Incorrect result";
    EXPECT_NEAR(result[2].b, 8.0274112635957717, 1e-6) << "Incorrect result";
    EXPECT_NEAR(result[2].c, -1.4007444718589364, 1e-6) << "Incorrect result";
    EXPECT_NEAR(result[2].d, 0.561904761904762, 1e-6) << "Incorrect result";
}
