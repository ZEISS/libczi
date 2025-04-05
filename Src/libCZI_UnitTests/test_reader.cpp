// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"
#include "MemInputOutputStream.h"
#include "MemOutputStream.h"
#include "utils.h"
#include <array>
#include <thread>

using namespace libCZI;
using namespace std;

namespace
{
    tuple<shared_ptr<void>, size_t> CreateCziDocumentOneSubblock4x4Gray8()
    {
        auto writer = CreateCZIWriter();
        auto outStream = make_shared<CMemOutputStream>(0);
        auto spWriterInfo = make_shared<CCziWriterInfo >(GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } });
        writer->Create(outStream, spWriterInfo);
        auto bitmap = CreateTestBitmap(PixelType::Gray8, 4, 4);
        ScopedBitmapLockerSP lockBm{ bitmap };
        AddSubBlockInfoStridedBitmap addSbBlkInfo;
        addSbBlkInfo.Clear();
        addSbBlkInfo.coordinate = CDimCoordinate::Parse("C0");
        addSbBlkInfo.mIndexValid = true;
        addSbBlkInfo.mIndex = 0;
        addSbBlkInfo.x = 0;
        addSbBlkInfo.y = 0;
        addSbBlkInfo.logicalWidth = bitmap->GetWidth();
        addSbBlkInfo.logicalHeight = bitmap->GetHeight();
        addSbBlkInfo.physicalWidth = bitmap->GetWidth();
        addSbBlkInfo.physicalHeight = bitmap->GetHeight();
        addSbBlkInfo.PixelType = bitmap->GetPixelType();
        addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
        addSbBlkInfo.strideBitmap = lockBm.stride;
        writer->SyncAddSubBlock(addSbBlkInfo);
        writer->Close();

        size_t size_data;
        const auto data = outStream->GetCopy(&size_data);
        return make_tuple(data, size_data);
    }

    tuple<shared_ptr<void>, size_t> CreateCziDocumentContainingOneSubblockWhichIsTooShort()
    {
        auto writer = CreateCZIWriter();
        auto outStream = make_shared<CMemOutputStream>(0);
        auto spWriterInfo = make_shared<CCziWriterInfo >(GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } });
        writer->Create(outStream, spWriterInfo);
        auto bitmap = CreateTestBitmap(PixelType::Gray8, 4, 4);
        uint8_t data_too_short[11] = { 0,1,2,3,4,5,6,7,8,9,10 };
        AddSubBlockInfoMemPtr addSbBlkInfo;
        addSbBlkInfo.Clear();
        addSbBlkInfo.coordinate = CDimCoordinate::Parse("C0");
        addSbBlkInfo.mIndexValid = true;
        addSbBlkInfo.mIndex = 0;
        addSbBlkInfo.x = 0;
        addSbBlkInfo.y = 0;
        addSbBlkInfo.logicalWidth = bitmap->GetWidth();
        addSbBlkInfo.logicalHeight = bitmap->GetHeight();
        addSbBlkInfo.physicalWidth = bitmap->GetWidth();
        addSbBlkInfo.physicalHeight = bitmap->GetHeight();
        addSbBlkInfo.PixelType = bitmap->GetPixelType();
        addSbBlkInfo.ptrData = data_too_short;
        addSbBlkInfo.dataSize = sizeof(data_too_short);
        addSbBlkInfo.SetCompressionMode(CompressionMode::UnCompressed);
        writer->SyncAddSubBlock(addSbBlkInfo);
        writer->Close();

        size_t size_data;
        const auto data = outStream->GetCopy(&size_data);
        return make_tuple(data, size_data);
    }
}

TEST(CziReader, ReaderException)
{
    class MyException : public std::exception
    {
    private:
        std::string exception_text_;
        std::error_code code_;
    public:
        MyException(const std::string& exceptionText, std::error_code code) :exception_text_(exceptionText), code_(code) {}

        const char* what() const noexcept override
        {
            return this->exception_text_.c_str();
        }

        std::error_code code() const noexcept
        {
            return this->code_;
        }
    };

    class CTestStreamImp :public libCZI::IStream
    {
    private:
        std::string exceptionText;
        std::error_code code;
    public:
        CTestStreamImp(const std::string& exceptionText, std::error_code code) :exceptionText(exceptionText), code(code) {}

        virtual void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override
        {
            throw MyException(this->exceptionText, this->code);
        }
    };

    static const char* ExceptionText = "Test-1";
    static std::error_code ErrorCode = error_code(42, generic_category());
    auto stream = std::make_shared<CTestStreamImp>(ExceptionText, ErrorCode);
    auto spReader = libCZI::CreateCZIReader();
    bool exceptionCorrect = false;
    try
    {
        spReader->Open(stream);
    }
    catch (LibCZIIOException& excp)
    {
        try
        {
            excp.rethrow_nested();
        }
        catch (MyException& innerExcp)
        {
            // according to standard, the content of the what()-test is implementation-specific,
            // so it is not suited for checking - but it seems that the code goes unaltered
            const char* errorText = innerExcp.what();
            error_code errCode = innerExcp.code();
            if (errCode == ErrorCode)
            {
                exceptionCorrect = true;
            }
        }
    }

    EXPECT_TRUE(exceptionCorrect) << "Incorrect result";
}

TEST(CziReader, ReaderException2)
{
    class CTestStreamImp :public libCZI::IStream
    {
    public:
        virtual void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override
        {
            if (offset != 0 || size < 16)
            {
                throw std::ios_base::failure("UNEXPECTED");
            }

            static const std::uint8_t FILEHDRMAGIC[16] = { 'Z','I','S','R','A','W','F','I','L' + 1,'E','\0','\0','\0','\0','\0','\0' };
            memset(pv, 0, (size_t)size);
            memcpy(pv, FILEHDRMAGIC, 16);
            *ptrBytesRead = size;
        }
    };

    static const char* ExceptionText = "Test-1";
    static std::error_code ErrorCode = error_code(42, generic_category());
    auto stream = std::make_shared<CTestStreamImp>();
    auto spReader = libCZI::CreateCZIReader();
    bool exceptionCorrect = false;
    try
    {
        spReader->Open(stream);
    }
    catch (LibCZICZIParseException& excp)
    {
        auto errCode = excp.GetErrorCode();
        if (errCode == LibCZICZIParseException::ErrorCode::CorruptedData)
        {
            exceptionCorrect = true;
        }
    }

    EXPECT_TRUE(exceptionCorrect) << "Incorrect result";
}

TEST(CziReader, ReaderStateException)
{
    bool expectedExceptionCaught = false;
    auto spReader = libCZI::CreateCZIReader();
    try
    {
        spReader->GetStatistics();
    }
    catch (logic_error)
    {
        expectedExceptionCaught = true;
    }

    EXPECT_TRUE(expectedExceptionCaught) << "Incorrect behavior";
}

TEST(CziReader, CheckThatSubBlockInfoFromSubBlockDirectoryIsAuthorativeByDefaultNoException)
{
    // with this test we verify that the information in the subblock-directory is used, not
    //  the information in the subblock-header.

    // now, we modify the information in the sub-block-header
    auto test_czi = CreateCziDocumentOneSubblock4x4Gray8();
    uint8_t* p = static_cast<uint8_t*>(get<0>(test_czi).get());
    ASSERT_TRUE(*(p + 0x250) == 'D' && *(p + 0x251) == 'V') << "The CZI-document does not have the expected content.";
    ASSERT_TRUE(*(p + 0x2ac) == 'C') << "The CZI-document does not have the expected content.";

    *(p + 0x252) = static_cast<uint8_t>(PixelType::Gray16); // change pixel type
    *(p + 0x2b0) = 0x01;    // set the C-coordinate to '1'
    *(p + 0x278) = *(p + 0x280) = 0x05;    // set  Size-X to '5'
    *(p + 0x28c) = *(p + 0x294) = 0x06;    // set  Size-Y to '6'

    // act
    auto inputStream = CreateStreamFromMemory(get<0>(test_czi), get<1>(test_czi));
    auto reader = CreateCZIReader();
    ICZIReader::OpenOptions openOptions;
    // instruct to "ignore" a discrepancy between the sub-block-header and the sub-block-directory, and use default for "which information
    //  takes precedence", where default is "sub-block-directory"
    openOptions.subBlockDirectoryInfoPolicy = openOptions.subBlockDirectoryInfoPolicy | ICZIReader::OpenOptions::SubBlockDirectoryInfoPolicy::IgnoreDiscrepancy;
    reader->Open(inputStream, &openOptions);
    auto sub_block = reader->ReadSubBlock(0);

    // assert

    // now, we expect that we get the information from the subblock-directory
    const auto sub_block_info = sub_block->GetSubBlockInfo();
    EXPECT_EQ(sub_block_info.pixelType, PixelType::Gray8) << "Incorrect behavior, this information is to be retrieved from sub-block directory";
    EXPECT_EQ(sub_block_info.physicalSize.w, 4) << "Incorrect behavior, this information is to be retrieved from sub-block directory";
    EXPECT_EQ(sub_block_info.physicalSize.h, 4) << "Incorrect behavior, this information is to be retrieved from sub-block directory";
    EXPECT_EQ(sub_block_info.logicalRect.w, 4) << "Incorrect behavior, this information is to be retrieved from sub-block directory";
    EXPECT_EQ(sub_block_info.logicalRect.h, 4) << "Incorrect behavior, this information is to be retrieved from sub-block directory";
    int c_index = -1;
    const bool b = sub_block_info.coordinate.TryGetPosition(DimensionIndex::C, &c_index);
    EXPECT_TRUE(b) << "Incorrect behavior, this information is to be retrieved from sub-block directory";
    EXPECT_EQ(c_index, 0) << "Incorrect behavior, this information is to be retrieved from sub-block directory";
}

TEST(CziReader, CheckThatSubBlockInfoFromSubBlockHeaderIsUsedIfConfiguredNoException)
{
    // With this test we verify that the information in the subblock-header is used, not
    //  the information from the subblock-directory. Note that this is *not* the default behavior,
    //  and not the recommended behavior.

    // now, we modify the information in the sub-block-header
    auto test_czi = CreateCziDocumentOneSubblock4x4Gray8();
    uint8_t* p = static_cast<uint8_t*>(get<0>(test_czi).get());
    ASSERT_TRUE(*(p + 0x250) == 'D' && *(p + 0x251) == 'V') << "The CZI-document does not have the expected content.";
    ASSERT_TRUE(*(p + 0x2ac) == 'C') << "The CZI-document does not have the expected content.";

    *(p + 0x252) = static_cast<uint8_t>(PixelType::Gray16); // change pixel type
    *(p + 0x2b0) = 0x01;    // set the C-coordinate to '1'
    *(p + 0x278) = *(p + 0x280) = 0x05;    // set  Size-X to '5'
    *(p + 0x28c) = *(p + 0x294) = 0x06;    // set  Size-Y to '6'

    // act
    auto inputStream = CreateStreamFromMemory(get<0>(test_czi), get<1>(test_czi));
    auto reader = CreateCZIReader();
    ICZIReader::OpenOptions openOptions;
    // instruct to "ignore" a discrepancy between the sub-block-header and the sub-block-directory, and give precedence to the sub-block-header
    openOptions.subBlockDirectoryInfoPolicy = ICZIReader::OpenOptions::SubBlockDirectoryInfoPolicy::SubBlockHeaderPrecedence | ICZIReader::OpenOptions::SubBlockDirectoryInfoPolicy::IgnoreDiscrepancy;
    reader->Open(inputStream, &openOptions);
    auto sub_block = reader->ReadSubBlock(0);

    // assert

    // now, we expect that we get the information from the subblock-directory
    const auto sub_block_info = sub_block->GetSubBlockInfo();
    EXPECT_EQ(sub_block_info.pixelType, PixelType::Gray16) << "Incorrect behavior, this information is to be retrieved from sub-block header";
    EXPECT_EQ(sub_block_info.physicalSize.w, 5) << "Incorrect behavior, this information is to be retrieved from sub-block header";
    EXPECT_EQ(sub_block_info.physicalSize.h, 6) << "Incorrect behavior, this information is to be retrieved from sub-block header";
    EXPECT_EQ(sub_block_info.logicalRect.w, 5) << "Incorrect behavior, this information is to be retrieved from sub-block header";
    EXPECT_EQ(sub_block_info.logicalRect.h, 6) << "Incorrect behavior, this information is to be retrieved from sub-block header";
    int c_index = -1;
    const bool b = sub_block_info.coordinate.TryGetPosition(DimensionIndex::C, &c_index);
    EXPECT_TRUE(b) << "Incorrect behavior, this information is to be retrieved from sub-block header";
    EXPECT_EQ(c_index, 1) << "Incorrect behavior, this information is to be retrieved from sub-block header";
}

TEST(CziReader, CheckThatExceptionIsThrownWhenEnabledIfSubBlockDirectoryAndSubblockHeaderDiffer)
{
    // arrange
    auto test_czi = CreateCziDocumentOneSubblock4x4Gray8();
    uint8_t* p = static_cast<uint8_t*>(get<0>(test_czi).get());
    ASSERT_TRUE(*(p + 0x250) == 'D' && *(p + 0x251) == 'V') << "The CZI-document does not have the expected content.";
    ASSERT_TRUE(*(p + 0x2ac) == 'C') << "The CZI-document does not have the expected content.";

    *(p + 0x252) = static_cast<uint8_t>(PixelType::Gray16);     // change pixel type
    *(p + 0x2b0) = 0x01;                                        // set the C-coordinate to '1'
    *(p + 0x278) = *(p + 0x280) = 0x05;                         // set  Size-X to '5'
    *(p + 0x28c) = *(p + 0x294) = 0x06;                         // set  Size-Y to '6'

    // act
    auto inputStream = CreateStreamFromMemory(get<0>(test_czi), get<1>(test_czi));
    auto reader = CreateCZIReader();

    // use default options
    reader->Open(inputStream);
    try
    {
        auto sub_block = reader->ReadSubBlock(0);
        FAIL() << "Expected LibCZICZIParseException to be thrown";
    }
    catch (const LibCZICZIParseException& exception)
    {
        EXPECT_EQ(exception.GetErrorCode(), LibCZICZIParseException::ErrorCode::SubBlockDirectoryToSubBlockHeaderMismatch) << "not the correct errorcode";
    }
    catch (...)
    {
        FAIL() << "Expected LibCZICZIParseException";
    }
}

static tuple<shared_ptr<void>, size_t> CreateTestCzi()
{
    const auto writer = CreateCZIWriter();
    const auto outStream = make_shared<CMemOutputStream>(0);

    const auto spWriterInfo = make_shared<CCziWriterInfo>(
        GUID{ 0,0,0,{ 0,0,0,0,0,0,0,0 } },
        CDimBounds{ { DimensionIndex::T, 0, 1 }, { DimensionIndex::C, 0, 1 } });

    writer->Create(outStream, spWriterInfo);

    int count = 0;
    for (count = 0; count < 10; ++count)
    {
        ++count;
        const size_t size_of_bitmap = 100 * 100;
        unique_ptr<uint8_t[]> bitmap(new uint8_t[size_of_bitmap]);
        memset(bitmap.get(), count, size_of_bitmap);
        AddSubBlockInfoStridedBitmap addSbBlkInfo;
        addSbBlkInfo.Clear();
        addSbBlkInfo.coordinate.Set(DimensionIndex::C, 0);
        addSbBlkInfo.coordinate.Set(DimensionIndex::T, 0);
        addSbBlkInfo.mIndexValid = true;
        addSbBlkInfo.mIndex = count;
        addSbBlkInfo.x = 0;
        addSbBlkInfo.y = 0;
        addSbBlkInfo.logicalWidth = 100;
        addSbBlkInfo.logicalHeight = 100;
        addSbBlkInfo.physicalWidth = 100;
        addSbBlkInfo.physicalHeight = 100;
        addSbBlkInfo.PixelType = PixelType::Gray8;
        addSbBlkInfo.ptrBitmap = bitmap.get();
        addSbBlkInfo.strideBitmap = 100;
        writer->SyncAddSubBlock(addSbBlkInfo);
    }

    const auto metaDataBuilder = writer->GetPreparedMetadata(PrepareMetadataInfo{});

    WriteMetadataInfo write_metadata_info;
    const auto& strMetadata = metaDataBuilder->GetXml();
    write_metadata_info.szMetadata = strMetadata.c_str();
    write_metadata_info.szMetadataSize = strMetadata.size() + 1;
    write_metadata_info.ptrAttachment = nullptr;
    write_metadata_info.attachmentSize = 0;
    writer->SyncWriteMetadata(write_metadata_info);

    writer->Close();

    return make_tuple(outStream->GetCopy(nullptr), outStream->GetDataSize());
}

TEST(CziReader, Concurrency)
{
    // arrange
    auto czi_document_as_blob = CreateTestCzi();
    const auto memory_stream = make_shared<CMemInputOutputStream>(get<0>(czi_document_as_blob).get(), get<1>(czi_document_as_blob));
    const auto reader = CreateCZIReader();
    reader->Open(memory_stream);

    constexpr int numThreads = 5; // Number of threads to create
    std::array<thread, numThreads> threads; // Static array to store threads
    bool readsubblock_problem_occurred = false;
    for (int i = 0; i < 5; ++i)
    {
        threads[i] = thread([reader, i, &readsubblock_problem_occurred]()
        {
            try
            {
                reader->ReadSubBlock(i);
            }
            catch (logic_error&)
            {
                // Depending on the timing, we expect that either the operations succeeds (if the ReadSubBlock() call
                //  happens before the Close() call) or that it fails (if the ReadSubBlock() call happens after the Close() call).
                //  In the latter case, we expect a logic_error exception. Everything else is considered a problem.
                return;
            }
            catch (...)
            {
                readsubblock_problem_occurred = true;
            }
        });
    }

    reader->Close();

    for (thread& thread : threads)
    {
        thread.join();
    }

    EXPECT_FALSE(readsubblock_problem_occurred) << "Incorrect behavior";
}

TEST(CziReader, ReadSubBlockThatHasTooShortPayloadAndCheckResolutionProtocol)
{
    // arrange
    auto test_czi = CreateCziDocumentContainingOneSubblockWhichIsTooShort();

    // act
    auto input_stream = CreateStreamFromMemory(get<0>(test_czi), get<1>(test_czi));
    const auto reader = CreateCZIReader();
    reader->Open(input_stream);

    // assert
    const auto sub_block = reader->ReadSubBlock(0);

    CreateBitmapOptions options;
    options.handle_uncompressed_data_size_mismatch = true;
    auto bitmap = sub_block->CreateBitmap(&options);
    ASSERT_EQ(bitmap->GetWidth(), 4) << "Incorrect width";
    ASSERT_EQ(bitmap->GetHeight(), 4) << "Incorrect height";
    ASSERT_EQ(bitmap->GetPixelType(), PixelType::Gray8) << "Incorrect pixel type";
    auto locked_bitmap = bitmap->Lock();
    const uint8_t* bitmap_pointer = static_cast<const uint8_t*>(locked_bitmap.ptrDataRoi);
    EXPECT_EQ(bitmap_pointer[0 + 0 * locked_bitmap.stride], 0);
    EXPECT_EQ(bitmap_pointer[1 + 0 * locked_bitmap.stride], 1);
    EXPECT_EQ(bitmap_pointer[2 + 0 * locked_bitmap.stride], 2);
    EXPECT_EQ(bitmap_pointer[3 + 0 * locked_bitmap.stride], 3);
    EXPECT_EQ(bitmap_pointer[0 + 1 * locked_bitmap.stride], 4);
    EXPECT_EQ(bitmap_pointer[1 + 1 * locked_bitmap.stride], 5);
    EXPECT_EQ(bitmap_pointer[2 + 1 * locked_bitmap.stride], 6);
    EXPECT_EQ(bitmap_pointer[3 + 1 * locked_bitmap.stride], 7);
    EXPECT_EQ(bitmap_pointer[0 + 2 * locked_bitmap.stride], 8); 
    EXPECT_EQ(bitmap_pointer[1 + 2 * locked_bitmap.stride], 9);
    EXPECT_EQ(bitmap_pointer[2 + 2 * locked_bitmap.stride], 10);
    EXPECT_EQ(bitmap_pointer[3 + 2 * locked_bitmap.stride], 0);
    EXPECT_EQ(bitmap_pointer[0 + 3 * locked_bitmap.stride], 0); 
    EXPECT_EQ(bitmap_pointer[1 + 3 * locked_bitmap.stride], 0);
    EXPECT_EQ(bitmap_pointer[2 + 3 * locked_bitmap.stride], 0);
    EXPECT_EQ(bitmap_pointer[3 + 3 * locked_bitmap.stride], 0);
    bitmap->Unlock();
}
