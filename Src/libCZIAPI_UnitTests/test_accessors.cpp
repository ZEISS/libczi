// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include <libCZIApi.h>

#include "utilities.h"
#include "MemoryInputStream.h"
#include "MemoryOutputStream.h"
#include <memory>
#include <tuple>

using namespace std;

namespace
{
    tuple<shared_ptr<void>, size_t> CreateCziWithSingleSubBlockWithMask()
    {
        CziWriterObjectHandle writer = kInvalidObjectHandle;
        libCZI_CreateWriter(&writer, nullptr);

        unique_ptr<MemoryOutputStream> memory_output_stream = make_unique<MemoryOutputStream>(2000);
        OutputStreamObjectHandle output_stream = kInvalidObjectHandle;
        ExternalOutputStreamStructInterop external_stream = {};
        external_stream.opaque_handle1 = reinterpret_cast<std::uintptr_t>(memory_output_stream.get());
        external_stream.write_function = [](std::uintptr_t opaque_handle1, std::uintptr_t opaque_handle2, std::uint64_t offset, const void* pv, std::uint64_t size, std::uint64_t* out_bytes_written, ExternalStreamErrorInfoInterop* error_info) -> std::int32_t
            {
                (void)opaque_handle2;
                (void)error_info;
                auto memStream = reinterpret_cast<MemoryOutputStream*>(opaque_handle1);
                if (memStream == nullptr)
                {
                    return -1;
                }

                memStream->Write(offset, pv, size, out_bytes_written);
                return 0;
            };
        external_stream.close_function = [](std::uintptr_t opaque_handle1, std::uintptr_t opaque_handle2) -> void
            {
                (void)opaque_handle1;
                (void)opaque_handle2;
            };

        libCZI_CreateOutputStreamFromExternal(&external_stream, &output_stream);

        libCZI_WriterCreate(writer, output_stream, "{\"file_guid\" : \"123e4567-e89b-12d3-a456-42661417489b\"}");

        libCZI_ReleaseOutputStream(output_stream);

        static constexpr char sub_block_metadata_xml[] =
            "<METADATA>"
            "<AttachmentSchema>"
            "<DataFormat>CHUNKCONTAINER</DataFormat>"
            "</AttachmentSchema>"
            "</METADATA>";
        constexpr size_t sub_block_metadata_xml_size = sizeof(sub_block_metadata_xml) - 1;

        // that's the attachment data - a chunk container with a single chunk: a 4x4 mask bitmap with a checkerboard pattern
        static const uint8_t sub_block_attachment[] =
        {
            0x67, 0xEA, 0xE3, 0xCB, 0xFC, 0x5B, 0x2B, 0x49, 0xA1, 0x6A, 0xEC, 0xE3, 0x78, 0x03, 0x14, 0x48, // that's the GUID of the 'mask' chunk
            0x14, 0x00, 0x00, 0x00, // the size - 20 bytes of data
            0x04, 0x00, 0x00, 0x00, // the width (4 pixels)
            0x04, 0x00, 0x00, 0x00, // the height (4 pixels)
            0x00, 0x00, 0x00, 0x00, // the representation type (0 -> uncompressed bitonal bitmap)
            0x01, 0x00, 0x00, 0x00, // the stride (1 byte per row)
            0xa0,       // the actual mask data - a 4x4 checkerboard pattern   X_X_
            0x50,       //                                                     _X_X
            0xa0,       //                                                     X_X_
            0x50        //                                                     _X_X
        };

        static const uint8_t pixel_data[16] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };

        AddSubBlockInfoInterop add_sub_block_info = {};
        add_sub_block_info.coordinate.dimensions_valid = kDimensionC;
        add_sub_block_info.coordinate.value[0] = 0; // C=0
        add_sub_block_info.m_index_valid = true;
        add_sub_block_info.m_index = 0;
        add_sub_block_info.x = 0;
        add_sub_block_info.y = 0;
        add_sub_block_info.logical_width = 4;
        add_sub_block_info.logical_height = 4;
        add_sub_block_info.physical_width = 4;
        add_sub_block_info.physical_height = 4;
        add_sub_block_info.pixel_type = 0; // Gray8
        add_sub_block_info.compression_mode_raw = 0; // Uncompressed
        add_sub_block_info.data = pixel_data;
        add_sub_block_info.stride = 4;
        add_sub_block_info.size_data = sizeof(pixel_data);
        add_sub_block_info.size_metadata = sub_block_metadata_xml_size;
        add_sub_block_info.metadata = sub_block_metadata_xml;
        add_sub_block_info.size_attachment = sizeof(sub_block_attachment);
        add_sub_block_info.attachment = sub_block_attachment;

        libCZI_WriterAddSubBlock(writer, &add_sub_block_info);

        libCZI_WriterClose(writer);
        libCZI_ReleaseWriter(writer);

        size_t czi_document_size = 0;
        const shared_ptr<void> czi_document_data = memory_output_stream->GetCopy(&czi_document_size);
        return make_tuple(czi_document_data, czi_document_size);
    }
}

TEST(CZIAPI_Accessors, SingleChannelScalingTileAccessorScenario1)
{
    auto czi_data = CreateCziWithSingleSubBlockWithMask();
    ASSERT_TRUE(true);

    auto memory_input_stream_handler_object = new MemoryInputStream(get<0>(czi_data).get(), get<1>(czi_data));

    int input_stream_release_call_count = 0;
    ExternalInputStreamStructInterop external_input_stream_struct;
    external_input_stream_struct.opaque_handle1 = reinterpret_cast<uintptr_t>(memory_input_stream_handler_object);
    external_input_stream_struct.opaque_handle2 = reinterpret_cast<uintptr_t>(&input_stream_release_call_count);
    external_input_stream_struct.read_function = [](uintptr_t opaque_handle1, uintptr_t opaque_handle2, uint64_t offset, void* pv, uint64_t size, uint64_t* ptrBytesRead, ExternalStreamErrorInfoInterop* error_info) -> int32_t
        {
            (void)opaque_handle2;
            auto memory_input_stream_handler = reinterpret_cast<MemoryInputStream*>(opaque_handle1);
            return memory_input_stream_handler->Read(offset, pv, size, ptrBytesRead, error_info);
        };
    external_input_stream_struct.close_function = [](uintptr_t opaque_handle1, uintptr_t opaque_handle2)->void
        {
            auto memory_input_stream_handler = reinterpret_cast<MemoryInputStream*>(opaque_handle1);
            delete memory_input_stream_handler;
            auto input_stream_release_call_count = reinterpret_cast<int*>(opaque_handle2);
            ++(*input_stream_release_call_count);
        };

    InputStreamObjectHandle stream_object;
    LibCZIApiErrorCode error_code = libCZI_CreateInputStreamFromExternal(&external_input_stream_struct, &stream_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    CziReaderObjectHandle reader_object;
    error_code = libCZI_CreateReader(&reader_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    ReaderOpenInfoInterop reader_open_info;
    reader_open_info.streamObject = stream_object;
    error_code = libCZI_ReaderOpen(reader_object, &reader_open_info);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    error_code = libCZI_ReleaseInputStream(stream_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    SingleChannelScalingTileAccessorObjectHandle accessor_object;
    error_code = libCZI_CreateSingleChannelTileAccessor(reader_object, &accessor_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    CoordinateInterop coordinate = {};
    coordinate.dimensions_valid = kDimensionC;
    coordinate.value[0] = 0; // C=0
    IntRectInterop roi = {};
    roi.x = 0;
    roi.y = 0;
    roi.w = 4;
    roi.h = 4;

    BitmapObjectHandle bitmap_object = kInvalidObjectHandle;
    error_code = libCZI_SingleChannelTileAccessorGet(accessor_object, &coordinate, &roi, 1.0f, nullptr, &bitmap_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    BitmapLockInfoInterop lock_info = {};
    error_code = libCZI_BitmapLock(bitmap_object, &lock_info);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    static const uint8_t expected_result_wo_mask[16] = { 1, 2, 3, 4 , 5, 6, 7, 8, 9, 10, 11, 12,13, 14, 15, 16 };
    const uint8_t* composition_line = static_cast<const uint8_t*>(lock_info.ptrDataRoi);
    ASSERT_EQ(memcmp(composition_line, expected_result_wo_mask, 4), 0);
    composition_line += lock_info.stride;
    ASSERT_EQ(memcmp(composition_line, expected_result_wo_mask + 4, 4), 0);
    composition_line += lock_info.stride;
    ASSERT_EQ(memcmp(composition_line, expected_result_wo_mask + 8, 4), 0);
    composition_line += lock_info.stride;
    ASSERT_EQ(memcmp(composition_line, expected_result_wo_mask + 12, 4), 0);

    error_code = libCZI_BitmapUnlock(bitmap_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    error_code = libCZI_ReleaseBitmap(bitmap_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    AccessorOptionsInterop accessor_options = {};
    accessor_options.additional_parameters = "{\"mask_aware\":true}";
    accessor_options.back_ground_color_r = accessor_options.back_ground_color_b = accessor_options.back_ground_color_g = 0.0f; // black background
    error_code = libCZI_SingleChannelTileAccessorGet(accessor_object, &coordinate, &roi, 1.0f, &accessor_options, &bitmap_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    lock_info = {};
    error_code = libCZI_BitmapLock(bitmap_object, &lock_info);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    static const uint8_t expected_result_w_mask[16] = { 1, 0, 3, 0 , 0, 6, 0, 8, 9, 0, 11, 0,0, 14, 0, 16 };
    composition_line = static_cast<const uint8_t*>(lock_info.ptrDataRoi);
    ASSERT_EQ(memcmp(composition_line, expected_result_w_mask, 4), 0);
    composition_line += lock_info.stride;
    ASSERT_EQ(memcmp(composition_line, expected_result_w_mask + 4, 4), 0);
    composition_line += lock_info.stride;
    ASSERT_EQ(memcmp(composition_line, expected_result_w_mask + 8, 4), 0);
    composition_line += lock_info.stride;
    ASSERT_EQ(memcmp(composition_line, expected_result_w_mask + 12, 4), 0);

    error_code = libCZI_BitmapUnlock(bitmap_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    error_code = libCZI_ReleaseBitmap(bitmap_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    error_code = libCZI_ReleaseCreateSingleChannelTileAccessor(accessor_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    error_code = libCZI_ReleaseReader(reader_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);
}
