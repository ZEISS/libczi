// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "splines.h"
#include <cmath>
#include <Eigen/Eigen>

using namespace Eigen;
using namespace libCZI::detail;
using namespace std;

/*static*/std::vector<CSplines::Coefficients> CSplines::GetSplineCoefficients(int pointsCnt, const std::function<void(int index, double* x, double* y)>& getPoint)
{
    if (!getPoint || pointsCnt < 2)
    {
        throw invalid_argument("Not enough data points for spline fitting.");
    }

    const int n = pointsCnt - 1;
    std::vector<CSplines::Coefficients> splineCoefficients;
    splineCoefficients.reserve(n);

    // Initialization of the matrix A and the right hand side (rhs) for the linear equation system A * x = rhs.
    // The solution x contains the coefficients b_1, ..., b_n-1. 
    // The coefficients b_0 and b_n are free variables, and we set b_0 = b_n = 0.
    // (b_n does not appear in a spline but it is necessary for the evaluation of a_(n-1) and c_(n-1)). 
    // Matrix indices: First index is row index, second index is column index.

    MatrixXd matrix; matrix.resize(n - 1, n - 1);
    matrix.setZero();
    VectorXd rhs; rhs.resize(n - 1);

    double pt1x, pt1y, pt0x, pt0y;
    getPoint(0, &pt0x, &pt0y);
    getPoint(1, &pt1x, &pt1y);
    double dx1, dy1;
    double dx2 = pt1x - pt0x;
    double dy2 = pt1y - pt0y;

    for (int i = 0; i < n - 1; i++)
    {
        dx1 = dx2;

        getPoint(i + 1, &pt0x, &pt0y);
        getPoint(i + 2, &pt1x, &pt1y);

        dx2 = pt1x - pt0x;

        dy1 = dy2;
        dy2 = pt1y - pt0y;

        // Diagonal entry
        matrix(i, i) = 2 * (dx1 + dx2);

        if (i != n - 2)
        {
            // Secondary diagonal entries
            matrix(i + 1, i) = dx2;
            matrix(i, i + 1) = dx2;
        }

        if (abs(dx1) < 0.001)
        {
            dx1 = 0.001;
        }

        if (abs(dx2) < 0.001)
        {
            dx2 = 0.001;
        }

        rhs[i] = 3 * ((dy2 / dx2) - (dy1 / dx1));
    }

    VectorXd resultVector = matrix.colPivHouseholderQr().solve(rhs);

    // Resolve the coefficients for the spline curve.
    double x_i = 0;
    for (int i = 0; i < n; i++)
    {
        const double x_i_plus_1 = i < (n - 1) ? resultVector(i) : 0;
        Coefficients coeffs;
        getPoint(i, &pt0x, &pt0y);
        getPoint(i + 1, &pt1x, &pt1y);

        dx1 = pt1x - pt0x;
        dy1 = pt1y - pt0y;

        if (abs(dx1) < 0.0000001)
        {
            coeffs.a = 0;
            coeffs.c = 0;
        }
        else
        {
            coeffs.a = (x_i_plus_1 - x_i) / (3 * dx1);
            coeffs.c = (dy1 / dx1) - ((dx1 * (x_i_plus_1 + (2 * x_i))) / 3);
        }

        coeffs.b = x_i;
        coeffs.d = pt0y;
        splineCoefficients.push_back(coeffs);
        x_i = x_i_plus_1;
    }

    return splineCoefficients;
}

/*static*/double CSplines::CalculateSplineValue(double xPosition, const CSplines::Coefficients& coeffs)
{
    constexpr int n = 4;    // cubic spline, polynomial number is 4

    double y = 0;
    double xPower = 1;

    for (int i = n - 1; i >= 0; i--)
    {
        y += xPower * coeffs.Get(i);
        xPower *= xPosition;
    }

    return y;
}
