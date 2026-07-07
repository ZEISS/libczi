// SPDX-FileCopyrightText: 2026 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "libCZI_Config_Internal.h"

#if LIBCZI_EXPERIMENTAL_CHUNKED_COMPRESSION_AVAILABLE
#include <memory>

#include "libCZI_compress.h"
#include "libCZI_Pixels.h"
#include "libCZI_Site.h"

namespace libCZI
{
    namespace detail
    {
        /// Implementation of the decoder for chunked compression. Note that this is not a "real" compression scheme, but rather a "meta-scheme"
        /// which allows to encode some information about the compressed data in a header, and then apply a "real" compression scheme to the data.
        class CChunkedCompressionDecoder : public libCZI::IDecoder
        {
        public:
            static std::shared_ptr<CChunkedCompressionDecoder> Create();

            /// Passing in a block of chunked-compressed data, decode the image and return a bitmap object.
            /// This decoder requires that pixelType, width and height are passed in; the parameters must not be nullptr.
            ///
            /// Supported tokens in additional_arguments:
            /// - "IgnorePreprocessingInstruction": ignore the hi-lo byte packing preprocessing flag in the chunked-compression header.
            /// - "handle_data_size_mismatch": allow decompressed data size to differ from the size calculated from pixelType, width and height.
            ///
            /// \param ptrData              Pointer to a block of memory containing the chunked-compressed data.
            /// \param size                 The size of the memory block pointed to by ptrData.
            /// \param pixelType            The pixel type of the expected bitmap. Must not be nullptr.
            /// \param width                The width of the expected bitmap. Must not be nullptr.
            /// \param height               The height of the expected bitmap. Must not be nullptr.
            /// \param additional_arguments Optional decoder arguments as a token list. May be nullptr.
            ///
            /// \return A bitmap object with the decoded data.
            std::shared_ptr<libCZI::IBitmapData> Decode(const void* ptrData, size_t size, const libCZI::PixelType* pixelType, const std::uint32_t* width, const std::uint32_t* height, const char* additional_arguments) override;

        private:
            struct DecodeInformation
            {
                libCZI::PixelType pixelType;
                std::uint32_t width;
                std::uint32_t height;
                const void* ptr_subblock_data;
                size_t size_subblock_data;
                std::tuple<size_t, ChunkedCompressionHeaderHelper::HeaderInfo> chunk_header_info;
            };

            static std::shared_ptr<libCZI::IBitmapData> DecodeSizeMatchesExactly(const DecodeInformation& decode_information);
            static std::shared_ptr<libCZI::IBitmapData> DecodeSizeMatchesHandleSizeMismatch(const DecodeInformation& decode_information, size_t total_size_of_decompressed_data);

            static void DecompressNoPreprocessing(const DecodeInformation& decode_information, uint64_t source_offset, void* destination, size_t size_destination);
            static void DecompressLoHiBytePackingPreprocessing(const DecodeInformation& decode_information, uint64_t source_offset, void* destination, size_t size_destination);
        public:
            static const char* kOption_IgnorePreprocessingInstruction;
            static const char* kOption_handle_data_size_mismatch;
        };
    }
}
#endif
