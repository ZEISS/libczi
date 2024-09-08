// SPDX-FileCopyrightText: 2024 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once
#include <libCZI_Config.h>

#if LIBCZI_AZURESDK_BASED_STREAM_AVAILABLE

#include <memory>
#include <string>

#include "../libCZI.h"
#include <azure/storage/blobs.hpp>

class AzureBlobInputStream : public libCZI::IStream
{
private:
    std::unique_ptr<Azure::Storage::Blobs::BlobServiceClient> serviceClient_;
    std::unique_ptr<Azure::Storage::Blobs::BlockBlobClient> blockBlobClient_; 
public:
    AzureBlobInputStream(const std::string& url, const std::map<int, libCZI::StreamsFactory::Property>& property_bag);

    void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;

    ~AzureBlobInputStream() override;


    static std::string GetBuildInformation();
    static libCZI::StreamsFactory::Property GetClassProperty(const char* property_name);

private:
    // https ://learn.microsoft.com/en-us/azure/storage/blobs/quickstart-blobs-c-plus-plus?tabs=managed-identity%2Croles-azure-portal#authenticate-to-azure-and-authorize-access-to-blob-data
};

#endif
