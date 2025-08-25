#pragma once

#include <cstdint>
#include <stdexcept>

#include "libCZI_Pixels.h"

class BitmapOperationsBitonal
{
public:
    /// Get a value indicating if the pixel specified by its x and y coordinate is true or false.
    /// If x or y exceeds the size of the bitmap, an exception is thrown.
    ///
    /// \param 	x	   	The x coordinate of the pixel to query.
    /// \param 	y	   	The y coordinate of the pixel to query.
    /// \param 	width  	The width of the bitonal bitmap.
    /// \param 	height 	The height of the bitonal bitmap.
    /// \param 	ptrData	Pointer to the start of the bitonal bitmap data.
    /// \param 	stride 	The stride (in units of bytes).
    ///
    /// \returns	True if the pixel is "1", false if the pixel is "0".
    static bool GetPixelFromBitonal(std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height, const void* ptrData, std::uint32_t stride)
    {
        if (x >= width || y >= height)
        {
            throw std::out_of_range("Coordinates out of bounds.");
        }

        const std::uint8_t* ptr = static_cast<const std::uint8_t*>(ptrData) + static_cast<size_t>(y) * stride + (x / 8);
        return (*ptr & (1 << (7 - (x % 8)))) != 0;
    }


    /// Decimates the bitonal image by a factor of two. A bit in the destination bitmap is set if all
    /// bits in a neighborhood with size specified by region_size are set in the source bitmap.
    ///
    /// \param 		   	region_size	Size of the neighborhood - must be greater or equal to 0 and less than 8. Otherwise an exception is thrown.
    /// \param 		   	mask_src   	The bitonal source bitmap.
    /// \param 		   	stride_src 	The stride of the bitonal source in units of bytes.
    /// \param 		   	width_src  	The width of the bitonal source.
    /// \param 		   	height_src 	The height of the bitonal source.
    /// \param [out]	mask_dest  	The bitonal destination mask.
    /// \param 		   	stride_dest	The stride of the bitonal destination in units of bytes.
    /// \param 		   	width_dest 	The width of the destination.
    /// \param 		   	height_dest	The height of the destination.
    static void BitonalDecimate(
        int region_size, 
        const std::uint8_t* mask_src, 
        int stride_src, 
        int width_src, 
        int height_src, 
        std::uint8_t* mask_dest, 
        int stride_dest, 
        int width_dest, 
        int height_dest);

    static void NNResizeMaskAware(
        libCZI::IBitmapData* bmSrc,
        libCZI::IBitonalBitmapData* bmSrcMask, 
        libCZI::IBitmapData* bmDest,
        const libCZI::DblRect& roiSrc, 
        const libCZI::DblRect& roiDst);
};
