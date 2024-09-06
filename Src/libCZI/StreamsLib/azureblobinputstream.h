// SPDX-FileCopyrightText: 2024 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once
#include <libCZI_Config.h>

#if LIBCZI_AZURESDK_BASED_STREAM_AVAILABLE

#include "../libCZI.h"
#include <azure/storage/blobs.hpp>

class AzureBlobInputStream : public libCZI::IStream
{
private:
    Azure::Storage::Blobs::BlobServiceClient serviceClient_;
public:
    AzureBlobInputStream(const std::string& url, const std::map<int, libCZI::StreamsFactory::Property>& property_bag);

    void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;

    ~AzureBlobInputStream() override;


    static std::string GetBuildInformation();
    static libCZI::StreamsFactory::Property GetClassProperty(const char* property_name);
};

#endif
