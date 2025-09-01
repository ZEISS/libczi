#pragma once

#include <cstdint>
#include <stdexcept>

#include "libCZI_Pixels.h"
#include "BitmapOperations.h"

/// Here we gather operations that are specific to bitonal bitmaps.
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
        return (*ptr & 1 << (7 - x % 8)) != 0;
    }

    /// Sets pixel in bitonal bitmap to the specified value.
    /// If x or y exceeds the size of the bitmap, an exception is thrown.
    ///
    /// \param 	x	   	The x coordinate of the pixel to query.
    /// \param 	y	   	The y coordinate of the pixel to query.
    /// \param 	width  	The width of the bitonal bitmap.
    /// \param 	height 	The height of the bitonal bitmap.
    /// \param 	ptrData	Pointer to the start of the bitonal bitmap data.
    /// \param 	stride 	The stride (in units of bytes).
    /// \param  value  	The value to set the pixel to.
    static void SetPixelInBitonal(std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height, void* ptrData, std::uint32_t stride, bool value)
    {
        if (x >= width || y >= height)
        {
            throw std::out_of_range("Coordinates out of bounds.");
        }

        std::uint8_t* ptr = static_cast<std::uint8_t*>(ptrData) + static_cast<size_t>(y) * stride + (x / 8);
        if (value)
        {
            *ptr |= 1 << (7 - x % 8);
        }
        else
        {
            *ptr &= ~(1 << (7 - x % 8));
        }
    }

    /// Get a value indicating if the pixel specified by its x and y coordinate is true or false.
    /// This version does not check whether x or y exceeds the size of the bitmap (so it is the caller's
    /// responsibility to ensure that x and y are within bounds).
    ///
    /// \param 	x	   	The x coordinate of the pixel to query.
    /// \param 	y	   	The y coordinate of the pixel to query.
    /// \param 	ptrData	Pointer to the start of the bitonal bitmap data.
    /// \param 	stride 	The stride (in units of bytes).
    ///
    /// \returns	True if the pixel is "1", false if the pixel is "0".
    static bool GetPixelFromBitonalUnchecked(std::uint32_t x, std::uint32_t y, const void* ptrData, std::uint32_t stride)
    {
        const std::uint8_t* ptr = static_cast<const std::uint8_t*>(ptrData) + static_cast<size_t>(y) * stride + (x / 8);
        return (*ptr & (1 << (7 - (x % 8)))) != 0;
    }

    /// Sets pixel in bitonal bitmap to the specified value.
    /// This version does not check whether x or y exceeds the size of the bitmap (so it is the caller's
    /// responsibility to ensure that x and y are within bounds).
    ///
    /// \param 	x	   	The x coordinate of the pixel to query.
    /// \param 	y	   	The y coordinate of the pixel to query.
    /// \param 	ptrData	Pointer to the start of the bitonal bitmap data.
    /// \param 	stride 	The stride (in units of bytes).
    /// \param  value  	The value to set the pixel to.
    static void SetPixelInBitonalUnchecked(std::uint32_t x, std::uint32_t y, void* ptrData, std::uint32_t stride, bool value)
    {
        std::uint8_t* ptr = static_cast<std::uint8_t*>(ptrData) + static_cast<size_t>(y) * stride + (x / 8);
        if (value)
        {
            *ptr |= (1 << (7 - (x % 8)));
        }
        else
        {
            *ptr &= ~(1 << (7 - (x % 8)));
        }
    }

    /// Fills a rectangular region of interest (ROI) in a bitonal bitmap with a specified value.
    ///
    /// \param 		   	width  	The width of the bitmap in pixels.
    /// \param 		   	height 	The height of the bitmap in pixels.
    /// \param      	ptrData	Pointer to the bitmap data.
    /// \param 		   	stride 	The stride (in units of bytes).
    /// \param 		   	roi	   	The region of interest to fill. The ROI is clipped to the bitmap extent.
    /// \param 		   	value  	The value to fill the ROI with.
    static void Fill(std::uint32_t width, std::uint32_t height, void* ptrData, std::uint32_t stride, const libCZI::IntRect& roi, bool value);

    /// Sets all bits in the specified bitonal bitmap to the specified value.
    ///
    /// \param 		   	width  	The width.
    /// \param 		   	height 	The height.
    /// \param          ptrData	Pointer to the start of the bitonal bitmap data.
    /// \param 		   	stride  The stride (in units of bytes).
    /// \param 		   	value  	The value to set all pixels to.
    static void Set(std::uint32_t width, std::uint32_t height, void* ptrData, std::uint32_t stride, bool value);

    /// Decimates the bitonal image by a factor of two. A bit in the destination bitmap is set if all
    /// bits in a neighborhood with size specified by region_size are set in the source bitmap.
    ///
    /// \param 		   	region_size	Size of the neighborhood - must be greater or equal to 0 and less than 8. Otherwise, an exception is thrown.
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

    /// This structure gathers the information needed to copy a source bitmap with a specified mask into
    /// a destination bitmap at a specified offset.
    struct CopyWithOffsetAndMaskInfo : CBitmapOperations::CopyWithOffsetInfo
    {
        const void* maskPtr;                 ///< Pointer to the mask bitmap (may be null, in which case all pixels are considered valid).
        int maskStride;                      ///< The stride of the mask bitmap in bytes.
        int maskWidth;                       ///< The width of the mask bitmap in pixels. If the width of the mask bitmap is less than the width of the source bitmap, the pixels
                                             ///< in the source bitmap for which there is no corresponding pixel in the mask bitmap are considered invalid (i.e. not copied).
        int maskHeight;                      ///< The height of the mask bitmap in pixels. If the height of the mask bitmap is less than the height of the source bitmap, the pixels
                                             ///< in the source bitmap for which there is no corresponding pixel in the mask bitmap are considered invalid (i.e. not copied).
    };

    /// Copies the specified source bitmap into the specified destination bitmap at the specified offset, using the specified mask.
    /// If the mask pointer is null, the function behaves like CBitmapOperations::CopyWithOffset.
    /// If the width (or height) of the mask is less than the width (or height) of the source bitmap, the pixels
    /// in the source bitmap for which there is no corresponding pixel in the mask bitmap are considered invalid (i.e. not copied).
    ///
    /// \param 	info	The information.
    static void CopyWithOffsetAndMask(const CopyWithOffsetAndMaskInfo& info);

    static void Copy(
            libCZI::PixelType srcPixelType, 
            const void* srcPtr, 
            int srcStride, 
            libCZI::PixelType dstPixelType, 
            void* dstPtr, 
            int dstStride, 
            int width, 
            int height,
            const void* src_mask_ptr,
            int src_mask_stride,
            int mask_offset_x,
            int mask_offset_y,
            bool drawTileBorder);

};
