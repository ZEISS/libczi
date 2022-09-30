// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "pch.h"
#include "inc_libCZI.h"
#include "testImage.h"
#include "../libCZI/decoder.h"

using namespace libCZI;
using namespace std;

TEST(JxrDecode, Decode1)
{
		auto dec = CJxrLibDecoder::Create();
		size_t sizeEncoded; int expectedWidth, expectedHeight;
		auto ptrEncodedData = CTestImage::GetJpgXrCompressedImage(&sizeEncoded, &expectedWidth, &expectedHeight);
		auto bmDecoded = dec->Decode(ptrEncodedData, sizeEncoded, libCZI::PixelType::Bgr24, expectedWidth, expectedHeight);
		EXPECT_EQ((uint32_t)expectedWidth, bmDecoded->GetWidth()) << "Width is expected to be equal";
		EXPECT_EQ((uint32_t)expectedHeight, bmDecoded->GetHeight()) << "Height is expected to be equal";
		EXPECT_EQ(bmDecoded->GetPixelType(), PixelType::Bgr24) << "Not the correct pixeltype.";

		uint8_t hash[16] = { 0 };
		Utils::CalcMd5SumHash(bmDecoded.get(), hash, sizeof(hash));

		static const uint8_t expectedResult[16] = { 0x04,0x77,0x2f,0x32,0x2f,0x94,0x9b,0x07,0x0d,0x53,0xa5,0x24,0xea,0x64,0x5a,0x1a };
		EXPECT_TRUE(memcmp(hash, expectedResult, 16) == 0) << "Incorrect result";
}
