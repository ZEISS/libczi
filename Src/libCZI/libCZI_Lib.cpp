// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "libCZI.h"
#include "CZIReader.h"
#include "CziMetadata.h"
#include "SingleChannelTileAccessor.h"
#include "SingleChannelPyramidLevelTileAccessor.h"
#include "SingleChannelScalingTileAccessor.h"
#include "StreamImpl.h"
#include "CziWriter.h"
#include "CziReaderWriter.h"
#include "CziMetadataBuilder.h"
#include "SubblockMetadata.h"
#include "inc_libCZI_Config.h"
#include "SubblockAttachmentAccessor.h"

using namespace libCZI;
using namespace std;

void libCZI::GetLibCZIVersion(int* pMajor, int* pMinor/*nullptr*/, int* pPatch/*nullptr*/, int* pTweak/*nullptr*/)
{
    if (pMajor != nullptr)
    {
        *pMajor = atoi(LIBCZI_VERSION_MAJOR);
    }

    if (pMinor != nullptr)
    {
        *pMinor = atoi(LIBCZI_VERSION_MINOR);
    }

    if (pPatch != nullptr)
    {
        *pPatch = atoi(LIBCZI_VERSION_PATCH);
    }

    if (pTweak != nullptr)
    {
        *pTweak = atoi(LIBCZI_VERSION_TWEAK);
    }
}

void libCZI::GetLibCZIBuildInformation(BuildInformation& info)
{
    info.compilerIdentification = LIBCZI_CXX_COMPILER_IDENTIFICATION;
    info.repositoryUrl = LIBCZI_REPOSITORYREMOTEURL;
    info.repositoryBranch = LIBCZI_REPOSITORYBRANCH;
    info.repositoryTag = LIBCZI_REPOSITORYHASH;
}

std::shared_ptr<ICZIReader> libCZI::CreateCZIReader()
{
    return std::make_shared<CCZIReader>();
}

std::shared_ptr<ICziWriter> libCZI::CreateCZIWriter(const CZIWriterOptions* options)
{
    if (options == nullptr)
    {
        return std::make_shared<CCziWriter>();
    }

    return std::make_shared<CCziWriter>(*options);
}

std::shared_ptr<ICziReaderWriter> libCZI::CreateCZIReaderWriter()
{
    return std::make_shared<CCziReaderWriter>();
}

std::shared_ptr<libCZI::ICziMetadata> libCZI::CreateMetaFromMetadataSegment(IMetadataSegment* metadataSegment)
{
    return std::make_shared<CCziMetadata>(metadataSegment);
}

std::shared_ptr<IAccessor> libCZI::CreateAccesor(std::shared_ptr<ISubBlockRepository> repository, AccessorType accessorType)
{
    switch (accessorType)
    {
    case AccessorType::SingleChannelTileAccessor:
        return std::make_shared<CSingleChannelTileAccessor>(repository);
    case AccessorType::SingleChannelPyramidLayerTileAccessor:
        return std::make_shared<CSingleChannelPyramidLevelTileAccessor>(repository);
    case AccessorType::SingleChannelScalingTileAccessor:
        return std::make_shared<CSingleChannelScalingTileAccessor>(repository);
    }

    throw std::invalid_argument("unknown accessorType");
}

std::shared_ptr<libCZI::IStream> libCZI::CreateStreamFromFile(const wchar_t* szFilename)
{
    return StreamsFactory::CreateDefaultStreamForFile(szFilename);
}

std::shared_ptr<libCZI::IStream> libCZI::CreateStreamFromMemory(std::shared_ptr<const void> ptr, size_t dataSize)
{
    return make_shared<CStreamImplInMemory>(ptr, dataSize);
}

std::shared_ptr<libCZI::IStream> libCZI::CreateStreamFromMemory(libCZI::IAttachment* attachment)
{
    return make_shared<CStreamImplInMemory>(attachment);
}

std::shared_ptr<IOutputStream> libCZI::CreateOutputStreamForFile(const wchar_t* szwFilename, bool overwriteExisting)
{
#if LIBCZI_WINDOWSAPI_AVAILABLE
    return make_shared<CSimpleOutputStreamImplWindows>(szwFilename, overwriteExisting);
#else
#if LIBCZI_USE_PREADPWRITEBASED_STREAMIMPL
    return make_shared<COutputStreamImplPwrite>(szwFilename, overwriteExisting);
#else
    return make_shared<CSimpleOutputStreamStreams>(szwFilename, overwriteExisting);
#endif
#endif
}

std::shared_ptr<IOutputStream> libCZI::CreateOutputStreamForFileUtf8(const char* szFilename, bool overwriteExisting)
{
    wstring filename_wide = Utilities::convertUtf8ToWchar_t(szFilename);
    return libCZI::CreateOutputStreamForFile(filename_wide.c_str(), overwriteExisting);
}

std::shared_ptr<IInputOutputStream> libCZI::CreateInputOutputStreamForFile(const wchar_t* szFilename)
{
#if LIBCZI_WINDOWSAPI_AVAILABLE
    return make_shared<CSimpleInputOutputStreamImplWindows>(szFilename);
#else
#if LIBCZI_USE_PREADPWRITEBASED_STREAMIMPL
    return make_shared<CInputOutputStreamImplPreadPwrite>(szFilename);
#else
    return make_shared<CSimpleInputOutputStreamImpl>(szFilename);
#endif
#endif
}

std::shared_ptr<ICziMetadataBuilder> libCZI::CreateMetadataBuilder()
{
    return make_shared<CCZiMetadataBuilder>(L"ImageDocument");
}

std::shared_ptr<ICziMetadataBuilder> libCZI::CreateMetadataBuilderFromXml(const std::string& xml)
{
    return make_shared<CCZiMetadataBuilder>(L"ImageDocument", xml);
}

std::shared_ptr<ISubBlockMetadata> libCZI::CreateSubBlockMetadataFromSubBlock(const libCZI::ISubBlock* sub_block)
{
    if (sub_block == nullptr)
    {
        throw std::invalid_argument("sub_block must not be null");
    }

    size_t size_metadata;
    auto raw_data = sub_block->GetRawData(libCZI::ISubBlock::MemBlkType::Metadata, &size_metadata);
    auto sub_block_metadata = make_shared<SubblockMetadata>((const char*)raw_data.get(), size_metadata);
    return sub_block_metadata;
}

LIBCZI_API std::shared_ptr<ISubBlockAttachmentAccessor> libCZI::CreateSubBlockAttachmentAccessor(const std::shared_ptr<libCZI::ISubBlock>& sub_block, const std::shared_ptr<ISubBlockMetadata>& sub_block_metadata)
{
    if (sub_block == nullptr)
    {
        throw std::invalid_argument("sub_block must not be null");
    }

    if (sub_block_metadata == nullptr)
    {
        return std::make_shared<SubblockAttachmentAccessor>(sub_block, libCZI::CreateSubBlockMetadataFromSubBlock(sub_block.get()));
    }
    else
    {
        return std::make_shared<SubblockAttachmentAccessor>(sub_block, sub_block_metadata);
    }
}
