// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "pch.h"
#include "inc_libCZI.h"
#include "../libCZI/CziParse.h"

using namespace libCZI;

TEST(Utilities, CompareCoordinates1)
{
    CDimCoordinate a{ { libCZI::DimensionIndex::C,1 } };
    CDimCoordinate b{ { libCZI::DimensionIndex::C,2 } };

    int r = Utils::Compare(&a, &b);

    EXPECT_TRUE(r < 0) << "expecting 'a' to be less than 'b'";
}

TEST(Utilities, CompareCoordinates2)
{
    CDimCoordinate a{ { libCZI::DimensionIndex::C,1 },{ libCZI::DimensionIndex::T,1 } };
    CDimCoordinate b{ { libCZI::DimensionIndex::C,1 },{ libCZI::DimensionIndex::T,2 } };

    // coordinates are compared in the "numerical order of the dimension-enums", and C comes before T
    int r = Utils::Compare(&a, &b);

    EXPECT_TRUE(r < 0) << "expecting 'a' to be less than 'b'";
}

TEST(Utilities, CompareCoordinates3)
{
    CDimCoordinate a{ { libCZI::DimensionIndex::C,2 },{ libCZI::DimensionIndex::Z,1 } };
    CDimCoordinate b{ { libCZI::DimensionIndex::C,1 },{ libCZI::DimensionIndex::Z,2 } };

    // coordinates are compared in the "numerical order of the dimension-enums", and Z comes before C
    int r = Utils::Compare(&a, &b);

    EXPECT_TRUE(r < 0) << "expecting 'a' to be less than 'b'";
}

TEST(Utilities, CompareCoordinates4)
{
    CDimCoordinate a{ { libCZI::DimensionIndex::C,2 },{ libCZI::DimensionIndex::T,1 } };
    CDimCoordinate b{ { libCZI::DimensionIndex::C,1 },{ libCZI::DimensionIndex::T,2 } };

    // coordinates are compared in the "numerical order of the dimension-enums", and C comes before T
    int r = Utils::Compare(&a, &b);

    EXPECT_TRUE(r > 0) << "expecting 'b' to be less than 'a'";
}

TEST(Utilities, CompareCoordinates5)
{
    CDimCoordinate a{ { libCZI::DimensionIndex::T,1 } };
    CDimCoordinate b{ { libCZI::DimensionIndex::C,1 },{ libCZI::DimensionIndex::T,2 } };

    // coordinates are compared in the "numerical order of the dimension-enums", and C comes before T, so
    // we first try to compare DimensionIndex::C (which is invalid for a and valid for b), so a is less than b
    int r = Utils::Compare(&a, &b);

    EXPECT_TRUE(r < 0) << "expecting 'a' to be less than 'b'";
}

TEST(Utilities, CompareCoordinates6)
{
    CDimCoordinate a{ { libCZI::DimensionIndex::C,1 },{ libCZI::DimensionIndex::T,2 },{ libCZI::DimensionIndex::Z,5 } };
    CDimCoordinate b{ { libCZI::DimensionIndex::C,1 },{ libCZI::DimensionIndex::T,2 },{ libCZI::DimensionIndex::Z,5 } };

    // those are obviously equal
    int r = Utils::Compare(&a, &b);

    EXPECT_TRUE(r == 0) << "expecting 'a' to be equal to 'b'";
}

TEST(Utilities, HasSameDimensionsExpectTrue1)
{
    CDimCoordinate a{ { libCZI::DimensionIndex::C,1 },{ libCZI::DimensionIndex::T,2 },{ libCZI::DimensionIndex::Z,5 } };
    CDimCoordinate b{ { libCZI::DimensionIndex::C,1 },{ libCZI::DimensionIndex::T,2 },{ libCZI::DimensionIndex::Z,5 } };

    const bool sameDimensions = Utils::HasSameDimensions(&a, &b);

    EXPECT_TRUE(sameDimensions) << "expecting to find that 'a' and 'b' have same dimensions";
}

TEST(Utilities, HasSameDimensionsExpectTrue2)
{
    // use different order and different values, still expecting to get "true"
    CDimCoordinate a{ { libCZI::DimensionIndex::C,1 },{ libCZI::DimensionIndex::T,2 },{ libCZI::DimensionIndex::Z,5 } };
    CDimCoordinate b{ { libCZI::DimensionIndex::T,9 },{ libCZI::DimensionIndex::Z,7 }, { libCZI::DimensionIndex::C,1 } };

    const bool sameDimensions = Utils::HasSameDimensions(&a, &b);

    EXPECT_TRUE(sameDimensions) << "expecting to find that 'a' and 'b' have same dimensions";
}

TEST(Utilities, HasSameDimensionsExpectFalse1)
{
    CDimCoordinate a{ { libCZI::DimensionIndex::C,1 },{ libCZI::DimensionIndex::T,2 },{ libCZI::DimensionIndex::Z,5 } };
    CDimCoordinate b{ {  libCZI::DimensionIndex::T,2 },{ libCZI::DimensionIndex::Z,5 } };

    const bool sameDimensions = Utils::HasSameDimensions(&a, &b);

    EXPECT_FALSE(sameDimensions) << "expecting to find that 'a' and 'b' to not have same dimensions";
}

TEST(Utilities, HasSameDimensionsExpectFalse2)
{
    CDimCoordinate a{ { libCZI::DimensionIndex::C,1 },{ libCZI::DimensionIndex::T,2 },{ libCZI::DimensionIndex::Z,5 } };
    CDimCoordinate b{ { libCZI::DimensionIndex::C,1 },{  libCZI::DimensionIndex::T,2 } };

    const bool sameDimensions = Utils::HasSameDimensions(&a, &b);

    EXPECT_FALSE(sameDimensions) << "expecting to find that 'a' and 'b' to not have same dimensions";
}

class BitmapLockTestMock : public IBitmapData
{
private:
    int lckCnt = 0;
public:
    virtual PixelType		GetPixelType() const
    {
        throw std::runtime_error("not implemented");
    }
    virtual IntSize			GetSize() const
    {
        throw std::runtime_error("not implemented");
    }
    virtual BitmapLockInfo	Lock()
    {
        this->lckCnt++;
        return BitmapLockInfo();
    }
    virtual void			Unlock()
    {
        this->lckCnt--;
    }
    int GetLockCnt() const
    {
        return this->lckCnt;
    }
};

TEST(Utilities, ScopedBitmapLocker1)
{
    auto bitmap = std::make_shared<BitmapLockTestMock>();

    {
        ScopedBitmapLockerSP locker1{ bitmap };
        EXPECT_EQ(bitmap->GetLockCnt(), 1) << "expecting a lock-count of '1'";

        {
            ScopedBitmapLockerSP locker2{ bitmap };
            EXPECT_EQ(bitmap->GetLockCnt(), 2) << "expecting a lock-count of '2'";
        }

        EXPECT_EQ(bitmap->GetLockCnt(), 1) << "expecting a lock-count of '1'";
    }

    EXPECT_EQ(bitmap->GetLockCnt(), 0) << "expecting a lock-count of zero";
}

TEST(Utilities, ScopedBitmapLocker2)
{
    auto bitmap = std::make_shared<BitmapLockTestMock>();

    // check that the scoped-bitmap-locker also works as expected when it is copied/moved
    {
        std::vector<ScopedBitmapLockerSP> vec;
        vec.push_back(ScopedBitmapLockerSP{ bitmap });
        vec.push_back(ScopedBitmapLockerSP{ bitmap });
        EXPECT_EQ(bitmap->GetLockCnt(), 2) << "expecting a lock-count of '2'";
    }

    EXPECT_EQ(bitmap->GetLockCnt(), 0) << "expecting a lock-count of zero";
}

TEST(Utilities, ScopedBitmapLocker3)
{
    auto bitmap = std::make_shared<BitmapLockTestMock>();

    // check that the scoped-bitmap-locker also works as expected when it is copied/moved
    {
        std::vector<ScopedBitmapLockerSP> vec;
        vec.emplace_back(ScopedBitmapLockerSP{ bitmap });
        vec.emplace_back(ScopedBitmapLockerSP{ bitmap });
        EXPECT_EQ(bitmap->GetLockCnt(), 2) << "expecting a lock-count of '2'";
    }

    EXPECT_EQ(bitmap->GetLockCnt(), 0) << "expecting a lock-count of zero";
}

// test-fixture, cf. https://stackoverflow.com/questions/47354280/what-is-the-best-way-of-testing-private-methods-with-googletest
class CCZIParseTest : public ::testing::Test
{
public:
    static libCZI::DimensionIndex DimensionCharToDimensionIndex(const char* ptr, size_t size)
    {
        return CCZIParse::DimensionCharToDimensionIndex(ptr, size);
    }
};

TEST(Utilities, DimensionCharToDimensionIndex1)
{
    auto dimIdx = CCZIParseTest::DimensionCharToDimensionIndex("C", 1);
    EXPECT_EQ(dimIdx, DimensionIndex::C) << "not the expected result";

    dimIdx = CCZIParseTest::DimensionCharToDimensionIndex("V", 1);
    EXPECT_EQ(dimIdx, DimensionIndex::V) << "not the expected result";

    dimIdx = CCZIParseTest::DimensionCharToDimensionIndex("t", 1);
    EXPECT_EQ(dimIdx, DimensionIndex::T) << "not the expected result";

    dimIdx = CCZIParseTest::DimensionCharToDimensionIndex("B", 1);
    EXPECT_EQ(dimIdx, DimensionIndex::B) << "not the expected result";

    dimIdx = CCZIParseTest::DimensionCharToDimensionIndex("Z", 1);
    EXPECT_EQ(dimIdx, DimensionIndex::Z) << "not the expected result";
}

TEST(Utilities, DimensionCharToDimensionIndex2)
{
    ASSERT_THROW(
        CCZIParseTest::DimensionCharToDimensionIndex("X", 1),
        LibCZICZIParseException) << "was expecting an exception here";
}

TEST(Utilities, ParseCompressionOptionAndCheckForCorrectness1)
{
    const auto compressionOptions = Utils::ParseCompressionOptions("zstd1:ExplicitLevel=2");
    EXPECT_EQ(compressionOptions.first, CompressionMode::Zstd1);
    CompressParameter value;
    ASSERT_TRUE(compressionOptions.second->TryGetProperty(CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL, &value));
    EXPECT_EQ(value.GetInt32(), 2);
}

TEST(Utilities, ParseCompressionOptionAndCheckForCorrectness2)
{
    const auto compressionOptions = Utils::ParseCompressionOptions("zstd1:ExplicitLevel=5;PreProcess=HiLoByteUnpack");
    EXPECT_EQ(compressionOptions.first, CompressionMode::Zstd1);
    CompressParameter value;
    ASSERT_TRUE(compressionOptions.second->TryGetProperty(CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL, &value));
    EXPECT_EQ(value.GetInt32(), 5);
    ASSERT_TRUE(compressionOptions.second->TryGetProperty(CompressionParameterKey::ZSTD_PREPROCESS_DOLOHIBYTEPACKING, &value));
    EXPECT_TRUE(value.GetBoolean());
}

TEST(Utilities, ParseCompressionOptionAndCheckForCorrectness3)
{
    const auto compressionOptions = Utils::ParseCompressionOptions("zstd1:ExplicitLevel=-43");
    EXPECT_EQ(compressionOptions.first, CompressionMode::Zstd1);
    CompressParameter value;
    ASSERT_TRUE(compressionOptions.second->TryGetProperty(CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL, &value));
    EXPECT_EQ(value.GetInt32(), -43);
}

TEST(Utilities, ParseCompressionOptionAndExpectError)
{
    // the string representation is invalid, so we expect an error (i.e. an exception to be thrown)
    EXPECT_THROW(
        { const auto compressionOptions = Utils::ParseCompressionOptions("zstd0:ExplicitLevel=5;;  ;PreProcess=HiLoByteUnpack"); },
        std::logic_error
    );
}

TEST(Utilities, ParseCompressionOptionAndCheckForUnknownKeyValuePairsBeingIgnored)
{
    // strategy currently is: unknown "key-value pairs" are ignored
    const auto compressionOptions = Utils::ParseCompressionOptions("zstd0:Xyz=125;ABC=uvw");
    EXPECT_EQ(compressionOptions.first, CompressionMode::Zstd0);
    CompressParameter value;
    ASSERT_FALSE(compressionOptions.second->TryGetProperty(CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL, &value));
    ASSERT_FALSE(compressionOptions.second->TryGetProperty(CompressionParameterKey::ZSTD_PREPROCESS_DOLOHIBYTEPACKING, &value));
}

TEST(Utilities, ParseCompressionOptionEmptyPropertyBagCheckForCorrectCompressionMethod)
{
    // ensure that this somewhat special case works as intended: there are no key-value pairs, just the compression method
    auto compressionOptions = Utils::ParseCompressionOptions("zstd0:");
    EXPECT_EQ(compressionOptions.first, CompressionMode::Zstd0);
    compressionOptions = Utils::ParseCompressionOptions("zstd1:");
    EXPECT_EQ(compressionOptions.first, CompressionMode::Zstd1);
}
