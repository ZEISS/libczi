// SPDX-FileCopyrightText: 2017-2022 Carl Zeis-s Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <cstdint>
#include <cstddef>

class CTestImage
{
public:
    static const std::uint32_t BGR24TESTIMAGE_WIDTH = 1024;
    static const std::uint32_t BGR24TESTIMAGE_HEIGHT = 768;
    static void CopyBgr24Image(void* pDest, std::uint32_t width, std::uint32_t height, int stride);
    
    static const void* GetJpgXrCompressedImage_Bgr24(size_t* size, int* width, int* height);
    static const void* GetJpgXrCompressedImage_Gray8(size_t* size, int* width, int* height);
    static const void* GetZStd1CompressedImage(size_t* size, int* width, int* height);
    static const void* GetZStd1CompressedImageWithHiLoPacking(size_t* size, int* width, int* height);
};

