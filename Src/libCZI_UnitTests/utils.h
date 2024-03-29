// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "inc_libCZI.h"
#include "MemInputOutputStream.h"
#include <tuple>
#include <memory>

/**
 * \brief	Creates a testing bitmap with the given width, height and the pixel type.
 *			The function throws exception if unsupported pixel type is selected.
 * \param	pixeltype	The pixel type. Supports either PixelType::Gray8, PixelType::Gray16,
 *						PixelType::Bgr24 or PixelType::Bgr48. All other types are not supported.
 *						The function throws exception if other pixel type is selected.
 * \param	width		The width of bitmap image in pixels.
 * \param	height		The height of bitmap image in pixels.
 * \return	The pointer of created bitmap data object.
 * \throw	std::runtime_error exception if unsupported pixel type is selected.
 */
std::shared_ptr<libCZI::IBitmapData> CreateTestBitmap(libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height);

/**
 * \brief	Creates a testing bitmap with random pixel values. The image has given width, height and the pixel type.
 *			The function throws exception if unsupported pixel type is selected.
 * \param	pixeltype	The pixel type. Supports either PixelType::Gray8, PixelType::Gray16,
 *						PixelType::Bgr24 or PixelType::Bgr48. All other types are not supported.
 *						The function throws exception if other pixel type is selected.
 * \param	width		The width of bitmap image in pixels.
 * \param	height		The height of bitmap image in pixels.
 * \return	The pointer of created bitmap data object with random pixel values.
 * \throw	std::runtime_error exception if unsupported pixel type is selected or
 *			failed to allocated memory for the image.
 */
std::shared_ptr<libCZI::IBitmapData> CreateRandomBitmap(libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height);


std::shared_ptr<libCZI::IBitmapData> CreateGray8BitmapAndFill(std::uint32_t width, std::uint32_t height, uint8_t value);

/**
 * \brief	Creates an object with Zeiss logo bitmap image, which has fixed size of image width, height and pixel types
 * \return	A bitmap object of Zeiss logo image.
 */
std::shared_ptr<libCZI::IBitmapData> GetZeissLogoBitmap(void);

/**
 * \brief	Checks whether the bitmap data are equal or not.
 *			2 bitmap data are equal if they have same width, height, pixel type and same pixels in each line.
 * \param	bmp1	The bitmap data to check.
 * \param	bmp2	The bitmap data to check.
 * \return	Returns true if bitmaps have same data.
 */
bool AreBitmapDataEqual(const std::shared_ptr<libCZI::IBitmapData>& bmp1, const std::shared_ptr<libCZI::IBitmapData>& bmp2);

/// Compare two bitmaps which must both be of pixel-type  gray-float-32. The maximum allowed difference (per pixel) is given as a parameter.
///
/// \param  bmp1            The first bitmap.
/// \param  bmp2            The second bitmap.
/// \param  max_difference  The maximum difference.
///
/// \returns    True if both bitmaps are equal (i.e. the difference for all pixels is less than the specified value); false otherwise.
bool CompareGrayFloat32Bitmaps(const std::shared_ptr<libCZI::IBitmapData>& bmp1, const std::shared_ptr<libCZI::IBitmapData>& bmp2, float max_difference);

/// Calculates the maximum difference and the mean difference of the pixel values of the two bitmaps.
/// The two bitmaps must have the same size and pixel type, otherwise an exception is thrown.
///
/// \param  bmp1    The first bitmap.
/// \param  bmp2    The second bitmap.
///
/// \returns    The maximum difference and the mean difference of the pixel values of the two bitmaps.
std::tuple<float,float> CalculateMaxDifferenceMeanDifference(const std::shared_ptr<libCZI::IBitmapData>& bmp1, const std::shared_ptr<libCZI::IBitmapData>& bmp2);

void WriteOutTestCzi(const char* testcaseName, const char* testname, const void* ptr, size_t size);

void WriteOutTestCzi(const char* testcaseName, const char* testname, const std::shared_ptr<CMemInputOutputStream>& str);

template<typename input_iterator>
void CalcHash(std::uint8_t* ptrHash, input_iterator begin, input_iterator end)
{
    memset(ptrHash, 0, 16);
    for (input_iterator it = begin; it != end; ++it)
    {
        std::uint8_t hash[16];
        libCZI::Utils::CalcMd5SumHash(it->c_str(), it->length(), hash, 16);
        for (uint8_t i = 0; i < 16; ++i)
        {
            *(ptrHash + i) ^= hash[i];
        }
    }
}

