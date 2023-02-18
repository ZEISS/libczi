// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <functional>
#include <vector>
#include "libCZI_Metadata.h"

class CSplines
{
public:
    typedef libCZI::IDisplaySettings::CubicSplineCoefficients Coefficients;

    static std::vector<Coefficients> GetSplineCoefficients(int pointsCnt, const std::function<void(int index, double* x, double* y)>& getPoint);

    static double CalculateSplineValue(double xPosition, const Coefficients& coeffs);
};
