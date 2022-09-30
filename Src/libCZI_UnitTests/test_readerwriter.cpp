// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "pch.h"
#include "inc_libCZI.h"
#include "MemOutputStream.h"
#include "utils.h"
#include "MemInputOutputStream.h"
#include "SegmentWalker.h"

using namespace libCZI;
using namespace std;

static tuple<shared_ptr<void>, size_t> CreateTestCzi()
{
	auto writer = CreateCZIWriter();
	auto outStream = make_shared<CMemOutputStream>(0);

	auto spWriterInfo = make_shared<CCziWriterInfo >(
		GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } },
		CDimBounds{ { DimensionIndex::Z,0,10 },{ DimensionIndex::C,0,1 } },	// set a bounds for Z and C
		0, 5);	// set a bounds M : 0<=m<=5

	// reserve size in the subblockdirectory-segment for 50 subblocks (with max-size for coordinate)
	spWriterInfo->SetReservedSizeForSubBlockDirectory(true, 50);

	writer->Create(outStream, spWriterInfo);

	auto bitmap = CreateTestBitmap(PixelType::Gray8, 4, 4);

	ScopedBitmapLockerSP lockBm{ bitmap };
	AddSubBlockInfoStridedBitmap addSbBlkInfo;

	for (int z = 0; z < 10; ++z)
	{
		for (int c = 0; c < 1; ++c)
		{
			for (int m = 0; m < 5; ++m)
			{
				addSbBlkInfo.Clear();
				addSbBlkInfo.coordinate.Set(DimensionIndex::C, c);
				addSbBlkInfo.coordinate.Set(DimensionIndex::Z, z);
				addSbBlkInfo.mIndexValid = true;
				addSbBlkInfo.mIndex = m;
				addSbBlkInfo.x = m * bitmap->GetWidth();
				addSbBlkInfo.y = 0;
				addSbBlkInfo.logicalWidth = bitmap->GetWidth();
				addSbBlkInfo.logicalHeight = bitmap->GetHeight();
				addSbBlkInfo.physicalWidth = bitmap->GetWidth();
				addSbBlkInfo.physicalHeight = bitmap->GetHeight();
				addSbBlkInfo.PixelType = bitmap->GetPixelType();
				addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
				addSbBlkInfo.strideBitmap = lockBm.stride;

				writer->SyncAddSubBlock(addSbBlkInfo);
			}
		}
	}

	PrepareMetadataInfo prepare_metadata_info;
	auto metaDataBuilder = writer->GetPreparedMetadata(prepare_metadata_info);

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

static tuple<shared_ptr<void>, size_t> CreateTestCzi2()
{
	auto writer = CreateCZIWriter();
	auto outStream = make_shared<CMemOutputStream>(0);

	auto spWriterInfo = make_shared<CCziWriterInfo >(
		GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } },
		CDimBounds{ { DimensionIndex::Z,0,10 },{ DimensionIndex::C,0,1 } },	// set a bounds for Z and C
		0, 5);	// set a bounds M : 0<=m<=5

				// reserve size in the subblockdirectory-segment for 50 subblocks (with max-size for coordinate)
	spWriterInfo->SetReservedSizeForSubBlockDirectory(true, 50);

	writer->Create(outStream, spWriterInfo);

	auto bitmap = CreateTestBitmap(PixelType::Gray8, 4, 4);

	ScopedBitmapLockerSP lockBm{ bitmap };
	AddSubBlockInfoStridedBitmap addSbBlkInfo;

	for (int z = 0; z < 10; ++z)
	{
		for (int c = 0; c < 1; ++c)
		{
			for (int m = 0; m < 5; ++m)
			{
				addSbBlkInfo.Clear();
				addSbBlkInfo.coordinate.Set(DimensionIndex::C, c);
				addSbBlkInfo.coordinate.Set(DimensionIndex::Z, z);
				addSbBlkInfo.mIndexValid = true;
				addSbBlkInfo.mIndex = m;
				addSbBlkInfo.x = m * bitmap->GetWidth();
				addSbBlkInfo.y = 0;
				addSbBlkInfo.logicalWidth = bitmap->GetWidth();
				addSbBlkInfo.logicalHeight = bitmap->GetHeight();
				addSbBlkInfo.physicalWidth = bitmap->GetWidth();
				addSbBlkInfo.physicalHeight = bitmap->GetHeight();
				addSbBlkInfo.PixelType = bitmap->GetPixelType();
				addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
				addSbBlkInfo.strideBitmap = lockBm.stride;

				writer->SyncAddSubBlock(addSbBlkInfo);
			}
		}
	}

	const size_t SIZEATTACHMENT = 1000;
	AddAttachmentInfo addAttachmentInfo;
	addAttachmentInfo.contentGuid = GUID{ 0x1111111,0x2222,0x3333,{ 4,5,6,7,8,9,0xa,0xb } };
	addAttachmentInfo.SetName("ATTACHMENT1");
	addAttachmentInfo.SetContentFileType("TYPE1");
	std::unique_ptr<uint8_t, decltype(free)*> upBuffer(static_cast<uint8_t*>(malloc(SIZEATTACHMENT)), free);
	for (size_t i = 0; i < SIZEATTACHMENT; ++i)
	{
		*(upBuffer.get() + i) = static_cast<uint8_t>(i);
	}

	addAttachmentInfo.ptrData = upBuffer.get();
	addAttachmentInfo.dataSize = SIZEATTACHMENT;
	writer->SyncAddAttachment(addAttachmentInfo);

	PrepareMetadataInfo prepare_metadata_info;
	auto metaDataBuilder = writer->GetPreparedMetadata(prepare_metadata_info);

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

static tuple<shared_ptr<void>, size_t> CreateTestCzi3()
{
	auto writer = CreateCZIWriter();
	auto outStream = make_shared<CMemOutputStream>(0);

	auto spWriterInfo = make_shared<CCziWriterInfo >(
		GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } },
		CDimBounds{ { DimensionIndex::C,0,3 } },	// set a bounds for C
		0, 5);	// set a bounds M : 0<=m<=5

				// reserve size in the subblockdirectory-segment for 50 subblocks (with max-size for coordinate)
	spWriterInfo->SetReservedSizeForSubBlockDirectory(true, 4);

	writer->Create(outStream, spWriterInfo);

	auto bitmap = CreateTestBitmap(PixelType::Gray8, 512, 512);

	ScopedBitmapLockerSP lockBm{ bitmap };
	AddSubBlockInfoStridedBitmap addSbBlkInfo;

	for (int c = 0; c < 3; ++c)
	{
		addSbBlkInfo.Clear();
		addSbBlkInfo.coordinate.Set(DimensionIndex::C, c);
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
	}

	const size_t SIZEATTACHMENT = 1000;
	AddAttachmentInfo addAttachmentInfo;
	addAttachmentInfo.contentGuid = GUID{ 0x1111111,0x2222,0x3333,{ 4,5,6,7,8,9,0xa,0xb } };
	addAttachmentInfo.SetName("ATTACHMENT1");
	addAttachmentInfo.SetContentFileType("TYPE1");
	std::unique_ptr<uint8_t, decltype(free)*> upBuffer(static_cast<uint8_t*>(malloc(SIZEATTACHMENT)), free);
	for (size_t i = 0; i < SIZEATTACHMENT; ++i)
	{
		*(upBuffer.get() + i) = static_cast<uint8_t>(i);
	}

	addAttachmentInfo.ptrData = upBuffer.get();
	addAttachmentInfo.dataSize = SIZEATTACHMENT;
	writer->SyncAddAttachment(addAttachmentInfo);

	PrepareMetadataInfo prepare_metadata_info;
	auto metaDataBuilder = writer->GetPreparedMetadata(prepare_metadata_info);

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

TEST(CziReaderWriter, ReaderWriter1)
{
	auto testCzi = CreateTestCzi();
	/*
	FILE* fp = fopen("l:\\temp\\x\\test.czi", "wb");
	fwrite(get<0>(testCzi).get(), get<1>(testCzi), 1, fp);
	fclose(fp);
	*/

	auto inOutStream = make_shared<CMemInputOutputStream>(get<0>(testCzi).get(), get<1>(testCzi));
	auto rw = CreateCZIReaderWriter();
	rw->Create(inOutStream);

	auto sb0 = rw->ReadSubBlock(0);

	AddSubBlockInfoMemPtr addSbInfo;
	addSbInfo.coordinate = sb0->GetSubBlockInfo().coordinate;
	addSbInfo.mIndexValid = sb0->GetSubBlockInfo().mIndex != std::numeric_limits<int>::max();
	addSbInfo.mIndex = sb0->GetSubBlockInfo().mIndex;
	addSbInfo.x = sb0->GetSubBlockInfo().logicalRect.x;
	addSbInfo.y = sb0->GetSubBlockInfo().logicalRect.y;
	addSbInfo.logicalWidth = sb0->GetSubBlockInfo().logicalRect.w;
	addSbInfo.logicalHeight = sb0->GetSubBlockInfo().logicalRect.h;
	addSbInfo.physicalWidth = sb0->GetSubBlockInfo().physicalSize.w;
	addSbInfo.physicalHeight = sb0->GetSubBlockInfo().physicalSize.h;
	addSbInfo.PixelType = sb0->GetSubBlockInfo().pixelType;

	size_t sizeSbblkData;
	auto sblkdata = sb0->GetRawData(ISubBlock::MemBlkType::Data, &sizeSbblkData);
	std::unique_ptr<void, decltype(free)*> pBuffer(malloc(sizeSbblkData), free);
	memcpy(pBuffer.get(), sblkdata.get(), sizeSbblkData);
	for (size_t i = 0; i < sizeSbblkData; ++i)
	{
		++*(static_cast<uint8_t*>(pBuffer.get()) + i);
	}

	addSbInfo.ptrData = pBuffer.get();
	addSbInfo.dataSize = (uint32_t)sizeSbblkData;

	rw->ReplaceSubBlock(0, addSbInfo);

	rw->Close();
	rw.reset();

	int callCnt = 0;
	int indexOfModifiedSbBlk = -1;
	auto spReader = libCZI::CreateCZIReader();
	spReader->Open(inOutStream);
	spReader->EnumSubset(&sb0->GetSubBlockInfo().coordinate, nullptr, false,
		[&](int index, const SubBlockInfo& info)->bool
	{
		if (info.mIndex == sb0->GetSubBlockInfo().mIndex)
		{
			indexOfModifiedSbBlk = index;
			++callCnt;
		}
		return true;
	});

	EXPECT_TRUE(callCnt == 1 && indexOfModifiedSbBlk >= 0) << "not the expected result";

	auto readSbBlk = spReader->ReadSubBlock(indexOfModifiedSbBlk);

	size_t sizeReadBackSbBlk;
	auto readBackSbBlk = readSbBlk->GetRawData(ISubBlock::Data, &sizeReadBackSbBlk);
	EXPECT_TRUE(sizeReadBackSbBlk == sizeSbblkData) << "not the expected result";

	for (size_t i = 0; i < sizeSbblkData; ++i)
	{
		const uint8_t* ptr = static_cast<const uint8_t*>(readBackSbBlk.get()) + i;
		EXPECT_TRUE(*ptr == i + 1) << "not the expected result";
	}

	WriteOutTestCzi("CziReaderWriter", "ReaderWriter1", inOutStream);
}

TEST(CziReaderWriter, ReaderWriter2)
{
	auto testCzi = CreateTestCzi();

	auto inOutStream = make_shared<CMemInputOutputStream>(get<0>(testCzi).get(), get<1>(testCzi));
	auto rw = CreateCZIReaderWriter();
	rw->Create(inOutStream);

	auto sb0 = rw->ReadSubBlock(0);
	size_t sizeSbblkData;
	auto sblkdata = sb0->GetRawData(ISubBlock::MemBlkType::Data, &sizeSbblkData);

	// replace subblock #0 with a new one which is four times the size (so that the existing one gets deleted and a new one appended)
	AddSubBlockInfo addSbInfo;
	addSbInfo.coordinate = sb0->GetSubBlockInfo().coordinate;
	addSbInfo.mIndexValid = sb0->GetSubBlockInfo().mIndex != std::numeric_limits<int>::max();
	addSbInfo.mIndex = sb0->GetSubBlockInfo().mIndex;
	addSbInfo.x = sb0->GetSubBlockInfo().logicalRect.x;
	addSbInfo.y = sb0->GetSubBlockInfo().logicalRect.y;
	addSbInfo.logicalWidth = sb0->GetSubBlockInfo().logicalRect.w;
	addSbInfo.logicalHeight = sb0->GetSubBlockInfo().logicalRect.h * 4;
	addSbInfo.physicalWidth = sb0->GetSubBlockInfo().physicalSize.w;
	addSbInfo.physicalHeight = sb0->GetSubBlockInfo().physicalSize.h * 4;
	addSbInfo.PixelType = sb0->GetSubBlockInfo().pixelType;

	addSbInfo.sizeData = sizeSbblkData * 4;
	addSbInfo.getData = [&](int callCnt, size_t offset, const void*& ptr, size_t& size)->bool
	{
		// repeat every byte four times
		auto byteNo = callCnt / 4;
		if (size_t(byteNo) < sizeSbblkData)
		{
			ptr = ((const uint8_t*)sblkdata.get()) + byteNo;
			size = 1;
			return true;
		}

		return false;
	};

	rw->ReplaceSubBlock(0, addSbInfo);

	rw->Close();
	rw.reset();

	bool success = true;
	int subBlkCnt = 0;
	bool allReceived = false;
	CSegmentWalker::Walk(
		inOutStream.get(),
		[&](int cnt, const std::string& id, std::int64_t allocatedSize, std::int64_t usedSize)->bool
	{
		// we expect "FILEHEADER", "SUBBLKDIR", "DELETED", 49 x "SUBBLK", "METADATA", "SUBBLK"
		if (cnt == 0)
		{
			if (id != "ZISRAWFILE")
			{
				success = false; return false;
			}

			return true;
		}
		else if (cnt == 1)
		{
			if (id != "ZISRAWDIRECTORY")
			{
				success = false; return false;
			}

			return true;
		}
		else if (cnt == 2)
		{
			if (id != "DELETED")
			{
				success = false; return false;
			}

			return true;
		}
		else if (cnt <= 51)
		{
			if (id != "ZISRAWSUBBLOCK")
			{
				success = false; return false;
			}

			subBlkCnt++;
			return true;
		}
		else if (cnt == 52)
		{
			if (id != "ZISRAWMETADATA")
			{
				success = false;  return false;
			}

			return true;
		}
		else if (cnt == 53)
		{
			if (id != "ZISRAWSUBBLOCK")
			{
				success = false;  return false;
			}

			subBlkCnt++;
			allReceived = true;
			return true;
		}

		success = false;
		return false;
	});

	EXPECT_TRUE(success && allReceived) << "did not behave as expected";
	EXPECT_EQ(subBlkCnt, 50) << "did not behave as expected";
}

TEST(CziReaderWriter, ReaderWriter3)
{
	auto testCzi = CreateTestCzi2();

	auto inOutStream = make_shared<CMemInputOutputStream>(get<0>(testCzi).get(), get<1>(testCzi));
	auto rw = CreateCZIReaderWriter();
	rw->Create(inOutStream);

	int idxAttachment = -1;
	rw->EnumerateAttachments(
		[&](int index, const AttachmentInfo& info)->bool
	{
		if (strcmp(info.contentFileType, "TYPE1") == 0 && info.name == "ATTACHMENT1")
		{
			idxAttachment = index;
			return false;
		}

		return true;
	});

	EXPECT_TRUE(idxAttachment >= 0) << "did not behave as expected";

	auto attchmnt = rw->ReadAttachment(idxAttachment);
	const void* ptrAttchmnt; size_t sizeAttchmnt;
	attchmnt->DangerousGetRawData(ptrAttchmnt, sizeAttchmnt);

	std::unique_ptr<uint8_t, decltype(free)*> upBuffer((uint8_t*)malloc(sizeAttchmnt * 2), free);
	memcpy(upBuffer.get(), ptrAttchmnt, sizeAttchmnt);
	memcpy(upBuffer.get() + sizeAttchmnt, ptrAttchmnt, sizeAttchmnt);

	AddAttachmentInfo addAttachmentInfo;
	addAttachmentInfo.contentGuid = attchmnt->GetAttachmentInfo().contentGuid;
	addAttachmentInfo.SetContentFileType(attchmnt->GetAttachmentInfo().contentFileType);
	addAttachmentInfo.SetName(attchmnt->GetAttachmentInfo().name.c_str());
	addAttachmentInfo.ptrData = upBuffer.get();
	addAttachmentInfo.dataSize = uint32_t(sizeAttchmnt * 2);

	// the new attachment is twice the size of the existing one, so expect it to be added at the end
	rw->ReplaceAttachment(idxAttachment, addAttachmentInfo);

	rw->Close();


	bool success = true;
	int subBlkCnt = 0;
	bool allReceived = false;
	CSegmentWalker::Walk(
		inOutStream.get(),
		[&](int cnt, const std::string& id, std::int64_t allocatedSize, std::int64_t usedSize)->bool
	{
		// we expect "FILEHEADER", "SUBBLKDIR", 50 x "SUBBLK", "DELETED", "METADATA", "ATTACHMENTDIR", "ATTACHMENT"
		if (cnt == 0)
		{
			if (id != "ZISRAWFILE")
			{
				success = false; return false;
			}

			return true;
		}
		else if (cnt == 1)
		{
			if (id != "ZISRAWDIRECTORY")
			{
				success = false; return false;
			}

			return true;
		}
		else if (cnt <= 51)
		{
			if (id != "ZISRAWSUBBLOCK")
			{
				success = false; return false;
			}

			subBlkCnt++;
			return true;
		}
		else if (cnt == 52)
		{
			if (id != "DELETED")
			{
				success = false;  return false;
			}

			return true;
		}
		else if (cnt == 53)
		{
			if (id != "ZISRAWMETADATA")
			{
				success = false;  return false;
			}

			return true;
		}
		else if (cnt == 54)
		{
			if (id != "ZISRAWATTDIR")
			{
				success = false;  return false;
			}

			return true;
		}
		else if (cnt == 55)
		{
			if (id != "ZISRAWATTACH")
			{
				success = false;  return false;
			}

			allReceived = true;
			return true;
		}

		success = false;
		return false;
	});

	allReceived = true;

	EXPECT_TRUE(success && allReceived) << "did not behave as expected";
	EXPECT_EQ(subBlkCnt, 50) << "did not behave as expected";

	WriteOutTestCzi("CziReaderWriter", "ReaderWriter3", inOutStream);
}

TEST(CziReaderWriter, ReaderWriter4)
{
	auto testCzi = CreateTestCzi2();

	auto inOutStream = make_shared<CMemInputOutputStream>(get<0>(testCzi).get(), get<1>(testCzi));
	auto rw = CreateCZIReaderWriter();
	rw->Create(inOutStream);

	int idxAttachment = -1;
	rw->EnumerateAttachments(
		[&](int index, const AttachmentInfo& info)->bool
	{
		if (strcmp(info.contentFileType, "TYPE1") == 0 && info.name == "ATTACHMENT1")
		{
			idxAttachment = index;
			return false;
		}

		return true;
	});

	EXPECT_TRUE(idxAttachment >= 0) << "did not behave as expected";

	auto attchmnt = rw->ReadAttachment(idxAttachment);
	const void* ptrAttchmnt; size_t sizeAttchmnt;
	attchmnt->DangerousGetRawData(ptrAttchmnt, sizeAttchmnt);

	std::unique_ptr<uint8_t, decltype(free)*> upBuffer(static_cast<uint8_t*>(malloc(sizeAttchmnt)), free);
	memcpy(upBuffer.get(), ptrAttchmnt, sizeAttchmnt);
	for (size_t i = 0; i < sizeAttchmnt; ++i)
	{
		*(upBuffer.get() + i) *= 2;
	}

	AddAttachmentInfo addAttachmentInfo;
	addAttachmentInfo.contentGuid = attchmnt->GetAttachmentInfo().contentGuid;
	addAttachmentInfo.SetContentFileType(attchmnt->GetAttachmentInfo().contentFileType);
	addAttachmentInfo.SetName(attchmnt->GetAttachmentInfo().name.c_str());
	addAttachmentInfo.ptrData = upBuffer.get();
	addAttachmentInfo.dataSize = uint32_t(sizeAttchmnt);

	rw->ReplaceAttachment(idxAttachment, addAttachmentInfo);

	rw->Close();

	CSegmentWalker::ExpectedSegment expectedSegments[] =
	{
		{1,"ZISRAWFILE"},
		{1,"ZISRAWDIRECTORY"},
		{50,"ZISRAWSUBBLOCK"},
		{1,"ZISRAWATTACH"},
		{1,"ZISRAWMETADATA"},
		{1,"ZISRAWATTDIR"}
	};

	bool success = CSegmentWalker::CheckSegments(inOutStream.get(), expectedSegments, sizeof(expectedSegments) / sizeof(expectedSegments[0]));

	EXPECT_TRUE(success) << "did not behave as expected";

	int indexOfModifiedAttchmnt = -1;
	auto spReader = libCZI::CreateCZIReader();
	spReader->Open(inOutStream);
	spReader->EnumerateAttachments(
		[&](int index, const AttachmentInfo& info)->bool
	{
		if (strcmp(info.contentFileType, attchmnt->GetAttachmentInfo().contentFileType) == 0)
		{
			indexOfModifiedAttchmnt = index;
		}

		return true;
	});

	EXPECT_TRUE(indexOfModifiedAttchmnt >= 0) << "not the expected result";

	auto attchmntReadBack = spReader->ReadAttachment(indexOfModifiedAttchmnt);

	size_t sizeAttchmntReadBackData;
	auto attchmntReadBackData = attchmntReadBack->GetRawData(&sizeAttchmntReadBackData);
	EXPECT_TRUE(sizeAttchmntReadBackData == addAttachmentInfo.dataSize) << "not the expected result";

	for (size_t i = 0; i < sizeAttchmntReadBackData; ++i)
	{
		uint8_t v = static_cast<uint8_t>(i) * 2;
		EXPECT_EQ(v, *(static_cast<const uint8_t*>(attchmntReadBackData.get()) + i)) << "not the expected result";
	}

	WriteOutTestCzi("CziReaderWriter", "ReaderWriter4", inOutStream);
}

TEST(CziReaderWriter, ReaderWriter5)
{
	auto testCzi = CreateTestCzi2();

	auto inOutStream = make_shared<CMemInputOutputStream>(get<0>(testCzi).get(), get<1>(testCzi));
	auto rw = CreateCZIReaderWriter();
	rw->Create(inOutStream);

	rw->RemoveSubBlock(0);

	CSegmentWalker::ExpectedSegment expectedSegments[] =
	{
	{ 1,"ZISRAWFILE" },
	{ 1,"ZISRAWDIRECTORY" },
	{ 1,"DELETED" },
	{ 49,"ZISRAWSUBBLOCK" },
	{ 1,"ZISRAWATTACH" },
	{ 1,"ZISRAWMETADATA" },
	{ 1,"ZISRAWATTDIR" }
	};

	bool success = CSegmentWalker::CheckSegments(inOutStream.get(), expectedSegments, sizeof(expectedSegments) / sizeof(expectedSegments[0]));
	EXPECT_TRUE(success) << "not the expected result";
}

TEST(CziReaderWriter, ReaderWriter6)
{
	auto testCzi = CreateTestCzi2();

	auto inOutStream = make_shared<CMemInputOutputStream>(get<0>(testCzi).get(), get<1>(testCzi));
	auto rw = CreateCZIReaderWriter();
	rw->Create(inOutStream);

	auto bitmap = CreateTestBitmap(PixelType::Gray8, 4, 4);

	ScopedBitmapLockerSP lockBm{ bitmap };
	AddSubBlockInfoStridedBitmap addSbBlkInfo;

	addSbBlkInfo.Clear();
	addSbBlkInfo.coordinate.Set(DimensionIndex::C, 0);
	addSbBlkInfo.coordinate.Set(DimensionIndex::Z, 10);
	addSbBlkInfo.mIndexValid = true;
	addSbBlkInfo.mIndex = 0;
	addSbBlkInfo.x = 0 * bitmap->GetWidth();
	addSbBlkInfo.y = 0;
	addSbBlkInfo.logicalWidth = bitmap->GetWidth();
	addSbBlkInfo.logicalHeight = bitmap->GetHeight();
	addSbBlkInfo.physicalWidth = bitmap->GetWidth();
	addSbBlkInfo.physicalHeight = bitmap->GetHeight();
	addSbBlkInfo.PixelType = bitmap->GetPixelType();
	addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
	addSbBlkInfo.strideBitmap = lockBm.stride;
	rw->SyncAddSubBlock(addSbBlkInfo);

	addSbBlkInfo.Clear();
	addSbBlkInfo.coordinate.Set(DimensionIndex::C, 1);
	addSbBlkInfo.coordinate.Set(DimensionIndex::Z, 10);
	addSbBlkInfo.mIndexValid = true;
	addSbBlkInfo.mIndex = 0;
	addSbBlkInfo.x = 0 * bitmap->GetWidth();
	addSbBlkInfo.y = 0;
	addSbBlkInfo.logicalWidth = bitmap->GetWidth();
	addSbBlkInfo.logicalHeight = bitmap->GetHeight();
	addSbBlkInfo.physicalWidth = bitmap->GetWidth();
	addSbBlkInfo.physicalHeight = bitmap->GetHeight();
	addSbBlkInfo.PixelType = bitmap->GetPixelType();
	addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
	addSbBlkInfo.strideBitmap = lockBm.stride;
	rw->SyncAddSubBlock(addSbBlkInfo);

	rw->Close();
	rw.reset();

	WriteOutTestCzi("CziReaderWriter", "ReaderWriter6", inOutStream);

	auto spReader = libCZI::CreateCZIReader();
	spReader->Open(inOutStream);
	auto statistics = spReader->GetStatistics();
	EXPECT_TRUE(statistics.subBlockCount == 52) << "not the expected result";
	int zStart, zSize;
	bool b = statistics.dimBounds.TryGetInterval(DimensionIndex::Z, &zStart, &zSize);
	EXPECT_TRUE(b == true && zStart == 0 && zSize == 11) << "not the expected result";
}

TEST(CziReaderWriter, ReaderWriter7)
{
	auto testCzi = CreateTestCzi3();

	auto inOutStream = make_shared<CMemInputOutputStream>(get<0>(testCzi).get(), get<1>(testCzi));
	auto rw = CreateCZIReaderWriter();
	rw->Create(inOutStream);

	auto bitmap = CreateTestBitmap(PixelType::Gray8, 512, 512);

	ScopedBitmapLockerSP lockBm{ bitmap };
	AddSubBlockInfoStridedBitmap addSbBlkInfo;

	addSbBlkInfo.Clear();
	addSbBlkInfo.coordinate.Set(DimensionIndex::C, 0);
	addSbBlkInfo.mIndexValid = true;
	addSbBlkInfo.mIndex = 0;
	addSbBlkInfo.x = 0 * bitmap->GetWidth();
	addSbBlkInfo.y = 0;
	addSbBlkInfo.logicalWidth = bitmap->GetWidth();
	addSbBlkInfo.logicalHeight = bitmap->GetHeight();
	addSbBlkInfo.physicalWidth = bitmap->GetWidth();
	addSbBlkInfo.physicalHeight = bitmap->GetHeight();
	addSbBlkInfo.PixelType = bitmap->GetPixelType();
	addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
	addSbBlkInfo.strideBitmap = lockBm.stride;

	bool expectedExceptionCaught = false;
	try
	{
		rw->SyncAddSubBlock(addSbBlkInfo);
	}
	catch (LibCZIReaderWriteException& excp)
	{
		if (excp.GetErrorType() == LibCZIReaderWriteException::ErrorType::AddCoordinateAlreadyExisting)
		{
			expectedExceptionCaught = true;
		}
	}

	EXPECT_TRUE(expectedExceptionCaught == true) << "did not see the expected exception";
}

TEST(CziReaderWriter, ReaderWriter8)
{
	auto testCzi = CreateTestCzi3();

	auto inOutStream = make_shared<CMemInputOutputStream>(get<0>(testCzi).get(), get<1>(testCzi));
	auto rw = CreateCZIReaderWriter();
	rw->Create(inOutStream);

	const size_t SIZEATTACHMENT = 1000;
	AddAttachmentInfo addAttachmentInfo;
	addAttachmentInfo.contentGuid = GUID{ 0x1234567,0x2222,0x3333,{ 4,5,6,7,8,9,0xa,0xb } };
	addAttachmentInfo.SetName("ATTACHMENT1");
	addAttachmentInfo.SetContentFileType("TYPE1");
	std::unique_ptr<uint8_t, decltype(free)*> upBuffer(static_cast<uint8_t*>(malloc(SIZEATTACHMENT)), free);
	for (size_t i = 0; i < SIZEATTACHMENT; ++i)
	{
		*(upBuffer.get() + i) = static_cast<uint8_t>(i);
	}

	addAttachmentInfo.ptrData = upBuffer.get();
	addAttachmentInfo.dataSize = SIZEATTACHMENT;
	rw->SyncAddAttachment(addAttachmentInfo);

	rw->Close();
	rw.reset();

	WriteOutTestCzi("CziReaderWriter", "ReaderWriter8", inOutStream);

	CSegmentWalker::ExpectedSegment expectedSegments[] =
	{
		{ 1,"ZISRAWFILE" },
		{ 1,"ZISRAWDIRECTORY" },
		{ 3,"ZISRAWSUBBLOCK" },
		{ 1,"ZISRAWATTACH" },
		{ 1,"ZISRAWMETADATA" },
		{ 1,"DELETED" },
		{ 1,"ZISRAWATTACH" },
		{ 1,"ZISRAWATTDIR" }
	};

	bool success = CSegmentWalker::CheckSegments(inOutStream.get(), expectedSegments, sizeof(expectedSegments) / sizeof(expectedSegments[0]));
	EXPECT_TRUE(success) << "not the expected result";
}

TEST(CziReaderWriter, ReaderWriter9)
{
	auto testCzi = CreateTestCzi3();

	auto inOutStream = make_shared<CMemInputOutputStream>(get<0>(testCzi).get(), get<1>(testCzi));
	auto rw = CreateCZIReaderWriter();
	rw->Create(inOutStream);

	auto bitmap = CreateTestBitmap(PixelType::Gray8, 512, 512);

	ScopedBitmapLockerSP lockBm{ bitmap };
	AddSubBlockInfoStridedBitmap addSbBlkInfo;

	addSbBlkInfo.Clear();
	addSbBlkInfo.coordinate.Set(DimensionIndex::C, 3);
	addSbBlkInfo.mIndexValid = true;
	addSbBlkInfo.mIndex = 0;
	addSbBlkInfo.x = 0 * bitmap->GetWidth();
	addSbBlkInfo.y = 0;
	addSbBlkInfo.logicalWidth = bitmap->GetWidth();
	addSbBlkInfo.logicalHeight = bitmap->GetHeight();
	addSbBlkInfo.physicalWidth = bitmap->GetWidth();
	addSbBlkInfo.physicalHeight = bitmap->GetHeight();
	addSbBlkInfo.PixelType = bitmap->GetPixelType();
	addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
	addSbBlkInfo.strideBitmap = lockBm.stride;

	rw->SyncAddSubBlock(addSbBlkInfo);

	const size_t SIZEATTACHMENT = 1000;
	AddAttachmentInfo addAttachmentInfo;
	addAttachmentInfo.contentGuid = GUID{ 0x1234567,0x2222,0x3333,{ 4,5,6,7,8,9,0xa,0xb } };
	addAttachmentInfo.SetName("ATTACHMENT1");
	addAttachmentInfo.SetContentFileType("TYPE1");
	std::unique_ptr<uint8_t, decltype(free)*> upBuffer(static_cast<uint8_t*>(malloc(SIZEATTACHMENT)), free);
	for (size_t i = 0; i < SIZEATTACHMENT; ++i)
	{
		*(upBuffer.get() + i) = static_cast<uint8_t>(i);
	}

	addAttachmentInfo.ptrData = upBuffer.get();
	addAttachmentInfo.dataSize = SIZEATTACHMENT;
	rw->SyncAddAttachment(addAttachmentInfo);

	rw->Close();
	rw.reset();

	WriteOutTestCzi("CziReaderWriter", "ReaderWriter9", inOutStream);

	CSegmentWalker::ExpectedSegment expectedSegments[] =
	{
		{ 1,"ZISRAWFILE" },
		{ 1,"ZISRAWDIRECTORY" },
		{ 3,"ZISRAWSUBBLOCK" },
		{ 1,"ZISRAWATTACH" },
		{ 1,"ZISRAWMETADATA" },
		{ 1,"DELETED" },
		{ 1,"ZISRAWSUBBLOCK" },
		{ 1,"ZISRAWATTACH" },
		{ 1,"ZISRAWATTDIR" }
	};

	bool success = CSegmentWalker::CheckSegments(inOutStream.get(), expectedSegments, sizeof(expectedSegments) / sizeof(expectedSegments[0]));
	EXPECT_TRUE(success) << "not the expected result";
}

TEST(CziReaderWriter, ReaderWriter10)
{
	auto testCzi = CreateTestCzi3();

	auto inOutStream = make_shared<CMemInputOutputStream>(get<0>(testCzi).get(), get<1>(testCzi));
	auto rw = CreateCZIReaderWriter();
	rw->Create(inOutStream);

	auto metadataSegment = rw->ReadMetadataSegment();
	auto metadata = metadataSegment->CreateMetaFromMetadataSegment();

	string xml = metadata->GetXml();

	xml += "                                    ";

	WriteMetadataInfo writeMetadataInfo;
	writeMetadataInfo.Clear();
	writeMetadataInfo.szMetadata = xml.c_str();
	writeMetadataInfo.szMetadataSize = xml.size();

	rw->SyncWriteMetadata(writeMetadataInfo);

	rw->Close();
	rw.reset();

	WriteOutTestCzi("CziReaderWriter", "ReaderWriter10", inOutStream);

	CSegmentWalker::ExpectedSegment expectedSegments[] =
	{
		{ 1,"ZISRAWFILE" },
		{ 1,"ZISRAWDIRECTORY" },
		{ 3,"ZISRAWSUBBLOCK" },
		{ 1,"ZISRAWATTACH" },
		{ 1,"DELETED" },
		{ 1,"ZISRAWATTDIR" },
		{ 1,"ZISRAWMETADATA" }
	};

	bool success = CSegmentWalker::CheckSegments(inOutStream.get(), expectedSegments, sizeof(expectedSegments) / sizeof(expectedSegments[0]));
	EXPECT_TRUE(success) << "not the expected result";
}

TEST(CziReaderWriter, ReaderWriter11)
{
	auto testCzi = CreateTestCzi3();

	auto inOutStream = make_shared<CMemInputOutputStream>(get<0>(testCzi).get(), get<1>(testCzi));
	auto rw = CreateCZIReaderWriter();
	rw->Create(inOutStream);

	auto sbBlkStatistics1 = rw->GetStatistics();
	EXPECT_EQ(sbBlkStatistics1.subBlockCount, 3) << "did not behave as expected";
	EXPECT_TRUE(sbBlkStatistics1.boundingBox.x == 0 && sbBlkStatistics1.boundingBox.y == 0 && sbBlkStatistics1.boundingBox.w == 0x200 && sbBlkStatistics1.boundingBox.h == 0x200) << "did not behave as expected";
	int cStart, cSize;
	EXPECT_TRUE(sbBlkStatistics1.dimBounds.TryGetInterval(DimensionIndex::C, &cStart, &cSize)) << "did not behave as expected";
	EXPECT_EQ(cStart, 0) << "did not behave as expected";
	EXPECT_EQ(cSize, 3) << "did not behave as expected";

	auto bitmap = CreateTestBitmap(PixelType::Gray8, 513, 513);

	ScopedBitmapLockerSP lockBm{ bitmap };
	AddSubBlockInfoStridedBitmap addSbBlkInfo;

	addSbBlkInfo.Clear();
	addSbBlkInfo.coordinate.Set(DimensionIndex::C, 3);
	addSbBlkInfo.mIndexValid = true;
	addSbBlkInfo.mIndex = 0;
	addSbBlkInfo.x = 0 * bitmap->GetWidth();
	addSbBlkInfo.y = 0;
	addSbBlkInfo.logicalWidth = bitmap->GetWidth();
	addSbBlkInfo.logicalHeight = bitmap->GetHeight();
	addSbBlkInfo.physicalWidth = bitmap->GetWidth();
	addSbBlkInfo.physicalHeight = bitmap->GetHeight();
	addSbBlkInfo.PixelType = bitmap->GetPixelType();
	addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
	addSbBlkInfo.strideBitmap = lockBm.stride;

	rw->SyncAddSubBlock(addSbBlkInfo);

	auto sbBlkStatistics2 = rw->GetStatistics();

	EXPECT_EQ(sbBlkStatistics2.subBlockCount, 4) << "did not behave as expected";
	EXPECT_TRUE(sbBlkStatistics2.boundingBox.x == 0 && sbBlkStatistics2.boundingBox.y == 0 && sbBlkStatistics2.boundingBox.w == 0x201 && sbBlkStatistics2.boundingBox.h == 0x201) << "did not behave as expected";
	EXPECT_TRUE(sbBlkStatistics2.dimBounds.TryGetInterval(DimensionIndex::C, &cStart, &cSize)) << "did not behave as expected";
	EXPECT_EQ(cStart, 0) << "did not behave as expected";
	EXPECT_EQ(cSize, 4) << "did not behave as expected";
}

static bool CheckNotOperationalException(function<void()> func)
{
	try
	{
		func();
	}
	catch (logic_error&)
	{
		return true;
	}

	return false;
}

TEST(CziReaderWriter, ReaderWriter12)
{
	auto rw = CreateCZIReaderWriter();

	EXPECT_TRUE(CheckNotOperationalException([&]()->void {AddSubBlockInfo info; rw->ReplaceSubBlock(0, info); })) << "incorrect behavior";
	EXPECT_TRUE(CheckNotOperationalException([&]()->void {rw->RemoveSubBlock(0); })) << "incorrect behavior";
	EXPECT_TRUE(CheckNotOperationalException([&]()->void {AddAttachmentInfo info; rw->ReplaceAttachment(0, info); })) << "incorrect behavior";
	EXPECT_TRUE(CheckNotOperationalException([&]()->void {rw->RemoveAttachment(0); })) << "incorrect behavior";
	EXPECT_TRUE(CheckNotOperationalException([&]()->void {AddSubBlockInfo info; rw->SyncAddSubBlock(info); })) << "incorrect behavior";
	EXPECT_TRUE(CheckNotOperationalException([&]()->void {AddAttachmentInfo info; rw->SyncAddAttachment(info); })) << "incorrect behavior";
	EXPECT_TRUE(CheckNotOperationalException([&]()->void {WriteMetadataInfo info; rw->SyncWriteMetadata(info); })) << "incorrect behavior";
	EXPECT_TRUE(CheckNotOperationalException([&]()->void {rw->ReadMetadataSegment(); })) << "incorrect behavior";
	EXPECT_TRUE(CheckNotOperationalException([&]()->void {rw->Close(); })) << "incorrect behavior";

	EXPECT_TRUE(CheckNotOperationalException([&]()->void {rw->EnumerateSubBlocks([](int i, const SubBlockInfo& info)->bool {return true; }); })) << "incorrect behavior";
	EXPECT_TRUE(CheckNotOperationalException([&]()->void {rw->GetStatistics(); })) << "incorrect behavior";
	EXPECT_TRUE(CheckNotOperationalException([&]()->void {rw->GetPyramidStatistics(); })) << "incorrect behavior";

	EXPECT_TRUE(CheckNotOperationalException([&]()->void {rw->ReadAttachment(0); })) << "incorrect behavior";
}

TEST(CziReaderWriter, ReaderWriterUpdateGuid1)
{
	auto testCzi = CreateTestCzi();

	auto inOutStream = make_shared<CMemInputOutputStream>(get<0>(testCzi).get(), get<1>(testCzi));
	auto rw = CreateCZIReaderWriter();

	auto options = make_shared<CCziReaderWriterInfo>(GUID{ 1,1,1,{ 1,1,1,1,1,1,1,1 } });
	options->SetForceFileGuid(true);
	rw->Create(inOutStream, options);
	rw.reset();

	auto spReader = libCZI::CreateCZIReader();
	spReader->Open(inOutStream);

	bool isCorrectGuid = options->GetFileGuid() == spReader->GetFileHeaderInfo().fileGuid;
	EXPECT_TRUE(isCorrectGuid) << "incorrect behavior";
}

TEST(CziReaderWriter, ReaderWriterUpdateGuid2)
{
	auto testCzi = CreateTestCzi();

	auto inOutStream = make_shared<CMemInputOutputStream>(get<0>(testCzi).get(), get<1>(testCzi));

	// retrieve the GUID which we initially find in the CZI
	auto spReader = libCZI::CreateCZIReader();
	spReader->Open(inOutStream);
	GUID initialFileGuid = spReader->GetFileHeaderInfo().fileGuid;
	spReader.reset();

	auto rw = CreateCZIReaderWriter();

	auto options = make_shared<CCziReaderWriterInfo>();
	options->SetForceFileGuid(true);

	// now we expect that the reader-writer-object creates a new Guid (and puts it into the CZI)
	rw->Create(inOutStream, options);
	rw.reset();

	spReader = libCZI::CreateCZIReader();
	spReader->Open(inOutStream);

	// we have no idea which GUID was created, but it should now be different to the one the CZI had initially
	bool isCorrectGuid = initialFileGuid != spReader->GetFileHeaderInfo().fileGuid;
	EXPECT_TRUE(isCorrectGuid) << "incorrect behavior";
}

TEST(CziReaderWriter, ReaderWriterEmpty1)
{
	auto rw = CreateCZIReaderWriter();
	auto inOutStream = make_shared<CMemInputOutputStream>(0);

	rw->Create(inOutStream);

	auto bitmap = CreateTestBitmap(PixelType::Gray8, 4, 4);

	ScopedBitmapLockerSP lockBm{ bitmap };
	AddSubBlockInfoStridedBitmap addSbBlkInfo;

	for (int z = 0; z < 10; ++z)
	{
		for (int c = 0; c < 1; ++c)
		{
			for (int m = 0; m < 5; ++m)
			{
				addSbBlkInfo.Clear();
				addSbBlkInfo.coordinate.Set(DimensionIndex::C, c);
				addSbBlkInfo.coordinate.Set(DimensionIndex::Z, z);
				addSbBlkInfo.mIndexValid = true;
				addSbBlkInfo.mIndex = m;
				addSbBlkInfo.x = m * bitmap->GetWidth();
				addSbBlkInfo.y = 0;
				addSbBlkInfo.logicalWidth = bitmap->GetWidth();
				addSbBlkInfo.logicalHeight = bitmap->GetHeight();
				addSbBlkInfo.physicalWidth = bitmap->GetWidth();
				addSbBlkInfo.physicalHeight = bitmap->GetHeight();
				addSbBlkInfo.PixelType = bitmap->GetPixelType();
				addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
				addSbBlkInfo.strideBitmap = lockBm.stride;

				rw->SyncAddSubBlock(addSbBlkInfo);
			}
		}
	}

	rw->Close();
	rw.reset();

	WriteOutTestCzi("CziReaderWriter", "ReaderWriterEmpty1", inOutStream);

	CSegmentWalker::ExpectedSegment expectedSegments[] =
	{
		{ 1,"ZISRAWFILE" },
		{ 50,"ZISRAWSUBBLOCK" },
		{ 1,"ZISRAWDIRECTORY" }
	};

	bool success = CSegmentWalker::CheckSegments(inOutStream.get(), expectedSegments, sizeof(expectedSegments) / sizeof(expectedSegments[0]));
	EXPECT_TRUE(success) << "not the expected result";
}

static void AddSomeSubBlocks(ICziReaderWriter* rw)
{
	auto bitmap = CreateTestBitmap(PixelType::Gray8, 4, 4);

	ScopedBitmapLockerSP lockBm{ bitmap };
	AddSubBlockInfoStridedBitmap addSbBlkInfo;

	for (int z = 0; z < 10; ++z)
	{
		for (int c = 0; c < 1; ++c)
		{
			for (int m = 0; m < 5; ++m)
			{
				addSbBlkInfo.Clear();
				addSbBlkInfo.coordinate.Set(DimensionIndex::C, c);
				addSbBlkInfo.coordinate.Set(DimensionIndex::Z, z);
				addSbBlkInfo.mIndexValid = true;
				addSbBlkInfo.mIndex = m;
				addSbBlkInfo.x = m * bitmap->GetWidth();
				addSbBlkInfo.y = 0;
				addSbBlkInfo.logicalWidth = bitmap->GetWidth();
				addSbBlkInfo.logicalHeight = bitmap->GetHeight();
				addSbBlkInfo.physicalWidth = bitmap->GetWidth();
				addSbBlkInfo.physicalHeight = bitmap->GetHeight();
				addSbBlkInfo.PixelType = bitmap->GetPixelType();
				addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
				addSbBlkInfo.strideBitmap = lockBm.stride;

				rw->SyncAddSubBlock(addSbBlkInfo);
			}
		}
	}
}

TEST(CziReaderWriter, ReaderWriterEmpty2)
{
	auto rw = CreateCZIReaderWriter();
	auto inOutStream = make_shared<CMemInputOutputStream>(0);

	rw->Create(inOutStream);

	AddSomeSubBlocks(rw.get());

	auto spMdBuilder = libCZI::CreateMetadataBuilder();
	MetadataUtils::WriteFillWithSubBlockStatistics(spMdBuilder.get(), rw->GetStatistics());

	string xml = spMdBuilder->GetXml(true);
	WriteMetadataInfo writerMdInfo = { 0 };
	writerMdInfo.szMetadata = xml.c_str();
	writerMdInfo.szMetadataSize = xml.size();

	rw->SyncWriteMetadata(writerMdInfo);

	rw->Close();
	rw.reset();

	WriteOutTestCzi("CziReaderWriter", "ReaderWriterEmpty2", inOutStream);

	CSegmentWalker::ExpectedSegment expectedSegments[] =
	{
		{ 1,"ZISRAWFILE" },
		{ 50,"ZISRAWSUBBLOCK" },
		{ 1,"ZISRAWMETADATA" },
		{ 1,"ZISRAWDIRECTORY" }
	};

	bool success = CSegmentWalker::CheckSegments(inOutStream.get(), expectedSegments, sizeof(expectedSegments) / sizeof(expectedSegments[0]));
	EXPECT_TRUE(success) << "not the expected result";

	auto reader = CreateCZIReader();
	reader->Open(inOutStream);
	auto mdSegment = reader->ReadMetadataSegment();
	auto metadata = mdSegment->CreateMetaFromMetadataSegment();
	auto xmlRead = metadata->GetXml();

	const char* expectedResult =
		"<?xml version=\"1.0\"?>\n"
		"<ImageDocument>\n"
		" <Metadata>\n"
		"  <Information>\n"
		"   <Image>\n"
		"    <SizeX>20</SizeX>\n"
		"    <SizeY>4</SizeY>\n"
		"    <SizeZ>10</SizeZ>\n"
		"    <SizeC>1</SizeC>\n"
		"    <SizeM>5</SizeM>\n"
		"   </Image>\n"
		"  </Information>\n"
		" </Metadata>\n"
		"</ImageDocument>\n";

	EXPECT_TRUE(strcmp(xmlRead.c_str(), expectedResult) == 0) << "Incorrect result";
}

TEST(CziReaderWriter, ReaderWriterEmpty3)
{
	auto rw = CreateCZIReaderWriter();
	auto inOutStream = make_shared<CMemInputOutputStream>(0);

	rw->Create(inOutStream);

	AddSomeSubBlocks(rw.get());

	auto spMdBuilder = libCZI::CreateMetadataBuilder();
	MetadataUtils::WriteFillWithSubBlockStatistics(spMdBuilder.get(), rw->GetStatistics());

	string xml = spMdBuilder->GetXml(true);
	WriteMetadataInfo writerMdInfo = { 0 };
	writerMdInfo.szMetadata = xml.c_str();
	writerMdInfo.szMetadataSize = xml.size();

	rw->SyncWriteMetadata(writerMdInfo);

	// we now overwrite the metadata-segment (with some content which is larger, therefore we need to create a new segment and declare the previous one as "DELETED")
	xml += "<!-- THIS IS A COMMENT -->";
	writerMdInfo.szMetadata = xml.c_str();
	writerMdInfo.szMetadataSize = xml.size();
	rw->SyncWriteMetadata(writerMdInfo);

	rw->Close();
	rw.reset();

	WriteOutTestCzi("CziReaderWriter", "ReaderWriterEmpty3", inOutStream);

	CSegmentWalker::ExpectedSegment expectedSegments[] =
	{
		{  1, "ZISRAWFILE" },
		{ 50, "ZISRAWSUBBLOCK" },
		{  1, "DELETED" },
		{  1, "ZISRAWMETADATA" },
		{  1, "ZISRAWDIRECTORY" }
	};

	bool success = CSegmentWalker::CheckSegments(inOutStream.get(), expectedSegments, sizeof(expectedSegments) / sizeof(expectedSegments[0]));
	EXPECT_TRUE(success) << "not the expected result";

	auto reader = CreateCZIReader();
	reader->Open(inOutStream);
	auto mdSegment = reader->ReadMetadataSegment();
	size_t rawXmlDataSize;
	auto rawXmlData = mdSegment->GetRawData(IMetadataSegment::MemBlkType::XmlMetadata, &rawXmlDataSize);
	ASSERT_TRUE(rawXmlData) << "did not get metadata";
	ASSERT_TRUE(rawXmlDataSize == writerMdInfo.szMetadataSize) << "wrong size";
	EXPECT_TRUE(memcmp(rawXmlData.get(), xml.c_str(), rawXmlDataSize) == 0) << "wrong content";
	auto metadata = mdSegment->CreateMetaFromMetadataSegment();
	auto xmlRead = metadata->GetXml();

	const char* expectedResult =
		"<?xml version=\"1.0\"?>\n"
		"<ImageDocument>\n"
		" <Metadata>\n"
		"  <Information>\n"
		"   <Image>\n"
		"    <SizeX>20</SizeX>\n"
		"    <SizeY>4</SizeY>\n"
		"    <SizeZ>10</SizeZ>\n"
		"    <SizeC>1</SizeC>\n"
		"    <SizeM>5</SizeM>\n"
		"   </Image>\n"
		"  </Information>\n"
		" </Metadata>\n"
		"</ImageDocument>\n";

	EXPECT_TRUE(strcmp(xmlRead.c_str(), expectedResult) == 0) << "Incorrect result";
}
