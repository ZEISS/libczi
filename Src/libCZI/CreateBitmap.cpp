// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "bitmapData.h"
#include "Site.h"
#include "libCZI.h"
#include "BitmapOperations.h"
#include "inc_libCZI_Config.h"
#include "CziSubBlock.h"
#include "decoder_zstd.h"
#if LIBCZI_HAVE_LIBJXL
#include "decoder_jxl.h"
#endif

using namespace libCZI;
using namespace libCZI::detail;

namespace
{
    std::shared_ptr<libCZI::IBitmapData> CreateBitmapFromSubBlockData_JpgXr(
            const void* pv,
            size_t size,
            libCZI::PixelType pixelType,
            std::uint32_t width,
            std::uint32_t height,
            bool handle_jxr_bitmap_mismatch)
    {
        auto dec = GetSite()->GetDecoder(ImageDecoderType::JPXR_JxrLib, nullptr);
        if (!handle_jxr_bitmap_mismatch)
        {
            return dec->Decode(pv, size, pixelType, width, height);
        }
        else
        {
            // This means - according to the "resolution protocol", if there is a mismatch between the bitmap encoded as JpgXR and the
            //  description in the subblock, we have to crop or pad the bitmap to the size described in the subblock.
            auto decoded_bitmap = dec->Decode(pv, size, nullptr, nullptr, nullptr);
            if (decoded_bitmap->GetWidth() == width &&
                decoded_bitmap->GetHeight() == height &&
                decoded_bitmap->GetPixelType() == pixelType)
            {
                return decoded_bitmap;
            }
            else
            {
                // ok, we have a discrepancy between the size of the bitmap and the size described in the subblock, so let's crop or pad the bitmap

                // create a bitmap of the size described in the subblock
                auto adjusted_bitmap = CStdBitmapData::Create(pixelType, width, height);
                CBitmapOperations::Fill(adjusted_bitmap.get(), RgbFloatColor{ 0,0,0 });
                const ScopedBitmapLockerSP adjusted_bitmap_lock{ adjusted_bitmap };
                const ScopedBitmapLockerSP decoded_bitmap_lock{ decoded_bitmap };
                CBitmapOperations::CopyWithOffsetInfo copy_info;
                copy_info.xOffset = 0;
                copy_info.yOffset = 0;
                copy_info.srcPixelType = decoded_bitmap->GetPixelType();
                copy_info.srcPtr = decoded_bitmap_lock.ptrDataRoi;
                copy_info.srcStride = decoded_bitmap_lock.stride;
                copy_info.srcWidth = decoded_bitmap->GetWidth();
                copy_info.srcHeight = decoded_bitmap->GetHeight();
                copy_info.dstPixelType = pixelType;
                copy_info.dstPtr = adjusted_bitmap_lock.ptrDataRoi;
                copy_info.dstStride = adjusted_bitmap_lock.stride;
                copy_info.dstWidth = adjusted_bitmap->GetWidth();
                copy_info.dstHeight = adjusted_bitmap->GetHeight();
                copy_info.drawTileBorder = false;
                CBitmapOperations::CopyWithOffset(copy_info);
                return adjusted_bitmap;
            }
        }
    }

    std::shared_ptr<libCZI::IBitmapData> CreateBitmapFromSubBlock_JpgXr(ISubBlock* subBlk, bool handle_jxr_bitmap_mismatch)
    {
        const void* ptr;
        size_t size;
        subBlk->DangerousGetRawData(ISubBlock::MemBlkType::Data, ptr, size);
        const SubBlockInfo& sub_block_info = subBlk->GetSubBlockInfo();

        return CreateBitmapFromSubBlockData_JpgXr(ptr, size, sub_block_info.pixelType, sub_block_info.physicalSize.w, sub_block_info.physicalSize.h, handle_jxr_bitmap_mismatch);
    }

    std::shared_ptr<libCZI::IBitmapData> CreateBitmapFromSubBlockData_ZStd0(
            const void* pv,
            size_t size,
            libCZI::PixelType pixelType,
            std::uint32_t width,
            std::uint32_t height,
            bool handle_zstd_data_size_mismatch)
    {
        auto dec = GetSite()->GetDecoder(ImageDecoderType::ZStd0, nullptr);
        return dec->Decode(
                        pv,
                        size,
                        pixelType,
                        width,
                        height,
                        handle_zstd_data_size_mismatch ? CZstd0Decoder::kOption_handle_data_size_mismatch : nullptr);
    }

    std::shared_ptr<libCZI::IBitmapData> CreateBitmapFromSubBlock_ZStd0(ISubBlock* subBlk, bool handle_zstd_data_size_mismatch)
    {
        const void* ptr;
        size_t size;
        subBlk->DangerousGetRawData(ISubBlock::MemBlkType::Data, ptr, size);
        return CreateBitmapFromSubBlockData_ZStd0(ptr, size, subBlk->GetSubBlockInfo().pixelType, subBlk->GetSubBlockInfo().physicalSize.w, subBlk->GetSubBlockInfo().physicalSize.h, handle_zstd_data_size_mismatch);
    }

    std::shared_ptr<libCZI::IBitmapData> CreateBitmapFromSubBlockData_ZStd1(
            const void* pv,
            size_t size,
            libCZI::PixelType pixelType,
            std::uint32_t width,
            std::uint32_t height,
            bool handle_zstd_data_size_mismatch)
    {
        auto dec = GetSite()->GetDecoder(ImageDecoderType::ZStd1, nullptr);
        return dec->Decode(
                        pv,
                        size,
                        pixelType,
                        width,
                        height,
                        handle_zstd_data_size_mismatch ? CZstd1Decoder::kOption_handle_data_size_mismatch : nullptr);
    }

    std::shared_ptr<libCZI::IBitmapData> CreateBitmapFromSubBlock_ZStd1(ISubBlock* subBlk, bool handle_zstd_data_size_mismatch)
    {
        const void* ptr;
        size_t size;
        subBlk->DangerousGetRawData(ISubBlock::MemBlkType::Data, ptr, size);
        return CreateBitmapFromSubBlockData_ZStd1(ptr, size, subBlk->GetSubBlockInfo().pixelType, subBlk->GetSubBlockInfo().physicalSize.w, subBlk->GetSubBlockInfo().physicalSize.h, handle_zstd_data_size_mismatch);
    }

    std::shared_ptr<libCZI::IBitmapData> CreateBitmapFromSubBlockData_Uncompressed(
                                            const void* pv,
                                            size_t size,
                                            libCZI::PixelType pixelType,
                                            std::uint32_t width,
                                            std::uint32_t height,
                                            bool handle_uncompressed_data_size_mismatch)
    {
        // The stride with an uncompressed bitmap in CZI is exactly the line-size.
        const std::uint32_t source_stride = width * CziUtils::GetBytesPerPel(pixelType);
        const size_t expected_size = static_cast<size_t>(source_stride) * height;

        if (expected_size <= size)
        {
            auto bitmap = CStdBitmapData::Create(pixelType, width, height);
            ScopedBitmapLockerSP locked_bitmap{ bitmap };
#if LIBCZI_ISBIGENDIANHOST
            if (CziUtils::IsPixelTypeEndianessAgnostic(pixelType))
            {
                CBitmapOperations::Copy(pixelType, pv, source_stride, pixelType, locked_bitmap.ptrDataRoi, locked_bitmap.stride, width, height, false);
            }
            else
            {
                CBitmapOperations::CopyConvertBigEndian(pixelType, pv, source_stride, locked_bitmap.ptrDataRoi, locked_bitmap.stride, width, height);
            }
#else
            CBitmapOperations::Copy(pixelType, pv, source_stride, pixelType, locked_bitmap.ptrDataRoi, locked_bitmap.stride, width, height, false);
#endif
            return bitmap;
        }
        else
        {
            if (!handle_uncompressed_data_size_mismatch)
            {
                throw std::logic_error("insufficient size of subblock");
            }

            // ok - according to the "resolution protocol" the bitmap is to be filled with zeroes
            auto bitmap = CStdBitmapData::Create(pixelType, width, height);
            auto lock = bitmap->Lock();
            size_t remaining_size = size;
            for (uint32_t y = 0; y < height; ++y)
            {
                uint8_t* destination = static_cast<uint8_t*>(lock.ptrDataRoi) + y * static_cast<size_t>(lock.stride);
                if (remaining_size > 0)
                {
                    const uint8_t* source = static_cast<const uint8_t*>(pv) + y * static_cast<size_t>(source_stride);
                    size_t copy_size = std::min(remaining_size, static_cast<size_t>(source_stride));
                    memcpy(destination, source, copy_size);
                    remaining_size -= copy_size;
                    if (remaining_size == 0)
                    {
                        std::memset(destination + copy_size, 0, static_cast<size_t>(source_stride) - copy_size);
                    }
                }
                else
                {
                    std::memset(destination, 0, source_stride);
                }
            }

            bitmap->Unlock();

#if LIBCZI_ISBIGENDIANHOST
            if (!CziUtils::IsPixelTypeEndianessAgnostic(pixelType))
            {
                return CBitmapOperations::ConvertToBigEndian(bitmap.get());
            }
#endif

            return bitmap;
        }
    }

    std::shared_ptr<libCZI::IBitmapData> CreateBitmapFromSubBlock_Uncompressed(ISubBlock* subBlk, bool handle_uncompressed_data_size_mismatch)
    {
        const auto& sub_block_info = subBlk->GetSubBlockInfo();

        // The stride with an uncompressed bitmap in CZI is exactly the line-size.
        const std::uint32_t stride = sub_block_info.physicalSize.w * CziUtils::GetBytesPerPel(sub_block_info.pixelType);
        const size_t expected_size = static_cast<size_t>(stride) * sub_block_info.physicalSize.h;

        size_t size;
        auto sub_block_data = subBlk->GetRawData(ISubBlock::MemBlkType::Data, &size);

        if (expected_size <= size
#if LIBCZI_ISBIGENDIANHOST
            && CziUtils::IsPixelTypeEndianessAgnostic(subBlk->GetSubBlockInfo().pixelType)
#endif
            )
        {
            // only in this case (data is >= expected size, and on a big-endian host if the pixel type is endianness-agnostic)
            // we can directly use the data as bitmap data without copying or conversion
            CSharedPtrAllocator sharedPtrAllocator(sub_block_data);
            auto sb = CBitmapData<CSharedPtrAllocator>::Create(
                                                            sharedPtrAllocator,
                                                            sub_block_info.pixelType,
                                                            sub_block_info.physicalSize.w,
                                                            sub_block_info.physicalSize.h,
                                                            stride);
            return sb;
        }
        else
        {
            return CreateBitmapFromSubBlockData_Uncompressed(
                sub_block_data.get(),
                size,
                sub_block_info.pixelType,
                sub_block_info.physicalSize.w,
                sub_block_info.physicalSize.h,
                handle_uncompressed_data_size_mismatch);
        }
    }
}

#if LIBCZI_HAVE_LIBJXL
static std::shared_ptr<libCZI::IBitmapData> CreateBitmapFromSubBlock_Jxl(ISubBlock* subBlk)
{
    auto dec = GetSite()->GetDecoder(ImageDecoderType::JXL, nullptr);
    const void* ptr;
    size_t size;
    subBlk->DangerousGetRawData(ISubBlock::MemBlkType::Data, ptr, size);
    const SubBlockInfo& sub_block_info = subBlk->GetSubBlockInfo();
    return dec->Decode(ptr, size, sub_block_info.pixelType, sub_block_info.physicalSize.w, sub_block_info.physicalSize.h);
}
#endif

std::shared_ptr<libCZI::IBitmapData> libCZI::CreateBitmapFromSubBlock(ISubBlock* subBlk, const CreateBitmapOptions* options)
{
    switch (subBlk->GetSubBlockInfo().GetCompressionMode())
    {
    case CompressionMode::JpgXr:
        return CreateBitmapFromSubBlock_JpgXr(subBlk, options != nullptr ? options->handle_jpgxr_bitmap_mismatch : true);
    case CompressionMode::Zstd0:
        return CreateBitmapFromSubBlock_ZStd0(subBlk, options != nullptr ? options->handle_zstd_data_size_mismatch : true);
    case CompressionMode::Zstd1:
        return CreateBitmapFromSubBlock_ZStd1(subBlk, options != nullptr ? options->handle_zstd_data_size_mismatch : true);
    case CompressionMode::UnCompressed:
        return CreateBitmapFromSubBlock_Uncompressed(subBlk, options != nullptr ? options->handle_uncompressed_data_size_mismatch : true);
#if LIBCZI_HAVE_LIBJXL
    case CompressionMode::Jxl:
        return CreateBitmapFromSubBlock_Jxl(subBlk);
#endif
    default:    // silence warnings
        throw std::logic_error("The method or operation is not implemented.");
    }
}

std::shared_ptr<libCZI::IBitmapData> libCZI::CreateBitmapFromSubBlockData(
        libCZI::CompressionMode compression_mode,
        const void* pv,
        size_t size,
        libCZI::PixelType pixelType,
        std::uint32_t width,
        std::uint32_t height,
        const CreateBitmapOptions* options)
{
    if (pv == nullptr)
    {
        throw std::invalid_argument("The input data pointer is null.");
    }

    switch (compression_mode)
    {
    case CompressionMode::JpgXr:
        return CreateBitmapFromSubBlockData_JpgXr(pv, size, pixelType, width, height, options != nullptr ? options->handle_jpgxr_bitmap_mismatch : true);
    case CompressionMode::Zstd0:
        return CreateBitmapFromSubBlockData_ZStd0(pv, size, pixelType, width, height, options != nullptr ? options->handle_zstd_data_size_mismatch : true);
    case CompressionMode::Zstd1:
        return CreateBitmapFromSubBlockData_ZStd1(pv, size, pixelType, width, height, options != nullptr ? options->handle_zstd_data_size_mismatch : true);
    case CompressionMode::UnCompressed:
        return CreateBitmapFromSubBlockData_Uncompressed(pv, size, pixelType, width, height, options != nullptr ? options->handle_uncompressed_data_size_mismatch : true);
    default:
        throw std::logic_error("The specified compression mode is not supported or implemented.");
    }
}
