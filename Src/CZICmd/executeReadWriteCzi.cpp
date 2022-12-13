// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#if false
#include "stdafx.h"
#include "executeReadWriteCzi.h"
#include "IBitmapGen.h"

// TODO: EXPERIMENTAL EXPERIMENTAL

using namespace libCZI;
using namespace std;

bool executeReadWriteCzi(const CCmdLineOptions& options)
{
	std::wstring outputfilename = options.MakeOutputFilename(L"", L"czi");
	auto inOutStream = libCZI::CreateInputOutputStreamForFile(outputfilename.c_str());

	auto cziReaderWriter = CreateCZIReaderWriter();

	cziReaderWriter->Create(inOutStream);

	int key = -1;
	cziReaderWriter->EnumerateAttachments(
		[&](int index, const libCZI::AttachmentInfo& info)->bool
	{
		if (info.name == "Thumbnail")
		{
			key = index;
			return false;
		}
		return true;
	});

	AddAttachmentInfo add_attachment_info;
	add_attachment_info.contentGuid = GUID{ 0x12345678, 0x1234, 0x1234, { 1,2,3,4,5,6,7,8 } };
	add_attachment_info.SetContentFileType("ABCDEFGH");
	add_attachment_info.SetName("XYZ");

	add_attachment_info.dataSize = 500;
	add_attachment_info.ptrData = malloc(add_attachment_info.dataSize);

	cziReaderWriter->ReplaceAttachment(key, add_attachment_info);

	cziReaderWriter->Close();

	return true;
}

bool executeReadWriteCzi2(const CCmdLineOptions& options)
{
	std::wstring outputfilename = options.MakeOutputFilename(L"", L"czi");
	auto inOutStream = libCZI::CreateInputOutputStreamForFile(outputfilename.c_str());

	auto cziReaderWriter = CreateCZIReaderWriter();

	cziReaderWriter->Create(inOutStream);

	AddSubBlockInfoStridedBitmap addSbBlkInfo;
	addSbBlkInfo.coordinate = CDimCoordinate({ { DimensionIndex::Z,0 },{ DimensionIndex::C,0 },{ DimensionIndex::T,0 } });
	addSbBlkInfo.mIndex = 311;
	addSbBlkInfo.mIndexValid = false;
	addSbBlkInfo.x = 0;
	addSbBlkInfo.y = 0;
	addSbBlkInfo.physicalWidth = 512;
	addSbBlkInfo.physicalHeight = 512;
	addSbBlkInfo.logicalWidth = addSbBlkInfo.physicalWidth;
	addSbBlkInfo.logicalHeight = addSbBlkInfo.physicalHeight;

	int key = -1;
	cziReaderWriter->EnumerateSubBlocks(
		[&](int index, const libCZI::SubBlockInfo& info)->bool
	{
		if (Utils::Compare(&info.coordinate, &addSbBlkInfo.coordinate) == 0 /*&&
			info.mIndex == addSbBlkInfo.mIndex*/)
		{
			key = index;
			return false;
		}

		return true;
	});

	BitmapGenFactory::InitializeFactory();
	auto get = BitmapGenFactory::CreateDefaultBitmapGenerator();
	BitmapGenInfo genInfo;
	genInfo.Clear();
	genInfo.coord = &addSbBlkInfo.coordinate;
	genInfo.mIndex = addSbBlkInfo.mIndex;
	genInfo.mValid = addSbBlkInfo.mIndexValid;
	genInfo.tilePixelPosition = make_pair(addSbBlkInfo.physicalWidth, addSbBlkInfo.physicalHeight);
	auto bm = get->Create(PixelType::Gray16, std::get<0>(genInfo.tilePixelPosition), std::get<1>(genInfo.tilePixelPosition), genInfo);

	{
		ScopedBitmapLockerSP locker(bm);
		addSbBlkInfo.PixelType = bm->GetPixelType();
		addSbBlkInfo.ptrBitmap = locker.ptrDataRoi;
		addSbBlkInfo.strideBitmap = locker.stride;
		//addSbBlkInfo.ptrSbBlkMetadata = sbMdXml.c_str();
		//addSbBlkInfo.sbBlkMetadataSize = sbMdXml.size();
		cziReaderWriter->ReplaceSubBlock(key, addSbBlkInfo);
	}

	cziReaderWriter->Close();

	return true;
}

bool executeReadWriteCzi_(const CCmdLineOptions& options)
{
	std::wstring outputfilename = options.MakeOutputFilename(L"", L"czi");
	auto inOutStream = libCZI::CreateInputOutputStreamForFile(outputfilename.c_str());

	auto cziReaderWriter = CreateCZIReaderWriter();

	cziReaderWriter->Create(inOutStream);

	AddSubBlockInfoStridedBitmap addSbBlkInfo;
	addSbBlkInfo.coordinate = CDimCoordinate({ { DimensionIndex::C,0},{ DimensionIndex::S,4 },{DimensionIndex::B,0} });
	addSbBlkInfo.mIndex = 311;
	addSbBlkInfo.mIndexValid = true;
	addSbBlkInfo.x = -105152;
	addSbBlkInfo.y = 84246;
	addSbBlkInfo.physicalWidth = 1600;
	addSbBlkInfo.physicalHeight = 1200;
	addSbBlkInfo.logicalWidth = addSbBlkInfo.physicalWidth;
	addSbBlkInfo.logicalHeight = addSbBlkInfo.physicalHeight;

	int key = -1;
	cziReaderWriter->EnumerateSubBlocks(
		[&](int index, const libCZI::SubBlockInfo& info)->bool
	{
		if (Utils::Compare(&info.coordinate, &addSbBlkInfo.coordinate) == 0 &&
			info.mIndex == addSbBlkInfo.mIndex)
		{
			key = index;
			return false;
		}

		return true;
	});

	BitmapGenFactory::InitializeFactory();
	auto get = BitmapGenFactory::CreateDefaultBitmapGenerator();
	BitmapGenInfo genInfo;
	genInfo.Clear();
	genInfo.coord = &addSbBlkInfo.coordinate;
	genInfo.mIndex = addSbBlkInfo.mIndex;
	genInfo.mValid = addSbBlkInfo.mIndexValid;
	genInfo.tilePixelPosition = make_pair(addSbBlkInfo.physicalWidth, addSbBlkInfo.physicalHeight);
	auto bm = get->Create(PixelType::Bgr24, std::get<0>(genInfo.tilePixelPosition), std::get<1>(genInfo.tilePixelPosition), genInfo);

	{
		ScopedBitmapLockerSP locker(bm);
		addSbBlkInfo.PixelType = bm->GetPixelType();
		addSbBlkInfo.ptrBitmap = locker.ptrDataRoi;
		addSbBlkInfo.strideBitmap = locker.stride;
		//addSbBlkInfo.ptrSbBlkMetadata = sbMdXml.c_str();
		//addSbBlkInfo.sbBlkMetadataSize = sbMdXml.size();
		cziReaderWriter->ReplaceSubBlock(key, addSbBlkInfo);
	}

	cziReaderWriter->Close();

	return true;
}
#endif