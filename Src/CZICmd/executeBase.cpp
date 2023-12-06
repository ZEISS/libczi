// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "executeBase.h"

#include "cmdlineoptions.h"

using namespace std;
using namespace libCZI;

std::shared_ptr<ICZIReader> CExecuteBase::CreateAndOpenCziReader(const CCmdLineOptions& options)
{
    shared_ptr<IStream> stream;
    if (options.GetInputStreamClassName().empty())
    {
        stream = CExecuteBase::CreateStandardFileBasedStreamObject(options.GetCZIFilename().c_str());
    }
    else
    {
        stream = CExecuteBase::CreateInputStreamObject(
                                options.GetCZIFilename().c_str(),
                                options.GetInputStreamClassName(),
                                &options.GetInputStreamPropertyBag());
    }

    auto spReader = libCZI::CreateCZIReader();
    spReader->Open(stream);
    return spReader;
}

std::shared_ptr<IStream> CExecuteBase::CreateStandardFileBasedStreamObject(const wchar_t* fileName)
{
    auto stream = libCZI::CreateStreamFromFile(fileName);
    return stream;
}

std::shared_ptr<IStream> CExecuteBase::CreateInputStreamObject(const wchar_t* uri, const string& class_name, const std::map<int, libCZI::StreamsFactory::Property>* property_bag)
{
    libCZI::StreamsFactory::Initialize();
    libCZI::StreamsFactory::CreateStreamInfo stream_info;
    stream_info.class_name = class_name;
    if (property_bag != nullptr)
    {
        stream_info.property_bag = *property_bag;
    }

    auto stream = libCZI::StreamsFactory::CreateStream(stream_info, uri);
    if (!stream)
    {
        stringstream string_stream;
        string_stream << "Failed to create stream object of the class \"" << class_name << "\".";
        throw std::runtime_error(string_stream.str());
    }

    return stream;
}

IntRect CExecuteBase::GetRoiFromOptions(const CCmdLineOptions& options, const SubBlockStatistics& subBlockStatistics)
{
    IntRect roi{ options.GetRectX(), options.GetRectY(), options.GetRectW(), options.GetRectH() };
    if (options.GetIsRelativeRectCoordinate())
    {
        roi.x += subBlockStatistics.boundingBox.x;
        roi.y += subBlockStatistics.boundingBox.y;
    }

    return roi;
}

libCZI::RgbFloatColor CExecuteBase::GetBackgroundColorFromOptions(const CCmdLineOptions& options)
{
    return options.GetBackGroundColor();
}

void CExecuteBase::DoCalcHashOfResult(shared_ptr<libCZI::IBitmapData> bm, const CCmdLineOptions& options)
{
    DoCalcHashOfResult(bm.get(), options);
}

void CExecuteBase::HandleHashOfResult(const std::function<bool(uint8_t*, size_t)>& f, const CCmdLineOptions& options)
{
    if (!options.GetCalcHashOfResult())
    {
        return;
    }

    uint8_t md5sumHash[16];
    if (!f(md5sumHash, sizeof(md5sumHash)))
    {
        return;
    }

    string hashHex = BytesToHexString(md5sumHash, sizeof(md5sumHash));
    std::stringstream ss;
    ss << "hash of result: " << hashHex;
    auto log = options.GetLog();
    log->WriteLineStdOut(ss.str().c_str());
}

void CExecuteBase::DoCalcHashOfResult(libCZI::IBitmapData* bm, const CCmdLineOptions& options)
{
    HandleHashOfResult(
        [&](uint8_t* ptrHash, size_t size)->bool
        {
        Utils::CalcMd5SumHash(bm, ptrHash, (int)size);
        return true;
        },
        options);
}
