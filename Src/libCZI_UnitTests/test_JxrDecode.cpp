// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include <cstdint>
#include "inc_libCZI.h"
#include "testImage.h"
#include "../libCZI/decoder.h"

using namespace libCZI;
using namespace std;

TEST(JxrDecode, DecodeBgr24)
{
    auto dec = CJxrLibDecoder::Create();
    size_t sizeEncoded; int expectedWidth, expectedHeight;
    auto ptrEncodedData = CTestImage::GetJpgXrCompressedImage_Bgr24(&sizeEncoded, &expectedWidth, &expectedHeight);
    auto bmDecoded = dec->Decode(ptrEncodedData, sizeEncoded, libCZI::PixelType::Bgr24, expectedWidth, expectedHeight);
    EXPECT_EQ((uint32_t)expectedWidth, bmDecoded->GetWidth()) << "Width is expected to be equal";
    EXPECT_EQ((uint32_t)expectedHeight, bmDecoded->GetHeight()) << "Height is expected to be equal";
    EXPECT_EQ(bmDecoded->GetPixelType(), PixelType::Bgr24) << "Not the correct pixeltype.";

    uint8_t hash[16] = { 0 };
    Utils::CalcMd5SumHash(bmDecoded.get(), hash, sizeof(hash));

    static const uint8_t expectedResult[16] = { 0x04,0x77,0x2f,0x32,0x2f,0x94,0x9b,0x07,0x0d,0x53,0xa5,0x24,0xea,0x64,0x5a,0x1a };
    EXPECT_TRUE(memcmp(hash, expectedResult, 16) == 0) << "Incorrect result";
}

TEST(JxrDecode, DecodeGray8)
{
    auto dec = CJxrLibDecoder::Create();
    size_t sizeEncoded; int expectedWidth, expectedHeight;
    auto ptrEncodedData = CTestImage::GetJpgXrCompressedImage_Gray8(&sizeEncoded, &expectedWidth, &expectedHeight);
    auto bmDecoded = dec->Decode(ptrEncodedData, sizeEncoded, libCZI::PixelType::Gray8, expectedWidth, expectedHeight);
    EXPECT_EQ((uint32_t)expectedWidth, bmDecoded->GetWidth()) << "Width is expected to be equal";
    EXPECT_EQ((uint32_t)expectedHeight, bmDecoded->GetHeight()) << "Height is expected to be equal";
    EXPECT_EQ(bmDecoded->GetPixelType(), PixelType::Gray8) << "Not the correct pixeltype.";

    uint8_t hash[16] = { 0 };
    Utils::CalcMd5SumHash(bmDecoded.get(), hash, sizeof(hash));

    static const uint8_t expectedResult[16] = { 0x95, 0x4c, 0x70, 0x70, 0xae, 0xfb, 0x63, 0xc6, 0xc4, 0x0a, 0xb5, 0xec, 0xef, 0x73, 0x09, 0x8d };
    EXPECT_TRUE(memcmp(hash, expectedResult, 16) == 0) << "Incorrect result";
}

TEST(JxrDecode, TryDecodeInvalidDataExpectException)
{
    // pass invalid data to decoder, and expect an exception
    const auto dec = CJxrLibDecoder::Create();
    size_t sizeEncoded = 2345;
    unique_ptr<uint8_t> encoded_data(new uint8_t[sizeEncoded]);
    for (size_t i = 0; i < sizeEncoded; i++)
    {
        encoded_data.get()[i] = static_cast<uint8_t>(i);
    }

    EXPECT_ANY_THROW(dec->Decode(encoded_data.get(), sizeEncoded, libCZI::PixelType::Invalid, 0, 0));
}

TEST(JxrDecode, TestEncoder)
{
    auto bitmap = CBitmapData<CHeapAllocator>::Create(PixelType::Bgr24, CTestImage::BGR24TESTIMAGE_WIDTH, CTestImage::BGR24TESTIMAGE_HEIGHT);
    {
        ScopedBitmapLockerSP lck{ bitmap };
        CTestImage::CopyBgr24Image(lck.ptrDataRoi, bitmap->GetWidth(), bitmap->GetHeight(), lck.stride);
    }

    const auto codec = CJxrLibDecoder::Create();

    {
        ScopedBitmapLockerSP lck{ bitmap };
        codec->Encode(
            bitmap->GetPixelType(), 
            bitmap->GetWidth(), 
            bitmap->GetHeight(), 
            lck.stride, 
            lck.ptrDataRoi);
    }
}
