// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include <libCZIApi.h>

#include "testdata.h"
#include "utilities.h"
#include "MemoryInputStream.h"

using namespace std;

TEST(CZIAPI_Reader, ConstructExternalInputStreamAndOpenCZIAndCheck)
{
    CziReaderObjectHandle reader_object;
    InputStreamObjectHandle stream_object;

    LibCZIApiErrorCode error_code = libCZI_CreateReader(&reader_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    auto memory_input_stream_handler_object = new MemoryInputStream(CTestData::czi_with_subblock_of_size_t2, sizeof(CTestData::czi_with_subblock_of_size_t2));

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

    error_code = libCZI_CreateInputStreamFromExternal(&external_input_stream_struct, &stream_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    ReaderOpenInfoInterop reader_open_info;
    reader_open_info.streamObject = stream_object;
    error_code = libCZI_ReaderOpen(reader_object, &reader_open_info);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    error_code = libCZI_ReleaseInputStream(stream_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    SubBlockStatisticsInterop statistics;
    error_code = libCZI_ReaderGetStatisticsSimple(reader_object, &statistics);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);
    ASSERT_EQ(statistics.sub_block_count, 1);
    ASSERT_EQ(statistics.min_m_index, 0);
    ASSERT_EQ(statistics.max_m_index, 0);
    ASSERT_TRUE(statistics.bounding_box.x == 0 && statistics.bounding_box.y == 0 && statistics.bounding_box.w == 1 && statistics.bounding_box.h == 1);
    ASSERT_TRUE(statistics.bounding_box_layer0.x == 0 && statistics.bounding_box_layer0.y == 0 && statistics.bounding_box_layer0.w == 1 && statistics.bounding_box_layer0.h == 1);
    libCZI::CDimBounds dim_bounds;
    dim_bounds = Utilities::ConvertDimBoundsInterop(statistics.dim_bounds);
    const auto dim_bounds_as_string = libCZI::Utils::DimBoundsToString(&dim_bounds);
    ASSERT_STREQ("C0:1T0:1", dim_bounds_as_string.c_str());

    error_code = libCZI_ReleaseReader(reader_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    ASSERT_EQ(1, input_stream_release_call_count) << "The 'external input-stream-object' is not released as expected.";
}

TEST(CZIAPI_Reader, ConstructCziAndOpenCziAndCheckContent)
{
    Utilities::MosaicInfo  mosaic_info{ 5,5,{ {0,0,1}, {10,10,2}, {10,0,3}, {0,10,4}} };
    auto czi_data = Utilities::CreateMosaicCzi(mosaic_info);
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

    SubBlockStatisticsInterop statistics;
    error_code = libCZI_ReaderGetStatisticsSimple(reader_object, &statistics);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);
    ASSERT_EQ(statistics.sub_block_count, 4);
    ASSERT_EQ(statistics.min_m_index, 0);
    ASSERT_EQ(statistics.max_m_index, 3);
    ASSERT_TRUE(statistics.bounding_box.x == 0 && statistics.bounding_box.y == 0 && statistics.bounding_box.w == 15 && statistics.bounding_box.h == 15);
    ASSERT_TRUE(statistics.bounding_box_layer0.x == 0 && statistics.bounding_box_layer0.y == 0 && statistics.bounding_box_layer0.w == 15 && statistics.bounding_box_layer0.h == 15);
    libCZI::CDimBounds dim_bounds;
    dim_bounds = Utilities::ConvertDimBoundsInterop(statistics.dim_bounds);
    const auto dim_bounds_as_string = libCZI::Utils::DimBoundsToString(&dim_bounds);
    ASSERT_STREQ("C0:1", dim_bounds_as_string.c_str());

    SubBlockObjectHandle sub_block_object;
    error_code = libCZI_ReaderReadSubBlock(reader_object, 0, &sub_block_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    BitmapObjectHandle bitmap_object;
    error_code = libCZI_SubBlockCreateBitmap(sub_block_object, &bitmap_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);
    BitmapInfoInterop bitmap_info;
    libCZI_BitmapGetInfo(bitmap_object, &bitmap_info);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);
    ASSERT_EQ(5, bitmap_info.width);
    ASSERT_EQ(5, bitmap_info.height);
    ASSERT_EQ(static_cast<int32_t>(libCZI::PixelType::Gray8), bitmap_info.pixelType);

    error_code = libCZI_ReleaseBitmap(bitmap_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);
    error_code = libCZI_ReleaseSubBlock(sub_block_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    error_code = libCZI_ReleaseReader(reader_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    ASSERT_EQ(1, input_stream_release_call_count) << "The 'external input-stream-object' is not released as expected.";
};

TEST(CZIAPI_Reader, ConstructMultiSceneCziAndOpenCziAndCheckContent)
{
    map<int, Utilities::MosaicInfo> per_scene_mosaic_info
    {
        {0,{ 5,5,{ {0,0,1}, {10,10,2}, {10,0,3}, {0,10,4}} }},
        {1,{ 3,3,{ {20,20,3}, {23,20,4}} }},
        {2,{ 2,2,{ {30,30,5}} }}
    };

    auto czi_data = Utilities::CreateMultiSceneMosaicCzi(per_scene_mosaic_info);
    auto memory_input_stream_handler_object = new MemoryInputStream(get<0>(czi_data).get(), get<1>(czi_data));

    /*FILE* fp = fopen("P:\\3scenes.czi", "wb");
    fwrite(get<0>(czi_data).get(), 1, get<1>(czi_data), fp);
    fclose(fp);*/

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

    SubBlockStatisticsInterop statistics;
    error_code = libCZI_ReaderGetStatisticsSimple(reader_object, &statistics);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);
    ASSERT_EQ(statistics.sub_block_count, 7);
    ASSERT_EQ(statistics.min_m_index, 0);
    ASSERT_EQ(statistics.max_m_index, 3);
    ASSERT_TRUE(statistics.bounding_box.x == 0 && statistics.bounding_box.y == 0 && statistics.bounding_box.w == 32 && statistics.bounding_box.h == 32);
    ASSERT_TRUE(statistics.bounding_box_layer0.x == 0 && statistics.bounding_box_layer0.y == 0 && statistics.bounding_box_layer0.w == 32 && statistics.bounding_box_layer0.h == 32);
    libCZI::CDimBounds dim_bounds;
    dim_bounds = Utilities::ConvertDimBoundsInterop(statistics.dim_bounds);
    const auto dim_bounds_as_string = libCZI::Utils::DimBoundsToString(&dim_bounds);
    ASSERT_STREQ("C0:1S0:3", dim_bounds_as_string.c_str());

    int number_of_per_channel_bounding_boxes = 4;
    SubBlockStatisticsInteropEx* statistics_ex = static_cast<SubBlockStatisticsInteropEx*>(malloc(sizeof(SubBlockStatisticsInteropEx) + number_of_per_channel_bounding_boxes * sizeof(BoundingBoxesInterop)));
    error_code = libCZI_ReaderGetStatisticsEx(reader_object, statistics_ex, &number_of_per_channel_bounding_boxes);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);
    ASSERT_EQ(number_of_per_channel_bounding_boxes, 3);
    ASSERT_EQ(statistics_ex->number_of_per_scenes_bounding_boxes, 3);
    ASSERT_TRUE(statistics_ex->per_scenes_bounding_boxes[0].bounding_box.x == 0 && statistics_ex->per_scenes_bounding_boxes[0].bounding_box.y == 0 && statistics_ex->per_scenes_bounding_boxes[0].bounding_box.w == 15 && statistics_ex->per_scenes_bounding_boxes[0].bounding_box.h == 15);
    ASSERT_TRUE(statistics_ex->per_scenes_bounding_boxes[0].bounding_box_layer0_only.x == 0 && statistics_ex->per_scenes_bounding_boxes[0].bounding_box_layer0_only.y == 0 && statistics_ex->per_scenes_bounding_boxes[0].bounding_box_layer0_only.w == 15 && statistics_ex->per_scenes_bounding_boxes[0].bounding_box_layer0_only.h == 15);
    ASSERT_TRUE(statistics_ex->per_scenes_bounding_boxes[1].bounding_box.x == 20 && statistics_ex->per_scenes_bounding_boxes[1].bounding_box.y == 20 && statistics_ex->per_scenes_bounding_boxes[1].bounding_box.w == 6 && statistics_ex->per_scenes_bounding_boxes[1].bounding_box.h == 3);
    ASSERT_TRUE(statistics_ex->per_scenes_bounding_boxes[1].bounding_box_layer0_only.x == 20 && statistics_ex->per_scenes_bounding_boxes[1].bounding_box_layer0_only.y == 20 && statistics_ex->per_scenes_bounding_boxes[1].bounding_box_layer0_only.w == 6 && statistics_ex->per_scenes_bounding_boxes[1].bounding_box_layer0_only.h == 3);
    ASSERT_TRUE(statistics_ex->per_scenes_bounding_boxes[2].bounding_box.x == 30 && statistics_ex->per_scenes_bounding_boxes[2].bounding_box.y == 30 && statistics_ex->per_scenes_bounding_boxes[2].bounding_box.w == 2 && statistics_ex->per_scenes_bounding_boxes[2].bounding_box.h == 2);
    ASSERT_TRUE(statistics_ex->per_scenes_bounding_boxes[2].bounding_box_layer0_only.x == 30 && statistics_ex->per_scenes_bounding_boxes[2].bounding_box_layer0_only.y == 30 && statistics_ex->per_scenes_bounding_boxes[2].bounding_box_layer0_only.w == 2 && statistics_ex->per_scenes_bounding_boxes[2].bounding_box_layer0_only.h == 2);
    free(statistics_ex);

    for (int i = 3; i >= 0; --i)
    {
        number_of_per_channel_bounding_boxes = i;
        statistics_ex = static_cast<SubBlockStatisticsInteropEx*>(malloc(sizeof(SubBlockStatisticsInteropEx) + number_of_per_channel_bounding_boxes * sizeof(BoundingBoxesInterop)));
        error_code = libCZI_ReaderGetStatisticsEx(reader_object, statistics_ex, &number_of_per_channel_bounding_boxes);
        ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);
        ASSERT_EQ(number_of_per_channel_bounding_boxes, 3);
        ASSERT_EQ(statistics_ex->number_of_per_scenes_bounding_boxes, i);
        free(statistics_ex);
    }

    error_code = libCZI_ReleaseReader(reader_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    ASSERT_EQ(1, input_stream_release_call_count) << "The 'external input-stream-object' is not released as expected.";
};

TEST(CZIAPI_Reader, ConstructExternalInputStreamAndTryGetSubBlockInfoForIndex)
{
    CziReaderObjectHandle reader_object;
    InputStreamObjectHandle stream_object;

    LibCZIApiErrorCode error_code = libCZI_CreateReader(&reader_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    auto memory_input_stream_handler_object = new MemoryInputStream(CTestData::czi_with_subblock_of_size_t2, sizeof(CTestData::czi_with_subblock_of_size_t2));

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

    error_code = libCZI_CreateInputStreamFromExternal(&external_input_stream_struct, &stream_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    ReaderOpenInfoInterop reader_open_info;
    reader_open_info.streamObject = stream_object;
    error_code = libCZI_ReaderOpen(reader_object, &reader_open_info);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    error_code = libCZI_ReleaseInputStream(stream_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    SubBlockInfoInterop info;
    error_code = libCZI_TryGetSubBlockInfoForIndex(reader_object, 0, &info);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);
    ASSERT_EQ(info.compression_mode_raw, 0);

    ASSERT_EQ(info.logical_rect.x, 0);
    ASSERT_EQ(info.logical_rect.y, 0);
    ASSERT_EQ(info.logical_rect.w, 1);
    ASSERT_EQ(info.logical_rect.h, 1);

    ASSERT_EQ(info.m_index, 0);

    ASSERT_EQ(info.physical_size.w, 1);
    ASSERT_EQ(info.physical_size.h, 1);

    ASSERT_EQ(info.pixel_type, 0);

    const libCZI::CDimCoordinate dim_coordinate = Utilities::ConvertCoordinateInterop(info.coordinate);
    const auto dim_coordinate_as_string = libCZI::Utils::DimCoordinateToString(&dim_coordinate);
    ASSERT_STREQ("C0T0", dim_coordinate_as_string.c_str());

    error_code = libCZI_ReleaseReader(reader_object);
    ASSERT_EQ(LibCZIApi_ErrorCode_OK, error_code);

    ASSERT_EQ(1, input_stream_release_call_count) << "The 'external input-stream-object' is not released as expected.";
}
