// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <map>
#include <memory>
#include "libCZI.h"
#include "CziMetadata.h"

class CCziMetadataDocumentInfo : public libCZI::ICziMultiDimensionDocumentInfo
{
private:
    std::shared_ptr<CCziMetadata>   metadata;
    std::map < libCZI::DimensionIndex, std::shared_ptr<libCZI::IDimensionInfo>> dimensions;
public:
    explicit CCziMetadataDocumentInfo(std::shared_ptr<CCziMetadata> md);

public: // interface ICziMultiDimensionDocumentInfo
    libCZI::GeneralDocumentInfo GetGeneralDocumentInfo() const override;
    libCZI::ScalingInfoEx GetScalingInfoEx() const override;
    libCZI::ScalingInfo GetScalingInfo() const override;
    void EnumDimensions(const std::function<bool(libCZI::DimensionIndex)>& enumDimensions) override;
    std::shared_ptr<libCZI::IDimensionInfo> GetDimensionInfo(libCZI::DimensionIndex dim) override;
    std::shared_ptr<libCZI::IDimensionZInfo> GetDimensionZInfo() override;
    std::shared_ptr<libCZI::IDimensionTInfo> GetDimensionTInfo() override;
    std::shared_ptr<libCZI::IDimensionsChannelsInfo> GetDimensionChannelsInfo() override;
    std::shared_ptr<libCZI::IDisplaySettings> GetDisplaySettings() const override;

private:
    void ParseDimensionInfo();

private:
    pugi::xml_node GetNode(const wchar_t* path) const;
    static pugi::xml_node GetNodeRelativeFromNode(pugi::xml_node node, const wchar_t* path);
};
