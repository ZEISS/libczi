// SPDX-FileCopyrightText: 2024 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "libCZI.h"
#include "CziAttachment.h"
#include <functional>

/// Here we gather some common functionality that is used by both the CziReader and the CziReaderWriter classes.
class CziReaderCommon
{
public:
    static void EnumSubset(
        libCZI::ISubBlockRepository* repository,
        const libCZI::IDimCoordinate* planeCoordinate,
        const libCZI::IntRect* roi,
        bool onlyLayer0,
        const std::function<bool(int index, const libCZI::SubBlockInfo& info)>& funcEnum);

    static bool TryGetSubBlockInfoOfArbitrarySubBlockInChannel(
        libCZI::ISubBlockRepository* repository,
        int channelIndex, 
        libCZI::SubBlockInfo& info);

    static void EnumerateSubset(
        const std::function<void(const std::function<bool(int index, const CCziAttachmentsDirectory::AttachmentEntry&)>&)>& func,
        const char* contentFileType,
        const char* name,
        const std::function<bool(int index, const libCZI::AttachmentInfo& info)>& funcEnum);

    static libCZI::SubBlockInfo ConvertToSubBlockInfo(const CCziSubBlockDirectory::SubBlkEntry& entry);
};
