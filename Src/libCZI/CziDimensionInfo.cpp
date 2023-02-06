// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "stdafx.h"
#include "CziDimensionInfo.h"

/*virtual*/ libCZI::DimensionIndex CCziDimensionInfo::GetDimension() const
{
    return this->dimAndStartSize_.dimension;
}

/*virtual*/ void CCziDimensionInfo::GetInterval(int* start, int* end) const
{
    if (start != nullptr)
    {
        *start = this->dimAndStartSize_.start;
    }

    if (end != nullptr)
    {
        *end = this->dimAndStartSize_.size + this->dimAndStartSize_.start;
    }
}

/*virtual*/ void CCziDimensionInfo::GetIntervalIsExplicitlyStated(bool* startExplicitlyStated, bool* endExplicitlyStated) const
{
    if (startExplicitlyStated != nullptr)
    {
        *startExplicitlyStated = this->startExplicitlyStated_;
    }

    if (endExplicitlyStated != nullptr)
    {
        *endExplicitlyStated = this->endExplicitlyStated_;
    }
}

// ----------------------------------------------------------------------------------

CCziDimensionZInfo::CCziDimensionZInfo() :
    type(DefType::None),
    referencePosValid(false),
    xyzHandednessValid(false),
    zaxisDirectionValid(false),
    zdriveModeValid(false),
    zdriveSpeedValid(false)
{
}

void CCziDimensionZInfo::SetIntervalDefinition(const double& start, const double& increment)
{
    this->intervalIncrement = increment;
    this->intervalStart = start;
    this->type = DefType::Interval;
}

void CCziDimensionZInfo::SetListDefinition(std::vector<double>&& list)
{
    this->data = list;
    this->type = DefType::List;
}

void CCziDimensionZInfo::SetStartPosition(const double& startPos)
{
    this->referencePos = startPos;
    this->referencePosValid = true;
}

void CCziDimensionZInfo::SetXyzHandedness(libCZI::IDimensionZInfo::XyzHandedness handedness)
{
    this->xyzHandedness = handedness;
    this->xyzHandednessValid = true;
}

void CCziDimensionZInfo::SetZAxisDirection(libCZI::IDimensionZInfo::ZaxisDirection zaxisDirection)
{
    this->zaxisDirection = zaxisDirection;
    this->zaxisDirectionValid = true;
}

void CCziDimensionZInfo::SetZDriveMode(libCZI::IDimensionZInfo::ZDriveMode zdrivemode)
{
    this->zdriveMode = zdrivemode;
    this->zdriveModeValid = true;
}

void CCziDimensionZInfo::SetZDriveSpeed(const double& d)
{
    this->zdriveSpeed = d;
    this->zdriveSpeedValid = true;
}

bool CCziDimensionZInfo::TryGetReferencePosition(double* d) /*override*/
{
    if (this->referencePosValid == true)
    {
        if (d != nullptr)
        {
            *d = this->referencePos;
        }

        return true;
    }

    return false;
}

bool CCziDimensionZInfo::TryGetIntervalDefinition(double* offset, double* increment) /*override*/
{
    if (this->type == DefType::Interval)
    {
        if (offset != nullptr)
        {
            *offset = this->intervalStart;
        }

        if (increment != nullptr)
        {
            *increment = this->intervalIncrement;
        }

        return true;
    }

    return false;
}

bool CCziDimensionZInfo::TryGetPositionList(std::vector<double>* positions) /*override*/
{
    if (this->type == DefType::List)
    {
        if (positions != nullptr)
        {
            *positions = this->data;
        }

        return true;
    }

    return false;
}

bool CCziDimensionZInfo::TryGetXyzHandedness(XyzHandedness* xyzHandedness) /*override*/
{
    if (this->xyzHandednessValid)
    {
        if (xyzHandedness != nullptr)
        {
            *xyzHandedness = this->xyzHandedness;
        }

        return true;
    }

    return false;
}

bool CCziDimensionZInfo::TryGetZAxisDirection(ZaxisDirection* zAxisDirection) /*override*/
{
    if (this->zaxisDirectionValid)
    {
        if (zAxisDirection != nullptr)
        {
            *zAxisDirection = this->zaxisDirection;
        }

        return true;
    }

    return false;
}

bool CCziDimensionZInfo::TryGetZDriveMode(ZDriveMode* zdrivemode) /*override*/
{
    if (this->zdriveModeValid)
    {
        if (zdrivemode != nullptr)
        {
            *zdrivemode = this->zdriveMode;
        }

        return true;
    }

    return false;
}

bool CCziDimensionZInfo::TryZDriveSpeed(double* zdrivespeed) /*override*/
{
    if (this->zdriveSpeedValid)
    {
        if (zdrivespeed != nullptr)
        {
            *zdrivespeed = this->zdriveSpeed;
        }

        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------------

CCziDimensionTInfo::CCziDimensionTInfo() :
    type(DefType::None),
    startTimeValid(false)
{
}

void CCziDimensionTInfo::SetStartTime(const libCZI::XmlDateTime& dateTime)
{
    this->startTime = dateTime;
    this->startTimeValid = true;
}

void CCziDimensionTInfo::SetIntervalDefinition(const double& start, const double& increment)
{
    this->intervalIncrement = increment;
    this->intervalStart = start;
    this->type = DefType::Interval;
}

void CCziDimensionTInfo::SetListDefinition(std::vector<double>&& list)
{
    this->offsets = list;
    this->type = DefType::List;
}

bool CCziDimensionTInfo::TryGetStartTime(libCZI::XmlDateTime* dateTime) /*override*/
{
    if (this->startTimeValid)
    {
        if (dateTime != nullptr)
        {
            *dateTime = this->startTime;
        }

        return true;
    }

    return false;
}

bool CCziDimensionTInfo::TryGetIntervalDefinition(double* offset, double* increment) /*override*/
{
    if (this->type == DefType::Interval)
    {
        if (offset != nullptr)
        {
            *offset = this->intervalStart;
        }

        if (increment != nullptr)
        {
            *increment = this->intervalIncrement;
        }

        return true;
    }

    return false;
}

bool CCziDimensionTInfo::TryGetOffsetsList(std::vector<double>* offsets) /*override*/
{

    if (this->type == DefType::List)
    {
        if (offsets != nullptr)
        {
            *offsets = this->offsets;
        }

        return true;
    }

    return false;
}
