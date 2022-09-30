// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <functional>
#include <set>
#include "libCZI.h"

class CCziSubBlockDirectoryBase
{
public:
	struct SubBlkEntry
	{
		libCZI::CDimCoordinate coordinate;
		int mIndex;
		int x;
		int y;
		int width;
		int height;
		int storedWidth;
		int storedHeight;
		int PixelType;
		std::uint64_t FilePosition;
		int Compression;

		bool IsMIndexValid() const
		{
			return this->mIndex != (std::numeric_limits<int>::min)() ? true : false;
		}

		bool IsStoredSizeEqualLogicalSize() const
		{
			return this->width == this->storedWidth && this->height == this->storedHeight;
		}

		void Invalidate()
		{
			this->mIndex = this->x = this->y = this->width = this->height = this->storedWidth = this->storedHeight = (std::numeric_limits<int>::min)();
		}
	};

	struct SubBlkEntryEx : SubBlkEntry
	{
		std::uint64_t allocatedSize;
	};

	static bool CompareForEquality_Coordinate(const SubBlkEntry& a, const SubBlkEntry& b);
};

class CSbBlkStatisticsUpdater
{
private:
	libCZI::SubBlockStatistics statistics;
	libCZI::PyramidStatistics pyramidStatistics;
	bool pyramidStatisticsDirty;
public:
	CSbBlkStatisticsUpdater();
	void UpdateStatistics(const CCziSubBlockDirectoryBase::SubBlkEntry& entry);
	void Consolidate();

	const libCZI::SubBlockStatistics& GetStatistics() const;
	const libCZI::PyramidStatistics& GetPyramidStatistics();

	void Clear();
private:
	void SortPyramidStatistics();
	static void UpdateBoundingBox(libCZI::IntRect& rect, const CCziSubBlockDirectoryBase::SubBlkEntry& entry);
	static bool TryToDeterminePyramidLayerInfo(const CCziSubBlockDirectoryBase::SubBlkEntry& entry, std::uint8_t* ptrMinificationFactor, std::uint8_t* ptrPyramidLayerNo);
	static void UpdatePyramidLayerStatistics(std::vector<libCZI::PyramidStatistics::PyramidLayerStatistics>& vec, const libCZI::PyramidStatistics::PyramidLayerInfo& pli);
};


class CCziSubBlockDirectory : public CCziSubBlockDirectoryBase
{
private:
	std::vector<SubBlkEntry> subBlks;
	mutable CSbBlkStatisticsUpdater sblkStatistics;
	enum class State
	{
		AddingAllowed,
		AddingFinished
	};
	State state;
public:
	CCziSubBlockDirectory();

	const libCZI::SubBlockStatistics& GetStatistics() const;
	const libCZI::PyramidStatistics& GetPyramidStatistics() const;

	void AddSubBlock(const SubBlkEntry& entry);
	void AddingFinished();

	void EnumSubBlocks(std::function<bool(int index, const SubBlkEntry&)> func);
	bool TryGetSubBlock(int index, SubBlkEntry& entry) const;
};

class PixelTypeForChannelIndexStatistic
{
protected:
	bool pixeltypeNoValidChannelIdxValid;
	int pixelTypeNoValidChannel;
	std::map<int, int> pixelTypePerChannelIndex;
public:
	PixelTypeForChannelIndexStatistic() :pixeltypeNoValidChannelIdxValid(false)
	{}

	/// Attempts to get the pixeltype for "sub-blocks without a channel index".
	///
	/// \param [in,out] pixelType If non-null, will receive the pixeltype (if successful).
	/// \return True if it succeeds, false if it fails.
	bool TryGetPixelTypeForNoChannelIndex(int* pixelType) const;

	/// Gets a map where key is the channel-index and value is the pixel-type that was determinded.
	///
	/// \return A map where key is the channel-index and value is the pixel-type that was determinded.
	const std::map<int, int>& GetChannelIndexPixelTypeMap() const { return this->pixelTypePerChannelIndex; }
};

class CWriterCziSubBlockDirectory : public CCziSubBlockDirectoryBase
{
private:
	class PixelTypeForChannelIndexStatisticCreate : public PixelTypeForChannelIndexStatistic
	{
	public:
		void AddSbBlk(const SubBlkEntry& entry);
	};
private:
	mutable CSbBlkStatisticsUpdater sblkStatistics;
	std::map<int, int> mapChannelIdxPixelType;
	PixelTypeForChannelIndexStatisticCreate pixelTypeForChannel;
public:
	bool TryAddSubBlock(const SubBlkEntry& entry);

	bool EnumEntries(const std::function<bool(size_t index, const SubBlkEntry&)>& func) const;

	const libCZI::SubBlockStatistics& GetStatistics() const;
	const libCZI::PyramidStatistics& GetPyramidStatistics() const;
	const PixelTypeForChannelIndexStatistic& GetPixelTypeForChannel() const;
private:
	struct SubBlkEntryCompare
	{
		bool operator() (const SubBlkEntry& a, const SubBlkEntry& b) const;
	};

	std::set<SubBlkEntry, SubBlkEntryCompare> subBlks;
};

class CReaderWriterCziSubBlockDirectory : public CCziSubBlockDirectoryBase
{
private:
	mutable CSbBlkStatisticsUpdater sblkStatistics;

	/// Indicates that the "SubBlock-Statistics" is current and up-to-date. Attention: this only means that "GetStatistics()" is valid,
	/// "GetPyramidStatistics" might be not current.
	bool sbBlkStatisticsCurrent;

	/// Indicates that the "SubBlock-Statistics" is current and up-to-date AND that it is "consolidated", ie. that also
	/// "GetPyramidStatistics" is valid and up-to-date.
	bool sbBlkStatisticsConsolidated;
	
	int nextSbBlkIndex;
	std::map<int, SubBlkEntry> subBlks;

	bool isModified;
public:
	CReaderWriterCziSubBlockDirectory() :sbBlkStatisticsCurrent(true), sbBlkStatisticsConsolidated(false), nextSbBlkIndex(0), isModified(false) {}

	bool IsModified()const { return this->isModified; }
	void SetModified(bool modified) { this->isModified = modified; }

	void AddSubBlock(const SubBlkEntry& entry, int* key = nullptr);

	bool TryGetSubBlock(int key, SubBlkEntry* entry) const;

	bool TryModifySubBlock(int key, const SubBlkEntry& entry);
	bool TryRemoveSubBlock(int key, SubBlkEntry* entry = nullptr);
	bool TryAddSubBlock(const SubBlkEntry& entry, int* key);

	bool EnumEntries(const std::function<bool(int index, const SubBlkEntry&)>& func) const;

	const libCZI::SubBlockStatistics& GetStatistics();
	const libCZI::PyramidStatistics& GetPyramidStatistics();

private:
	void RecreateSubBlockStatistics();
	void EnsureSbBlkStatisticsConsolidated();
};