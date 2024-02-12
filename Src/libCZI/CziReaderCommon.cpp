// SPDX-FileCopyrightText: 2024 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CziReaderCommon.h"

#include "CziUtils.h"
#include "utilities.h"

using namespace std;
using namespace libCZI;

/*static*/void CziReaderCommon::EnumSubset(
    libCZI::ISubBlockRepository* repository,
    const libCZI::IDimCoordinate* planeCoordinate,
    const libCZI::IntRect* roi,
    bool onlyLayer0,
    const std::function<bool(int index, const libCZI::SubBlockInfo& info)>& funcEnum)
{
    // Ok... for a first tentative, experimental and quick-n-dirty implementation, simply
    //      walk through all the subblocks. We surely want to have something more elaborated
    //      here.
    repository->EnumerateSubBlocks(
        [&](int index, const SubBlockInfo& info)->bool
        {
            if (onlyLayer0 == false || (info.physicalSize.w == info.logicalRect.w && info.physicalSize.h == info.logicalRect.h))
            {
                if (planeCoordinate == nullptr || CziUtils::CompareCoordinate(planeCoordinate, &info.coordinate) == true)
                {
                    if (roi == nullptr || Utilities::DoIntersect(*roi, info.logicalRect))
                    {
                        const bool b = funcEnum(index, info);
                        return b;
                    }
                }
            }

            return true;
        });
}

/*static*/bool CziReaderCommon::TryGetSubBlockInfoOfArbitrarySubBlockInChannel(
    libCZI::ISubBlockRepository* repository,
    int channelIndex,
    libCZI::SubBlockInfo& info)
{
    bool foundASubBlock = false;
    SubBlockStatistics s = repository->GetStatistics();
    if (!s.dimBounds.IsValid(DimensionIndex::C))
    {
        // in this case -> just take the first subblock...
        repository->EnumerateSubBlocks(
            [&](int index, const SubBlockInfo& sbinfo)->bool
            {
                info = sbinfo;
                foundASubBlock = true;
                return false;
            });
    }
    else
    {
        repository->EnumerateSubBlocks(
            [&](int index, const SubBlockInfo& sbinfo)->bool
            {
                int c;
                if (sbinfo.coordinate.TryGetPosition(DimensionIndex::C, &c) == true && c == channelIndex)
                {
                    info = sbinfo;
                    foundASubBlock = true;
                    return false;
                }

                return true;
            });
    }

    return foundASubBlock;
}

/*static*/void CziReaderCommon::EnumerateSubset(
    const std::function<void(const std::function<bool(int index, const CCziAttachmentsDirectory::AttachmentEntry&)>&)>& func,
    const char* contentFileType,
    const char* name,
    const std::function<bool(int index, const libCZI::AttachmentInfo& info)>& funcEnum)
{
    libCZI::AttachmentInfo ai;
    ai.contentFileType[sizeof(ai.contentFileType) - 1] = '\0';
    func(
        [&](int index, const CCziAttachmentsDirectoryBase::AttachmentEntry& ae)
        {
            if (contentFileType == nullptr || strcmp(contentFileType, ae.ContentFileType) == 0)
            {
                if (name == nullptr || strcmp(name, ae.Name) == 0)
                {
                    ai.contentGuid = ae.ContentGuid;
                    memcpy(ai.contentFileType, ae.ContentFileType, sizeof(ae.ContentFileType));
                    ai.name = ae.Name;
                    bool b = funcEnum(index, ai);
                    return b;
                }
            }

            return true;
        });
}

/*static*/libCZI::SubBlockInfo CziReaderCommon::ConvertToSubBlockInfo(const CCziSubBlockDirectory::SubBlkEntry& entry)
{
    SubBlockInfo info;
    info.compressionModeRaw = entry.Compression;
    info.pixelType = CziUtils::PixelTypeFromInt(entry.PixelType);
    info.coordinate = entry.coordinate;
    info.logicalRect = IntRect{ entry.x,entry.y,entry.width,entry.height };
    info.physicalSize = IntSize{ (std::uint32_t)entry.storedWidth, (std::uint32_t)entry.storedHeight };
    info.mIndex = entry.mIndex;
    info.pyramidType = CziUtils::PyramidTypeFromByte(entry.pyramid_type_from_spare);
    return info;
}
