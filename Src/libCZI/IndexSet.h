// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "libCZI.h"

class CIndexSet : public libCZI::IIndexSet
{
private:
	struct interval
	{
		int start, end;

		bool IsContained(int index) const { return this->start <= index && index <= this->end ? true : false; }
	};

	std::vector<interval> intervals;
public:
	CIndexSet(const std::wstring& str);

	/// Query if 'index' is contained in the set.
	///
	/// \param index Index to query.
	///
	/// \return True if the specified index is contained, false if not.
	bool IsContained(int index) const override;
private:
	void ParseString(const std::wstring& str);
	void AddInterval(int start, int end);
	static std::tuple<int, int> ParsePart(const std::wstring& str);
	static int ValueFromNumString(const std::wstring& str);
};
