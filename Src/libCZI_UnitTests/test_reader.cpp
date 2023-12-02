// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"

using namespace libCZI;
using namespace std;

TEST(DimCoordinate, ReaderException)
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

TEST(DimCoordinate, ReaderException2)
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

TEST(DimCoordinate, ReaderStateException)
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
