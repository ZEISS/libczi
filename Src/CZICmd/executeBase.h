// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <memory>
#include <functional>
#include <map>
#include "executeBase.h"
#include "inc_libCZI.h"

class CCmdLineOptions;

class CExecuteBase
{
protected:
    static std::shared_ptr<libCZI::ICZIReader> CreateAndOpenCziReader(const CCmdLineOptions& options);
    static std::shared_ptr<libCZI::IStream> CreateStandardFileBasedStreamObject(const wchar_t* fileName);
    static std::shared_ptr<libCZI::IStream> CreateInputStreamObject(const wchar_t* uri, const std::string& class_name, const std::map<int, libCZI::StreamsFactory::Property>* property_bag);
    static libCZI::IntRect GetRoiFromOptions(const CCmdLineOptions& options, const libCZI::SubBlockStatistics& subBlockStatistics);
    static libCZI::RgbFloatColor GetBackgroundColorFromOptions(const CCmdLineOptions& options);
    static void DoCalcHashOfResult(std::shared_ptr<libCZI::IBitmapData> bm, const CCmdLineOptions& options);
    static void HandleHashOfResult(const std::function<bool(uint8_t*, size_t)>& f, const CCmdLineOptions& options);
    static void DoCalcHashOfResult(libCZI::IBitmapData* bm, const CCmdLineOptions& options);
};
