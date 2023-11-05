#pragma once

#include <memory>
#include "../libCZI/libCZI.h"

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
    static std::shared_ptr<ISaveBitmap> CreateDefaultSaveBitmapObj();
};
