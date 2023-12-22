// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"
#include "MemInputOutputStream.h"
#include "MemOutputStream.h"
#include <array>
#include <thread>

using namespace libCZI;
using namespace std;

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
