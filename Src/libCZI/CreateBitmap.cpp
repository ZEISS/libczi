// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

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

static std::shared_ptr<libCZI::IBitmapData> CreateBitmapFromSubBlock_Uncompressed(ISubBlock* subBlk, bool handle_uncompressed_data_size_mismatch)
{
    const auto& sub_block_info = subBlk->GetSubBlockInfo();

    // The stride with an uncompressed bitmap in CZI is exactly the line-size.
    const std::uint32_t stride = sub_block_info.physicalSize.w * CziUtils::GetBytesPerPel(sub_block_info.pixelType);
    const size_t expected_size = static_cast<size_t>(stride) * sub_block_info.physicalSize.h;

    size_t size;
    auto sub_block_data = subBlk->GetRawData(ISubBlock::MemBlkType::Data, &size);
    //CSharedPtrAllocator sharedPtrAllocator(subBlk->GetRawData(ISubBlock::MemBlkType::Data, &size));

    if (expected_size <= size)
    {
        CSharedPtrAllocator sharedPtrAllocator(sub_block_data);
        auto sb = CBitmapData<CSharedPtrAllocator>::Create(
                                                        sharedPtrAllocator,
                                                        sub_block_info.pixelType,
                                                        sub_block_info.physicalSize.w,
                                                        sub_block_info.physicalSize.h,
                                                        stride);
#if LIBCZI_ISBIGENDIANHOST
        if (!CziUtils::IsPixelTypeEndianessAgnostic(subBlk->GetSubBlockInfo().pixelType))
        {
            return CBitmapOperations::ConvertToBigEndian(sb.get());
        }
#endif

        return sb;
    }
    else
    {
        if (!handle_uncompressed_data_size_mismatch)
        {
            throw std::logic_error("insufficient size of subblock");
        }

        // ok - according to the "resolution protocol" the bitmap is to be filled with zeroes
        auto bitmap = CStdBitmapData::Create(sub_block_info.pixelType, sub_block_info.physicalSize.w, sub_block_info.physicalSize.h);
        auto lock = bitmap->Lock();
        size_t remaining_size = size;
        for (uint32_t y = 0; y < sub_block_info.physicalSize.h; ++y)
        {
            uint8_t* destination = static_cast<uint8_t*>(lock.ptrDataRoi) + y * static_cast<size_t>(lock.stride);
            if (remaining_size > 0)
            {
                const uint8_t* source = static_cast<const uint8_t*>(sub_block_data.get()) + y * static_cast<size_t>(stride);
                size_t copy_size = std::min(remaining_size, static_cast<size_t>(stride));
                memcpy(destination, source, copy_size);
                remaining_size -= copy_size;
                if (remaining_size == 0)
                {
                    std::memset(destination + copy_size, 0, static_cast<size_t>(stride) - copy_size);
                }
            }
            else
            {
                std::memset(destination, 0, stride);
            }
        }

        bitmap->Unlock();

#if LIBCZI_ISBIGENDIANHOST
        if (!CziUtils::IsPixelTypeEndianessAgnostic(subBlk->GetSubBlockInfo().pixelType))
        {
            return CBitmapOperations::ConvertToBigEndian(bitmap.get());
        }
#endif

        return bitmap;
    }
/*
    if (handle_uncompressed_data_size_mismatch)
    {
        
    }
    else
    {
        size_t size;
        CSharedPtrAllocator sharedPtrAllocator(subBlk->GetRawData(ISubBlock::MemBlkType::Data, &size));

      //  const auto& sbBlkInfo = subBlk->GetSubBlockInfo();

        // The stride with an uncompressed bitmap in CZI is exactly the linesize.
        //const std::uint32_t stride = sbBlkInfo.physicalSize.w * CziUtils::GetBytesPerPel(sbBlkInfo.pixelType);
        //if (static_cast<size_t>(stride) * sbBlkInfo.physicalSize.h > size)
        if (expected_size > size)
        {
            throw std::logic_error("insufficient size of subblock");
        }

        auto sb = CBitmapData<CSharedPtrAllocator>::Create(
            sharedPtrAllocator,
            sub_block_info.pixelType,
            sub_block_info.physicalSize.w,
            sub_block_info.physicalSize.h,
            stride);

#if LIBCZI_ISBIGENDIANHOST
        if (!CziUtils::IsPixelTypeEndianessAgnostic(subBlk->GetSubBlockInfo().pixelType))
        {
            return CBitmapOperations::ConvertToBigEndian(sb.get());
        }
#endif

        return sb;
    }*/
}

std::shared_ptr<libCZI::IBitmapData> libCZI::CreateBitmapFromSubBlock(ISubBlock* subBlk, const CreateBitmapOptions* options)
{
    switch (subBlk->GetSubBlockInfo().GetCompressionMode())
    {
    case CompressionMode::JpgXr:
        return CreateBitmapFromSubBlock_JpgXr(subBlk);
    case CompressionMode::Zstd0:
        return CreateBitmapFromSubBlock_ZStd0(subBlk);
    case CompressionMode::Zstd1:
        return CreateBitmapFromSubBlock_ZStd1(subBlk);
    case CompressionMode::UnCompressed:
        return CreateBitmapFromSubBlock_Uncompressed(subBlk, options != nullptr ? options->handle_uncompressed_data_size_mismatch : true);
    default:    // silence warnings
        throw std::logic_error("The method or operation is not implemented.");
    }
}
