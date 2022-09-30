// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "pch.h"
#include "inc_libCZI.h"

using namespace libCZI;

TEST(StreamImplementations, StreamInMemory1)
{
	std::uint8_t* buffer = (std::uint8_t*)malloc(10);
	for (int i = 0; i < 10; ++i)
	{
		buffer[i] = i + 1;
	}

	std::shared_ptr<const void> spBuffer(buffer, [](const void* p)->void {free(const_cast<void*>(p)); });
	auto stream = CreateStreamFromMemory(spBuffer, 10);

	std::uint8_t* bufferForRead = (std::uint8_t*)malloc(10);
	memset(bufferForRead, 0, 10);
	std::unique_ptr<std::uint8_t, void(*)(std::uint8_t*)> upBufferForRead(bufferForRead, [](std::uint8_t* p)->void {free(p); });
	std::uint64_t bytesRead;

	stream->Read(0, bufferForRead, 10, &bytesRead);

	EXPECT_EQ(bytesRead, 10) << "incorrect number of bytes read";
	bool correct = memcmp(buffer, bufferForRead, 10) == 0;
	EXPECT_TRUE(correct) << "incorrect result";
}

TEST(StreamImplementations, StreamInMemory2)
{
	std::uint8_t* buffer = (std::uint8_t*)malloc(10);
	for (int i = 0; i < 10; ++i)
	{
		buffer[i] = i + 1;
	}

	std::shared_ptr<const void> spBuffer(buffer, [](const void* p)->void {free(const_cast<void*>(p)); });
	auto stream = CreateStreamFromMemory(spBuffer, 10);

	std::uint8_t* bufferForRead = (std::uint8_t*)malloc(15);
	memset(bufferForRead, 0, 15);
	std::unique_ptr<std::uint8_t, void(*)(std::uint8_t*)> upBufferForRead(bufferForRead, [](std::uint8_t* p)->void {free(p); });
	std::uint64_t bytesRead;

	stream->Read(0, bufferForRead, 15, &bytesRead);

	EXPECT_EQ(bytesRead, 10) << "incorrect number of bytes read";
	bool correct = memcmp(buffer, bufferForRead, 10) == 0;
	EXPECT_TRUE(correct) << "incorrect result";

	EXPECT_TRUE(bufferForRead[10] == 0 && bufferForRead[11] == 0 && bufferForRead[12] == 0 &&
		bufferForRead[13] == 0 && bufferForRead[14] == 0) << "incorrect result";
}

TEST(StreamImplementations, StreamInMemory3)
{
	std::uint8_t* buffer = (std::uint8_t*)malloc(10);
	for (int i = 0; i < 10; ++i)
	{
		buffer[i] = i + 1;
	}

	std::shared_ptr<const void> spBuffer(buffer, [](const void* p)->void {free(const_cast<void*>(p)); });
	auto stream = CreateStreamFromMemory(spBuffer, 10);

	std::uint8_t* bufferForRead = (std::uint8_t*)malloc(15);
	memset(bufferForRead, 0, 15);
	std::unique_ptr<std::uint8_t, void(*)(std::uint8_t*)> upBufferForRead(bufferForRead, [](std::uint8_t* p)->void {free(p); });
	std::uint64_t bytesRead;

	stream->Read(9, bufferForRead, 3, &bytesRead);

	EXPECT_EQ(bytesRead, 1) << "incorrect number of bytes read";
	bool correct = memcmp(buffer + 9, bufferForRead, 1) == 0;
	EXPECT_TRUE(correct) << "incorrect result";

	EXPECT_TRUE(bufferForRead[1] == 0 && bufferForRead[2] == 0) << "incorrect result";
}
