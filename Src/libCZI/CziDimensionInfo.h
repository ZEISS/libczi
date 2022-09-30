// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "libCZI.h"
#include "CziMetadata.h"

class CCziDimensionInfo : public libCZI::IDimensionInfo
{
private:
	libCZI::DimensionAndStartSize dimAndStartSize_;
	bool startExplicitlyStated_;
	bool endExplicitlyStated_;
public:
	CCziDimensionInfo(const libCZI::DimensionAndStartSize& dimAndStartSize, bool startExplicitlyStated, bool endExplicitlyStated)
		: dimAndStartSize_(dimAndStartSize), startExplicitlyStated_(startExplicitlyStated), endExplicitlyStated_(endExplicitlyStated)
	{}

	virtual ~CCziDimensionInfo() override = default;
public: // interface IDimensionInfo
    libCZI::DimensionIndex GetDimension() const override;
	virtual void GetInterval(int* start, int* end) const override;
	virtual void GetIntervalIsExplicitlyStated(bool* startExplicitlyStated, bool* endExplicitlyStated) const override;
};

class CCziDimensionZInfo : public libCZI::IDimensionZInfo
{
private:
	enum class DefType : std::uint8_t
	{
		None,
		Interval,
		List
	};

	DefType	type;
	double intervalStart;
	double intervalIncrement;
	std::vector<double> data;
	bool referencePosValid;
	double referencePos;
	bool xyzHandednessValid;
	libCZI::IDimensionZInfo::XyzHandedness xyzHandedness;
	bool zaxisDirectionValid;
	libCZI::IDimensionZInfo::ZaxisDirection zaxisDirection;
	bool zdriveModeValid;
	libCZI::IDimensionZInfo::ZDriveMode zdriveMode;
	bool zdriveSpeedValid;
	double zdriveSpeed;
public:
	CCziDimensionZInfo();

	void SetIntervalDefinition(const double& start, const double& increment);
	void SetListDefinition(std::vector<double>&& list);
	void SetStartPosition(const double& startPos);
	void SetXyzHandedness(libCZI::IDimensionZInfo::XyzHandedness handedness);
	void SetZAxisDirection(libCZI::IDimensionZInfo::ZaxisDirection zaxisDirection);
	void SetZDriveMode(libCZI::IDimensionZInfo::ZDriveMode zdrivemode);
	void SetZDriveSpeed(const double& d);
public:
	bool TryGetReferencePosition(double* d) override;
	bool TryGetIntervalDefinition(double* start, double* increment) override;
	bool TryGetPositionList(std::vector<double>* positions) override;
	bool TryGetXyzHandedness(XyzHandedness* xyzHandedness) override;
	bool TryGetZAxisDirection(ZaxisDirection* zAxisDirection) override;
	bool TryGetZDriveMode(ZDriveMode* zdrivemode) override;
	bool TryZDriveSpeed(double* zdrivespeed) override;
};

class CCziDimensionTInfo : public libCZI::IDimensionTInfo
{
private:
	enum class DefType : std::uint8_t
	{
		None,
		Interval,
		List
	};

	DefType	type;
	double intervalStart;
	double intervalIncrement;
	std::vector<double> offsets;
	bool startTimeValid;
	libCZI::XmlDateTime startTime;
public:
	CCziDimensionTInfo();

	void SetStartTime(const libCZI::XmlDateTime& dateTime);
	void SetIntervalDefinition(const double& start, const double& increment);
	void SetListDefinition(std::vector<double>&& list);
public:
	bool TryGetStartTime(libCZI::XmlDateTime* dateTime) override;
	bool TryGetIntervalDefinition(double* offset, double* increment) override;
	bool TryGetOffsetsList(std::vector<double>* offsets) override;
};