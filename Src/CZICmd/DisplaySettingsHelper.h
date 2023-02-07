// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "inc_libCZI.h"
#include <memory>

class ChannelDisplaySettingsWrapper :public libCZI::IChannelDisplaySetting
{
private:
    const ChannelDisplaySettings& chDisplSettings;
public:
    explicit ChannelDisplaySettingsWrapper(const ChannelDisplaySettings& dspl)
        : chDisplSettings(dspl)
    {}

public:
    bool GetIsEnabled()const  override { return true; }
    float GetWeight() const override { return this->chDisplSettings.weight; }
    bool TryGetTintingColorRgb8(libCZI::Rgb8Color* pColor) const override
    {
        if (this->chDisplSettings.enableTinting == true)
        {
            if (pColor != nullptr)
            {
                *pColor = this->chDisplSettings.tinting.color;
            }

            return true;
        }

        return false;
    }

    void GetBlackWhitePoint(float* pBlack, float* pWhite) const override
    {
        if (pBlack != nullptr) { *pBlack = this->chDisplSettings.blackPoint; }
        if (pWhite != nullptr) { *pWhite = this->chDisplSettings.whitePoint; }
    }

    libCZI::IDisplaySettings::GradationCurveMode GetGradationCurveMode() const override
    {
        if (this->chDisplSettings.IsGammaValid())
        {
            return libCZI::IDisplaySettings::GradationCurveMode::Gamma;
        }

        if (this->chDisplSettings.IsSplinePointsValid())
        {
            return libCZI::IDisplaySettings::GradationCurveMode::Spline;
        }

        return libCZI::IDisplaySettings::GradationCurveMode::Linear;
    }

    bool TryGetGamma(float* gamma)const override
    {
        if (this->GetGradationCurveMode() == libCZI::IDisplaySettings::GradationCurveMode::Gamma)
        {
            if (gamma != nullptr) { *gamma = this->chDisplSettings.gamma; }
            return true;
        }

        return false;
    }

    bool TryGetSplineControlPoints(std::vector<libCZI::IDisplaySettings::SplineControlPoint>* ctrlPts) const override
    {
        throw std::runtime_error("not implemented");
    }

    bool TryGetSplineData(std::vector<libCZI::IDisplaySettings::SplineData>* data) const override
    {
        if (this->GetGradationCurveMode() == libCZI::IDisplaySettings::GradationCurveMode::Spline)
        {
            if (data != nullptr)
            {
                *data = libCZI::Utils::CalcSplineDataFromPoints(
                    (int)this->chDisplSettings.splinePoints.size(),
                    [&](int idx)->std::tuple<double, double>
                    {
                        return this->chDisplSettings.splinePoints.at(idx);
                    });
            }

            return true;
        }

        return false;
    }
};

class CDisplaySettingsWrapper : public libCZI::IDisplaySettings
{
private:
    std::map<int, std::shared_ptr<libCZI::IChannelDisplaySetting>> chDsplSettings;
public:
    explicit CDisplaySettingsWrapper(const CCmdLineOptions& options)
    {
        const auto& cmdLineChDsplSettingsMap = options.GetMultiChannelCompositeChannelInfos();
        for (std::map<int, ChannelDisplaySettings>::const_iterator it = cmdLineChDsplSettingsMap.cbegin(); it != cmdLineChDsplSettingsMap.cend(); ++it)
        {
            this->chDsplSettings[it->first] = std::make_shared<ChannelDisplaySettingsWrapper>(it->second);
        }
    }

    void EnumChannels(std::function<bool(int)> func) const override
    {
        for (std::map<int, std::shared_ptr<libCZI::IChannelDisplaySetting>>::const_iterator it = this->chDsplSettings.cbegin();
            it != this->chDsplSettings.cend(); ++it)
        {
            func(it->first);
        }
    }

    std::shared_ptr<libCZI::IChannelDisplaySetting> GetChannelDisplaySettings(int chIndex) const override
    {
        return this->chDsplSettings.at(chIndex);
    }
};
