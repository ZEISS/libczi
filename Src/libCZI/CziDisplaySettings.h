// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once
#include "libCZI.h"
#include "splines.h"
#include "pugixml.hpp"

class CChannelDisplaySettingsOnPod : public libCZI::IChannelDisplaySetting
{
private:
	libCZI::ChannelDisplaySettingsPOD cdsPod;
public:
	CChannelDisplaySettingsOnPod(const libCZI::ChannelDisplaySettingsPOD& pod)
		: cdsPod(pod)
	{}

public:	// interface IChannelDisplaySetting
	bool GetIsEnabled() const override;
	float GetWeight() const override;
	bool TryGetTintingColorRgb8(libCZI::Rgb8Color* pColor) const override;
	void GetBlackWhitePoint(float* pBlack, float* pWhite) const override;
	libCZI::IDisplaySettings::GradationCurveMode GetGradationCurveMode() const override;
	bool TryGetGamma(float* gamma) const override;
	bool TryGetSplineControlPoints(std::vector<libCZI::IDisplaySettings::SplineControlPoint>* ctrlPts) const override;
	bool TryGetSplineData(std::vector<libCZI::IDisplaySettings::SplineData>* data) const override;
};

class CDisplaySettingsOnPod : public libCZI::IDisplaySettings
{
private:
	std::map<int, std::shared_ptr<libCZI::IChannelDisplaySetting>> channelDs;
public:
	explicit CDisplaySettingsOnPod(std::function<bool(int no, int&, libCZI::ChannelDisplaySettingsPOD& dispSetting)> getChannelDisplaySettings);
	CDisplaySettingsOnPod(const libCZI::DisplaySettingsPOD& pod);

	static std::shared_ptr<libCZI::IDisplaySettings> CreateFromXml(pugi::xml_node node);
public:	// interface IDisplaySettings
	void EnumChannels(std::function<bool(int)> func) const override;
	std::shared_ptr<libCZI::IChannelDisplaySetting> GetChannelDisplaySettings(int chIndex) const override;
};

