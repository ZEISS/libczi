// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <chrono>

#include "bitmapData.h"
#include "Site.h"
#include "libCZI.h"
#include "BitmapOperations.h"
#include "inc_libCZI_Config.h"
#include "CziSubBlock.h"

using namespace libCZI;

static std::shared_ptr<libCZI::IBitmapData> CreateBitmapFromSubBlock_JpgXr(ISubBlock* subBlk)
{
    auto dec = GetSite()->GetDecoder(ImageDecoderType::JPXR_JxrLib, nullptr);
    const void* ptr; size_t size;
    subBlk->DangerousGetRawData(ISubBlock::MemBlkType::Data, ptr, size);
    SubBlockInfo subBlockInfo = subBlk->GetSubBlockInfo();
    return dec->Decode(ptr, size, subBlockInfo.pixelType, subBlockInfo.physicalSize.w, subBlockInfo.physicalSize.h);
}

static std::shared_ptr<libCZI::IBitmapData> CreateBitmapFromSubBlock_ZStd0(ISubBlock* subBlk)
{
    auto dec = GetSite()->GetDecoder(ImageDecoderType::ZStd0, nullptr);
    const void* ptr; size_t size;
    subBlk->DangerousGetRawData(ISubBlock::MemBlkType::Data, ptr, size);
    SubBlockInfo subBlockInfo = subBlk->GetSubBlockInfo();
    return dec->Decode(ptr, size, subBlockInfo.pixelType, subBlockInfo.physicalSize.w, subBlockInfo.physicalSize.h);
}

static std::shared_ptr<libCZI::IBitmapData> CreateBitmapFromSubBlock_ZStd1(ISubBlock* subBlk)
{
    auto dec = GetSite()->GetDecoder(ImageDecoderType::ZStd1, nullptr);
    const void* ptr; size_t size;
    subBlk->DangerousGetRawData(ISubBlock::MemBlkType::Data, ptr, size);
    SubBlockInfo subBlockInfo = subBlk->GetSubBlockInfo();

    return dec->Decode(ptr, size, subBlockInfo.pixelType, subBlockInfo.physicalSize.w, subBlockInfo.physicalSize.h);
}

static std::shared_ptr<libCZI::IBitmapData> CreateBitmapFromSubBlock_Uncompressed(ISubBlock* subBlk)
{
    size_t size;
    CSharedPtrAllocator sharedPtrAllocator(subBlk->GetRawData(ISubBlock::MemBlkType::Data, &size));

    const auto& sbBlkInfo = subBlk->GetSubBlockInfo();

    // TODO: How exactly shoud the stride be derived? It seems that stride must be exactly linesize.
    const std::uint32_t stride = sbBlkInfo.physicalSize.w * CziUtils::GetBytesPerPel(sbBlkInfo.pixelType);
    if (static_cast<size_t>(stride) * sbBlkInfo.physicalSize.h > size)
    {
        throw std::logic_error("insufficient size of subblock");
    }

    auto sb = CBitmapData<CSharedPtrAllocator>::Create(
        sharedPtrAllocator,
        sbBlkInfo.pixelType,
        sbBlkInfo.physicalSize.w,
        sbBlkInfo.physicalSize.h,
        stride);

#if LIBCZI_ISBIGENDIANHOST
    if (!CziUtils::IsPixelTypeEndianessAgnostic(subBlk->GetSubBlockInfo().pixelType))
    {
        return CBitmapOperations::ConvertToBigEndian(sb.get());
    }
#endif

    return sb;
}

std::shared_ptr<libCZI::IBitmapData> libCZI::CreateBitmapFromSubBlock(ISubBlock* subBlk)
{
    //auto start = std::chrono::high_resolution_clock::now();
    std::shared_ptr<libCZI::IBitmapData> bitmapData;
    switch (subBlk->GetSubBlockInfo().GetCompressionMode())
    {
    case CompressionMode::JpgXr:
        bitmapData = CreateBitmapFromSubBlock_JpgXr(subBlk);
        break;
    case CompressionMode::Zstd0:
        bitmapData = CreateBitmapFromSubBlock_ZStd0(subBlk);
        break;
    case CompressionMode::Zstd1:
        bitmapData = CreateBitmapFromSubBlock_ZStd1(subBlk);
        break;
    case CompressionMode::UnCompressed:
        bitmapData = CreateBitmapFromSubBlock_Uncompressed(subBlk);
        break;
    default:    // silence warnings
        throw std::logic_error("The method or operation is not implemented.");
    }
   /* auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    if (GetSite()->IsEnabled(LOGLEVEL_CHATTYINFORMATION))
    {
        IntSize extent = bitmapData->GetSize();
        int64_t data_size = static_cast<int64_t>(extent.w) * extent.h * CziUtils::GetBytesPerPel(bitmapData->GetPixelType());
        std::stringstream ss;
        double elapsed_time = elapsed_seconds.count();
        ss << "CreateBitmap: " << elapsed_time * 1000.0 << "ms, size: " << data_size / 1e6 << "MB -> " << (data_size / elapsed_time) / 1e6 << "MB/s";
        GetSite()->Log(LOGLEVEL_CHATTYINFORMATION, ss);
    }*/

    return bitmapData;
}
