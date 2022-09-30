// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "inc_libCZI.h"

/// This class allows to "walk" the segments of a CZI-file.
/// Note that for sake of simplicity it is implemented independently of libCZI.
class CSegmentWalker
{
public:
	static void Walk(libCZI::IStream* stream, std::function<bool(int cnt, const std::string& id, std::int64_t allocatedSize, std::int64_t usedSize)> func);

	struct ExpectedSegment
	{
		int cnt;
		const char* segmentId;
	};

	static bool CheckSegments(libCZI::IStream* stream, const CSegmentWalker::ExpectedSegment* expectedSegments, size_t countExpectedSegments);
};
