// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CziSubBlockDirectory.h"
#include "CziUtils.h"
#include <algorithm>
#include <cmath>
#include <cstddef>

using namespace libCZI;
using namespace libCZI::detail;
using namespace std;

/*static*/bool CCziSubBlockDirectoryBase::CompareForEquality_Coordinate(const SubBlkEntry& a, const SubBlkEntry& b)
{
    if (Utils::Compare(&a.coordinate, &b.coordinate) == 0)
    {
        // if the coordinates are equal, then m-index (if valid) must be different are at least one of the blocks must
        // be a pyramid-subblock
        if (a.IsMIndexValid() && b.IsMIndexValid())
        {
            if (a.mIndex == b.mIndex && a.IsStoredSizeEqualLogicalSize() && b.IsStoredSizeEqualLogicalSize())
            {
                return true;
            }
        }
    }

    return false;
}

//----------------------------------------------------------------------------------------

CSbBlkStatisticsUpdater::CSbBlkStatisticsUpdater() :pyramidStatisticsDirty(false)
{
    this->statistics.Invalidate();
    this->statistics.subBlockCount = 0;
}

void CSbBlkStatisticsUpdater::Clear()
{
    this->statistics.Invalidate();
    this->statistics.subBlockCount = 0;
    this->pyramidStatisticsDirty = false;
}

void CSbBlkStatisticsUpdater::UpdateStatistics(const CCziSubBlockDirectoryBase::SubBlkEntry& entry)
{
    // TODO: check validity of x,y etc.
    CSbBlkStatisticsUpdater::UpdateBoundingBox(this->statistics.boundingBox, entry);
    if (entry.IsStoredSizeEqualLogicalSize())
    {
        CSbBlkStatisticsUpdater::UpdateBoundingBox(this->statistics.boundingBoxLayer0Only, entry);
    }

    entry.coordinate.EnumValidDimensions(
        [&](libCZI::DimensionIndex dim, int value)->bool
        {
            int start, size;
            if (this->statistics.dimBounds.TryGetInterval(dim, &start, &size) == false)
            {
                this->statistics.dimBounds.Set(dim, value, 1);
            }
            else
            {
                bool changed = false;
                if (value < start)
                {
                    size += (start - value);
                    start = value;
                    changed = true;
                }
                else if (value >= start + size)
                {
                    size = 1 + value - start;
                    changed = true;
                }

                if (changed)
                {
                    this->statistics.dimBounds.Set(dim, start, size);
                }
            }

            if (entry.IsMIndexValid())
            {
                if (entry.mIndex < this->statistics.minMindex)
                {
                    this->statistics.minMindex = entry.mIndex;
                }

                if (entry.mIndex > this->statistics.maxMindex)
                {
                    this->statistics.maxMindex = entry.mIndex;
                }
            }

            return true;
        });

    // now deal with "enclosing Rect" for the scenes...
    int sceneIndex;
    if (entry.coordinate.TryGetPosition(libCZI::DimensionIndex::S, &sceneIndex))
    {
        auto it = this->statistics.sceneBoundingBoxes.find(sceneIndex);
        if (it != this->statistics.sceneBoundingBoxes.end())
        {
            CSbBlkStatisticsUpdater::UpdateBoundingBox(it->second.boundingBox, entry);
            if (entry.IsStoredSizeEqualLogicalSize() == true)
            {
                CSbBlkStatisticsUpdater::UpdateBoundingBox(it->second.boundingBoxLayer0, entry);
            }
        }
        else
        {
            BoundingBoxes boundingBoxes;
            boundingBoxes.boundingBox.x = entry.x;
            boundingBoxes.boundingBox.y = entry.y;
            boundingBoxes.boundingBox.w = entry.width;
            boundingBoxes.boundingBox.h = entry.height;
            if (entry.IsStoredSizeEqualLogicalSize() == true)
            {
                boundingBoxes.boundingBoxLayer0 = boundingBoxes.boundingBox;
            }
            else
            {
                boundingBoxes.boundingBoxLayer0.Invalidate();
            }

            this->statistics.sceneBoundingBoxes.insert(std::pair<int, BoundingBoxes>(sceneIndex, boundingBoxes));
        }
    }

    this->pyramidStatisticsDirty = true;

    // now deal with the pyramid-layer info
    if (!entry.coordinate.TryGetPosition(libCZI::DimensionIndex::S, &sceneIndex))
    {
        // in case that the scene-index is not valid, then we use int::max in order to
        // represent this fact
        sceneIndex = (std::numeric_limits<int>::max)();
    }

    PyramidStatistics::PyramidLayerInfo pli;
    bool b = CSbBlkStatisticsUpdater::TryToDeterminePyramidLayerInfo(entry, &pli.minificationFactor, &pli.pyramidLayerNo);
    if (b == false)
    {
        pli.minificationFactor = pli.pyramidLayerNo = 0xff;
    }

    auto it = this->pyramidStatistics.scenePyramidStatistics.find(sceneIndex);
    if (it != this->pyramidStatistics.scenePyramidStatistics.end())
    {
        CSbBlkStatisticsUpdater::UpdatePyramidLayerStatistics(it->second, pli);
    }
    else
    {
        std::vector<PyramidStatistics::PyramidLayerStatistics> vecPs;
        CSbBlkStatisticsUpdater::UpdatePyramidLayerStatistics(vecPs, pli);
        this->pyramidStatistics.scenePyramidStatistics.insert(std::pair<int, std::vector<PyramidStatistics::PyramidLayerStatistics>>(sceneIndex, vecPs));
    }

    ++this->statistics.subBlockCount;
}

/// This method is to be called in order to "finish up" the pyramid-statistics.
void CSbBlkStatisticsUpdater::Consolidate()
{
    if (this->pyramidStatisticsDirty)
    {
        this->SortPyramidStatistics();
        this->pyramidStatisticsDirty = true;
    }
}

const libCZI::SubBlockStatistics& CSbBlkStatisticsUpdater::GetStatistics() const
{
    return this->statistics;
}

const libCZI::PyramidStatistics& CSbBlkStatisticsUpdater::GetPyramidStatistics()
{
    this->Consolidate();
    return this->pyramidStatistics;
}

/*static*/void CSbBlkStatisticsUpdater::UpdateBoundingBox(libCZI::IntRect& rect, const CCziSubBlockDirectoryBase::SubBlkEntry& entry)
{
    if (rect.IsValid() == true)
    {
        if (rect.x > entry.x)
        {
            int diff = rect.x - entry.x;
            rect.x = entry.x;
            rect.w += diff;
        }

        if (rect.y > entry.y)
        {
            int diff = rect.y - entry.y;
            rect.y = entry.y;
            rect.h += diff;
        }

        if (rect.x + rect.w < entry.x + entry.width)
        {
            rect.w = (entry.x + entry.width) - rect.x;
        }

        if (rect.y + rect.h < entry.y + entry.height)
        {
            rect.h = (entry.y + entry.height) - rect.y;
        }
    }
    else
    {
        rect.x = entry.x;
        rect.y = entry.y;
        rect.w = entry.width;
        rect.h = entry.height;
    }
}

/// Attempts to to determine pyramid layer information from the given data.
/// If we have a layer-0 subblock, minificationFactor and pyramdidLayerNo are set to 0.
///
/// \param entry                       The subblock-entry.
/// \param [out] ptrMinificationFactor If non-null, the minification factor will be stored here.
/// \param [out] ptrPyramidLayerNo     If non-null, the pyramid layer no will be stored here.
///
/// \return True if it succeeds, false if it fails.
/*static*/bool CSbBlkStatisticsUpdater::TryToDeterminePyramidLayerInfo(const CCziSubBlockDirectoryBase::SubBlkEntry& entry, std::uint8_t* ptrMinificationFactor, std::uint8_t* ptrPyramidLayerNo)
{
    if (entry.width == entry.storedWidth && entry.height == entry.storedHeight)
    {
        if (ptrMinificationFactor != nullptr) { *ptrMinificationFactor = 0; }
        if (ptrPyramidLayerNo != nullptr) { *ptrPyramidLayerNo = 0; }
        return true;
    }

    double minificationFactor = CziUtils::CalculateMinificationFactor(entry.width, entry.height, entry.storedWidth, entry.storedHeight);

    struct MinificationFactorToPyramidLayerInfo
    {
        double value, delta_min, delta_max;
        int pyramidLayer;

        bool IsInRange(double v) const
        {
            if (v >= (this->value - this->delta_min) && v <= (this->value + this->delta_max))
            {
                return true;
            }

            return false;
        }
    };

    static const MinificationFactorToPyramidLayerInfo MinFacToPLI_Factor2[] =
    {
    { 2,.1,.1,1 },
    { 4,.2,.2,2 },
    { 8,.4,.4,3 },
    { 16,.8,.8,4 },
    { 32, 1,1,5 },
    { 64, 1,1,6 },
    { 128,1,1,7 },
    { 256,2,2,8 },
    { 512,4,4,9 },
    { 1024,10,10,10 }
    };

    static const MinificationFactorToPyramidLayerInfo MinFacToPLI_Factor3[] =
    {
    { 3,.1,.1,1 },
    { 9,.2,.2,2 },
    { 27,.8,.8,3 },
    { 81,1.5,1.5,4 },
    { 243, 2,2,5 },
    { 729, 5,5,6 },
    { 2187,15,15,7 }
    };

    // check whether it is a factor of 2
    for (size_t i = 0; i < sizeof(MinFacToPLI_Factor2) / sizeof(MinFacToPLI_Factor2[0]); ++i)
    {
        if (MinFacToPLI_Factor2[i].IsInRange(minificationFactor))
        {
            if (ptrMinificationFactor != nullptr) { *ptrMinificationFactor = 2; }
            if (ptrPyramidLayerNo != nullptr) { *ptrPyramidLayerNo = MinFacToPLI_Factor2[i].pyramidLayer; }
            return true;
        }
    }

    // check whether it is a factor of 3
    for (size_t i = 0; i < sizeof(MinFacToPLI_Factor3) / sizeof(MinFacToPLI_Factor3[0]); ++i)
    {
        if (MinFacToPLI_Factor3[i].IsInRange(minificationFactor))
        {
            if (ptrMinificationFactor != nullptr) { *ptrMinificationFactor = 3; }
            if (ptrPyramidLayerNo != nullptr) { *ptrPyramidLayerNo = MinFacToPLI_Factor3[i].pyramidLayer; }
            return true;
        }
    }

    return false;
}

/*static*/void CSbBlkStatisticsUpdater::UpdatePyramidLayerStatistics(std::vector<libCZI::PyramidStatistics::PyramidLayerStatistics>& vec, const PyramidStatistics::PyramidLayerInfo& pli)
{
    auto it = std::find_if(vec.begin(), vec.end(), [&](const PyramidStatistics::PyramidLayerStatistics& i) {return pli.minificationFactor == i.layerInfo.minificationFactor && pli.pyramidLayerNo == i.layerInfo.pyramidLayerNo; });
    if (it != vec.end())
    {
        ++it->count;
    }
    else
    {
        PyramidStatistics::PyramidLayerStatistics pls;
        pls.layerInfo = pli;
        pls.count = 1;
        vec.emplace_back(pls);
    }
}

void CSbBlkStatisticsUpdater::SortPyramidStatistics()
{
    for (auto& v : this->pyramidStatistics.scenePyramidStatistics)
    {
        std::sort(v.second.begin(), v.second.end(), [](const PyramidStatistics::PyramidLayerStatistics& a, const PyramidStatistics::PyramidLayerStatistics& b)->bool
            {
                // layer0 always "goes first"
                if (a.layerInfo.IsLayer0())
                {
                    return true;
                }

                // ...and "not identified" always last
                if (a.layerInfo.IsNotIdentifiedAsPyramidLayer())
                {
                    return false;
                }

                if (b.layerInfo.IsLayer0())
                {
                    return false;
                }

                if (b.layerInfo.IsNotIdentifiedAsPyramidLayer())
                {
                    return true;
                }

                int minificationFactorA = a.layerInfo.minificationFactor;
                for (int i = 0; i < a.layerInfo.pyramidLayerNo - 1; ++i)
                {
                    minificationFactorA *= a.layerInfo.minificationFactor;
                }

                int minificationFactorB = b.layerInfo.minificationFactor;
                for (int i = 0; i < b.layerInfo.pyramidLayerNo - 1; ++i)
                {
                    minificationFactorB *= b.layerInfo.minificationFactor;
                }

                return minificationFactorA < minificationFactorB;
            });
    }
}

// ---------------------------------------------------------------------------------------------

CCziSubBlockDirectory::CCziSubBlockDirectory() : state(State::AddingAllowed)
{
}

void CCziSubBlockDirectory::AddSubBlock(const SubBlkEntry& entry)
{
    if (this->state != State::AddingAllowed)
    {
        throw std::logic_error("The object is not allowing to add subblocks any more.");
    }

    this->subBlks.push_back(entry);
    this->sblkStatistics.UpdateStatistics(entry);
}

void CCziSubBlockDirectory::AddingFinished()
{
    this->state = State::AddingFinished;
    this->sblkStatistics.Consolidate();
    this->BuildSubsetIndex();
}

void CCziSubBlockDirectory::BuildSubsetIndex()
{
    this->indexedPlaneDimensions.clear();
    this->subsetIndex.clear();
    this->firstSubBlockByChannel.clear();
    this->firstSubBlockWithoutChannel = -1;

    const auto& statistics = this->sblkStatistics.GetStatistics();
    for (auto value = static_cast<int>(DimensionIndex::MinDim);
        value <= static_cast<int>(DimensionIndex::MaxDim); ++value)
    {
        const auto dimension = static_cast<DimensionIndex>(value);
        int size = 0;
        if (dimension != DimensionIndex::S &&
            statistics.dimBounds.TryGetInterval(dimension, nullptr, &size) && size > 1)
        {
            this->indexedPlaneDimensions.push_back(dimension);
        }
    }

    for (std::size_t index = 0; index < this->subBlks.size(); ++index)
    {
        const auto& entry = this->subBlks[index];
        PlaneKey planeKey;
        planeKey.reserve(this->indexedPlaneDimensions.size());
        for (const auto dimension : this->indexedPlaneDimensions)
        {
            int coordinate = (std::numeric_limits<int>::min)();
            entry.coordinate.TryGetPosition(dimension, &coordinate);
            planeKey.push_back(coordinate);
        }

        int scene = (std::numeric_limits<int>::min)();
        entry.coordinate.TryGetPosition(DimensionIndex::S, &scene);
        const auto zoom = Utils::CalcZoom(
            IntSize{static_cast<std::uint32_t>(entry.width), static_cast<std::uint32_t>(entry.height)},
            IntSize{static_cast<std::uint32_t>(entry.storedWidth), static_cast<std::uint32_t>(entry.storedHeight)});
        const int zoomKey = static_cast<int>(std::lround(zoom * 1000000.0f));
        this->subsetIndex[planeKey][{scene, zoomKey}].indicesByX.push_back(static_cast<int>(index));

        int channel = 0;
        if (entry.coordinate.TryGetPosition(DimensionIndex::C, &channel))
        {
            this->firstSubBlockByChannel.emplace(channel, static_cast<int>(index));
        }
        else if (this->firstSubBlockWithoutChannel < 0)
        {
            this->firstSubBlockWithoutChannel = static_cast<int>(index);
        }
    }

    for (auto& planeEntry : this->subsetIndex)
    {
        for (auto& groupEntry : planeEntry.second)
        {
            auto& group = groupEntry.second;
            std::sort(group.indicesByX.begin(), group.indicesByX.end(),
                [this](int left, int right)
                {
                    const auto& leftEntry = this->subBlks[left];
                    const auto& rightEntry = this->subBlks[right];
                    return leftEntry.x != rightEntry.x ? leftEntry.x < rightEntry.x : left < right;
                });
            group.prefixMaximumRight.reserve(group.indicesByX.size());
            std::int64_t maximumRight = (std::numeric_limits<std::int64_t>::min)();
            for (const int subBlockIndex : group.indicesByX)
            {
                const auto& entry = this->subBlks[subBlockIndex];
                maximumRight = (std::max)(maximumRight,
                    static_cast<std::int64_t>(entry.x) + entry.width);
                group.prefixMaximumRight.push_back(maximumRight);
            }
        }
    }
}

const libCZI::SubBlockStatistics& CCziSubBlockDirectory::GetStatistics() const
{
    return this->sblkStatistics.GetStatistics();
}

const libCZI::PyramidStatistics& CCziSubBlockDirectory::GetPyramidStatistics() const
{
    //return this->pyramidStatistics;
    return this->sblkStatistics.GetPyramidStatistics();
}

void CCziSubBlockDirectory::EnumSubBlocks(const std::function<bool(int index, const SubBlkEntry&)>& func)
{
    int i = 0;
    for (auto it = this->subBlks.cbegin(); it != this->subBlks.cend(); ++it)
    {
        bool b = func(i++, *it);
        if (b == false)
        {
            break;
        }
    }
}

bool CCziSubBlockDirectory::TryEnumSubset(
    const libCZI::IDimCoordinate* planeCoordinate,
    const libCZI::IntRect* roi,
    bool onlyLayer0,
    const libCZI::IIndexSet* sceneFilter,
    const std::function<bool(int index, const SubBlkEntry&)>& func) const
{
    if (planeCoordinate == nullptr || this->state != State::AddingFinished)
    {
        return false;
    }

    PlaneKey planeKey;
    planeKey.reserve(this->indexedPlaneDimensions.size());
    for (const auto dimension : this->indexedPlaneDimensions)
    {
        int coordinate = 0;
        if (!planeCoordinate->TryGetPosition(dimension, &coordinate))
        {
            return false;
        }

        planeKey.push_back(coordinate);
    }

    const auto planeIterator = this->subsetIndex.find(planeKey);
    if (planeIterator == this->subsetIndex.end())
    {
        return true;
    }

    std::vector<int> candidates;
    for (const auto& groupEntry : planeIterator->second)
    {
        const int scene = groupEntry.first.first;
        if (scene != (std::numeric_limits<int>::min)() &&
            sceneFilter != nullptr && !sceneFilter->IsContained(scene))
        {
            continue;
        }

        const auto& group = groupEntry.second;
        if (roi == nullptr)
        {
            candidates.insert(candidates.end(), group.indicesByX.begin(), group.indicesByX.end());
            continue;
        }

        const std::int64_t roiRight = static_cast<std::int64_t>(roi->x) + roi->w;
        auto end = std::lower_bound(group.indicesByX.begin(), group.indicesByX.end(), roiRight,
            [this](int index, std::int64_t right)
            {
                return static_cast<std::int64_t>(this->subBlks[index].x) < right;
            });
        while (end != group.indicesByX.begin())
        {
            --end;
            const auto position = static_cast<std::size_t>(std::distance(group.indicesByX.begin(), end));
            if (group.prefixMaximumRight[position] <= roi->x)
            {
                break;
            }

            const auto& entry = this->subBlks[*end];
            const std::int64_t entryRight = static_cast<std::int64_t>(entry.x) + entry.width;
            const std::int64_t entryBottom = static_cast<std::int64_t>(entry.y) + entry.height;
            const std::int64_t roiBottom = static_cast<std::int64_t>(roi->y) + roi->h;
            if (entryRight > roi->x && entryBottom > roi->y &&
                static_cast<std::int64_t>(entry.y) < roiBottom)
            {
                candidates.push_back(*end);
            }
        }
    }

    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
    for (const int index : candidates)
    {
        const auto& entry = this->subBlks[index];
        if (!CziUtils::CompareCoordinate(planeCoordinate, &entry.coordinate))
        {
            continue;
        }

        if ((!onlyLayer0 || entry.IsStoredSizeEqualLogicalSize()) && !func(index, entry))
        {
            break;
        }
    }

    return true;
}

bool CCziSubBlockDirectory::TryGetSubBlockOfArbitrarySubBlockInChannel(
    int channelIndex, SubBlkEntry& entry) const
{
    const auto channelIterator = this->firstSubBlockByChannel.find(channelIndex);
    if (channelIterator != this->firstSubBlockByChannel.end())
    {
        entry = this->subBlks[channelIterator->second];
        return true;
    }

    // If a channel dimension exists, a request for an absent channel must fail.
    // A channel-less subblock is used only for documents without that dimension.
    if (!this->firstSubBlockByChannel.empty() || this->firstSubBlockWithoutChannel < 0)
    {
        return false;
    }

    entry = this->subBlks[this->firstSubBlockWithoutChannel];
    return true;
}

bool CCziSubBlockDirectory::TryGetSceneBoundingBox(int sceneIndex, libCZI::IntRect& boundingBox) const
{
    const auto& sceneBoundingBoxes = this->sblkStatistics.GetStatistics().sceneBoundingBoxes;
    const auto iterator = sceneBoundingBoxes.find(sceneIndex);
    if (iterator == sceneBoundingBoxes.end())
    {
        return false;
    }

    boundingBox = iterator->second.boundingBox;
    return true;
}

bool CCziSubBlockDirectory::TryGetSubBlock(int index, SubBlkEntry& entry) const
{
    if (index < (int)this->subBlks.size())
    {
        entry = this->subBlks.at(index);
        return true;
    }

    return false;
}

//----------------------------------------------------------------------------------------------

bool PixelTypeForChannelIndexStatistic::TryGetPixelTypeForNoChannelIndex(int* pixelType) const
{
    if (!this->pixeltypeNoValidChannelIdxValid)
    {
        return false;
    }

    if (pixelType != nullptr)
    {
        *pixelType = this->pixelTypeNoValidChannel;
    }

    return true;
}

//----------------------------------------------------------------------------------------------

CWriterCziSubBlockDirectory::CWriterCziSubBlockDirectory(bool allow_duplicate_subblocks)
    : subBlkEntryComparison{ allow_duplicate_subblocks },
    subBlks(subBlkEntryComparison)
{
}

bool CWriterCziSubBlockDirectory::TryAddSubBlock(const SubBlkEntry& entry)
{
    auto insert = this->subBlks.insert(entry);

    if (insert.second)
    {
        this->sblkStatistics.UpdateStatistics(entry);
        this->pixelTypeForChannel.AddSbBlk(entry);
    }

    return insert.second;
}

bool CWriterCziSubBlockDirectory::SubBlkEntryCompare::operator()(const SubBlkEntry& a, const SubBlkEntry& b) const
{
    // returns true if the first argument goes before the second argument in the strict weak ordering it defines, 
    // and false otherwise.

    // 1st check: zoom
    // 2nd check:  coordinate
    // 3rd check:  m-Index
    // 4th check: (only if both subblocks have invalid M-indices) is coordinate
    // 5th check: (only if the the property "include_file_position_" of the comparison-object is true)
    //            the file-position is compared   

    // 1st check: subblocks from a lower layer go before subblocks from an upper layer
    float zoomA = Utils::CalcZoom(IntSize{ (std::uint32_t)a.width,(std::uint32_t)a.height }, IntSize{ (std::uint32_t)a.storedWidth,(std::uint32_t)a.storedHeight });
    float zoomB = Utils::CalcZoom(IntSize{ (std::uint32_t)b.width,(std::uint32_t)b.height }, IntSize{ (std::uint32_t)b.storedWidth,(std::uint32_t)b.storedHeight });
    if (fabs(zoomA - zoomB) > 0.0001)
    {
        if (zoomA > zoomB)
        {
            return true;
        }

        return false;
    }

    // 2nd check: plane coordinates
    int r = Utils::Compare(&a.coordinate, &b.coordinate);
    if (r < 0)
    {
        return true;
    }
    else if (r > 0)
    {
        return false;
    }

    // 3rd check: m-index
    if (a.IsMIndexValid() == true && b.IsMIndexValid() == false)
    {
        return true;
    }

    if (a.IsMIndexValid() == false && b.IsMIndexValid() == true)
    {
        return false;
    }

    if (a.IsMIndexValid() == true && b.IsMIndexValid() == true)
    {
        if (a.mIndex < b.mIndex)
        {
            return true;
        }
        else if (a.mIndex > b.mIndex)
        {
            return false;
        }
    }

    // 4th check: the x-y-position
    if (a.IsMIndexValid() == false && b.IsMIndexValid() == false)
    {
        int v = a.x - b.x;
        if (v < 0)
        {
            return true;
        }
        if (v > 0)
        {
            return false;
        }

        v = a.y - b.y;
        if (v < 0)
        {
            return true;
        }
        if (v > 0)
        {
            return false;
        }
    }

    // 5th check: if we are instructed to include the file-position, then a lower file-position
    //  shoud go first (otherwise - they are considered "equal" which means that we do not allow
    //  to add a second one as it would be a duplicate).
    if (this->include_file_position_ && a.FilePosition < b.FilePosition)
    {
        return true;
    }

    return false;
}

bool CWriterCziSubBlockDirectory::EnumEntries(const std::function<bool(size_t index, const SubBlkEntry&)>& func) const
{
    size_t index = 0;
    for (auto it = this->subBlks.cbegin(); it != this->subBlks.cend(); ++it)
    {
        if (!func(index++, *it))
        {
            return false;
        }
    }

    return true;
}

const libCZI::SubBlockStatistics& CWriterCziSubBlockDirectory::GetStatistics() const
{
    return this->sblkStatistics.GetStatistics();
}

const libCZI::PyramidStatistics& CWriterCziSubBlockDirectory::GetPyramidStatistics() const
{
    return this->sblkStatistics.GetPyramidStatistics();
}

const PixelTypeForChannelIndexStatistic& CWriterCziSubBlockDirectory::GetPixelTypeForChannel() const
{
    return this->pixelTypeForChannel;
}

void CWriterCziSubBlockDirectory::PixelTypeForChannelIndexStatisticCreate::AddSbBlk(const SubBlkEntry& entry)
{
    int chIdx;
    if (!entry.coordinate.TryGetPosition(DimensionIndex::C, &chIdx))
    {
        if (!this->pixeltypeNoValidChannelIdxValid)
        {
            this->pixelTypeNoValidChannel = entry.PixelType;
            this->pixeltypeNoValidChannelIdxValid = true;
        }
    }
    else
    {
        // note that "insert" will only "insert" if the key does not already exist; if it exists, it does nothing
        this->pixelTypePerChannelIndex.insert(pair<int, int>(chIdx, entry.PixelType));
    }
}

//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------

void CReaderWriterCziSubBlockDirectory::AddSubBlock(const SubBlkEntry& entry, int* key /*= nullptr*/)
{
    this->subBlks.insert(std::pair<int, SubBlkEntry>(this->nextSbBlkIndex, entry));
    if (key != nullptr)
    {
        *key = this->nextSbBlkIndex;
    }

    this->SetModified(true);
    if (this->sbBlkStatisticsCurrent)   // no need to update if it is already "not up-to-date"
    {
        this->sblkStatistics.UpdateStatistics(entry);
        this->sbBlkStatisticsConsolidated = false;
    }

    this->nextSbBlkIndex++;
}

bool CReaderWriterCziSubBlockDirectory::EnumEntries(const std::function<bool(int index, const SubBlkEntry&)>& func) const
{
    for (auto it = this->subBlks.cbegin(); it != this->subBlks.cend(); ++it)
    {
        bool b = func(it->first, it->second);
        if (!b)
        {
            return false;
        }
    }

    return true;
}

bool CReaderWriterCziSubBlockDirectory::TryGetSubBlock(int key, SubBlkEntry* entry) const
{
    const auto& it = this->subBlks.find(key);
    if (it == this->subBlks.cend())
    {
        return false;
    }

    if (entry != nullptr)
    {
        *entry = it->second;
    }

    return true;
}

bool CReaderWriterCziSubBlockDirectory::TryModifySubBlock(int key, const SubBlkEntry& entry)
{
    auto it = this->subBlks.find(key);
    if (it == this->subBlks.end())
    {
        return false;
    }

    it->second = entry;
    this->SetModified(true);

    // TODO: Check if coordinates etc. are unmodified, in which case we do not have to invalidate the subblock-statistics
    this->sbBlkStatisticsCurrent = false;
    this->sbBlkStatisticsConsolidated = false;

    return true;
}

bool CReaderWriterCziSubBlockDirectory::TryRemoveSubBlock(int key, SubBlkEntry* entry)
{
    auto it = this->subBlks.find(key);
    if (it == this->subBlks.end())
    {
        return false;
    }

    if (entry != nullptr)
    {
        *entry = it->second;
    }

    this->subBlks.erase(it);
    this->SetModified(true);

    this->sbBlkStatisticsCurrent = false;
    this->sbBlkStatisticsConsolidated = false;

    return true;
}

/// First check whether a subblock already exists (with an equal coordinate); and if so, we return false.
/// Otherwise, we add the subblock and store its index into "key" (if it is non-null).
///
/// \param          entry The entry to add.
/// \param [in,out] key   If non-null, this will receive the key for the newly added enty (if return value is true).
///
/// \return True if it succeeds, false if it fails (because an entry with identical coordinate already exists).
bool CReaderWriterCziSubBlockDirectory::TryAddSubBlock(const SubBlkEntry& entry, int* key)
{
    for (auto it = this->subBlks.cbegin(); it != this->subBlks.cend(); ++it)
    {
        if (CCziSubBlockDirectoryBase::CompareForEquality_Coordinate(it->second, entry))
        {
            return false;
        }
    }

    this->AddSubBlock(entry, key);
    return true;
}

const libCZI::SubBlockStatistics& CReaderWriterCziSubBlockDirectory::GetStatistics()
{
    if (!this->sbBlkStatisticsCurrent)
    {
        this->RecreateSubBlockStatistics();
    }

    return this->sblkStatistics.GetStatistics();
}

const libCZI::PyramidStatistics& CReaderWriterCziSubBlockDirectory::GetPyramidStatistics()
{
    if (!this->sbBlkStatisticsConsolidated)
    {
        this->EnsureSbBlkStatisticsConsolidated();
    }

    return this->sblkStatistics.GetPyramidStatistics();
}

void CReaderWriterCziSubBlockDirectory::EnsureSbBlkStatisticsConsolidated()
{
    if (!this->sbBlkStatisticsCurrent)
    {
        this->RecreateSubBlockStatistics();
    }

    if (!this->sbBlkStatisticsConsolidated)
    {
        this->sblkStatistics.Consolidate();
        this->sbBlkStatisticsConsolidated = true;
    }
}

void CReaderWriterCziSubBlockDirectory::RecreateSubBlockStatistics()
{
    this->sblkStatistics.Clear();
    for (auto it = this->subBlks.cbegin(); it != this->subBlks.cend(); ++it)
    {
        this->sblkStatistics.UpdateStatistics(it->second);
    }

    this->sbBlkStatisticsCurrent = true;
    this->sbBlkStatisticsConsolidated = false;
}
