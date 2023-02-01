// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "inc_libCZI.h"

enum class SaveDataFormat
{
    Invalid,
    PNG
};

class ISaveBitmap
{
public:
    virtual void Save(const wchar_t* fileName, SaveDataFormat dataFormat, libCZI::IBitmapData* bitmap) = 0;
    virtual ~ISaveBitmap() = default;
};

class CSaveBitmapFactory
{
public:
    static const char WIC_CLASS[];
    static const char LIBPNG_CLASS[];

    static std::shared_ptr<ISaveBitmap> CreateSaveBitmapObj(const char* className);
    static std::shared_ptr<ISaveBitmap> CreateDefaultSaveBitmapObj();
};
