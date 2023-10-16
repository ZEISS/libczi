// SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <functional>
#include <memory>
#include <tuple>
#include <cstdint>
#include <sstream>

/// This class encapsulates the JXR codec.
class JxrDecode
{
public:
    /// Values that represent the pixel formats supported by the codec.
    enum class PixelFormat
    {
        kInvalid,       ///< An enum constant representing the invalid option
        kBgr24,         ///< An enum constant representing the "BGR24" pixel type - 3 bytes per pixel, 8 bits per channel, in B-G-R order.
        kBgr48,         ///< An enum constant representing the "BGR48" pixel type - 6 bytes per pixel, 16 bits per channel, in B-G-R order. Note that 
                        ///< there BGR48 is not natively supported by the codec, so it is converted to RGB48. This happens transparently to the user,
                        ///< but at this point there is a performance penalty.
        kGray8,         ///< An enum constant representing the "GRAY8" pixel type - 1 byte per pixel, 8 bits per channel.
        kGray16,        ///< An enum constant representing the "GRAY16" pixel type - 2 bytes per pixel, 16 bits per channel.
        kGray32Float,   ///< An enum constant representing the "GRAY32Float" pixel type - 4 bytes per pixel, 32 bits per channel.
    };

    /// This class is used to represent a blob containing the compressed data.
    /// Note that it is movable but not copyable.
    class CompressedData
    {
    private:
        void* obj_handle_;
    public:
        CompressedData() :obj_handle_(nullptr) {} // default constructor

        /// This method moves a CompressedData object into the current object via swapping.
        ///
        /// \param other     The CompressedData object to move
        ///
        /// \returns         A reference to the current object after the move has been completed.
        CompressedData& operator=(CompressedData&& other) noexcept
        {
            // "other" is soon going to be destroyed, so we let it destroy our current resource instead and we take "other"'s current resource via swapping
            std::swap(this->obj_handle_, other.obj_handle_);
            return *this;
        }

        /// This method moves a CompressedData object into the current object via swapping.
        ///
        /// \param other     The CompressedData object to move
        CompressedData(CompressedData&& other) noexcept
        {
            // we "steal" the resource from "other"
            this->obj_handle_ = other.obj_handle_;
            // "other" will soon be destroyed and its destructor will do nothing because we null out its resource here
            other.obj_handle_ = nullptr;
        }

        CompressedData(const CompressedData&) = delete; // prevent copy constructor to be used
        CompressedData& operator=(const CompressedData&) = delete; // prevent copy assignment to be used

        /// Gets a pointer to the memory representing the compressed data. The memory is owned by the object and will be freed when the object is destroyed.
        ///
        /// \returns    A pointer to the memory representing the compressed data.
        void* GetMemory() const;

        /// Gets the size of the compressed data in units of bytes.
        ///
        /// \returns    The size (in bytes).
        size_t GetSize() const;

        ~CompressedData();
    protected:
        friend class JxrDecode;
        CompressedData(void* obj_handle) :obj_handle_(obj_handle) {}
    };

    /// Decodes the specified data, giving an uncompressed bitmap.
    /// The course of action is as follows:
    /// * The decoder will be initialized with the specified compressed data.  
    /// * The characteristics of the compressed data will be determined - i.e the pixel type, width, height, etc. are determined.  
    /// * The 'get_destination_func' is called, passing in the pixel type, width, and height (as determined in the step before).    
    /// * The 'get_destination_func' function  now is to check whether it can receive the bitmap (as reported). If it cannot, it   
    ///   must throw an exception (which will be propagated to the caller). Otherwise, it must return a tuple, containing
    ///   a pointer to a buffer that can hold the uncompressed bitmap and the stride. This buffer must remain valid until the
    ///   the function returns - either normally or by throwing an exception.
    /// Notes:
    /// * The decoder will call the 'get_destination_func' function only once, and the buffer must be valid  
    ///   until the method returns.
    /// * The 'get_destination_func' function may choose to throw an exception (if the memory cannot be allocated,  
    ///   or the reported characteristics are determined to be invalid, etc). 
    ///
    /// \param  ptrData                 Information describing the pointer.
    /// \param  size                    The size.
    /// \param  get_destination_func    The get destination function.
    static void Decode(
            const void* ptrData,
            size_t size,
            const std::function<std::tuple<void*/*destination_bitmap*/, std::uint32_t/*stride*/>(PixelFormat pixel_format, std::uint32_t  width, std::uint32_t  height)>& get_destination_func);

    /// Compresses the specified bitmap into the JXR (aka JPEG XR) format.
    /// 
    /// \param pixel_format     The pixel type.
    /// \param width            The width of the bitmap in pixels.
    /// \param height           The height of the bitmap in pixels.
    /// \param stride           The stride of the bitmap in bytes.
    /// \param ptr_bitmap       A pointer to the uncompressed bitmap data.
    /// \param quality          A parameter that controls the quality of the compression. This is a number between 0 and 1,
    ///                         and the higher the number, the better the quality. The default is 1. A value of 1 gives the best
    ///                         possible quality which is loss-less.
    ///
    /// \returns                A CompressedData object containing the compressed data.
    static CompressedData Encode(
            JxrDecode::PixelFormat pixel_format,
            std::uint32_t  width,
            std::uint32_t  height,
            std::uint32_t  stride,
            const void* ptr_bitmap,
            float quality = 1.f);
private:
    static void ThrowJxrlibError(const std::string& message, int error_code);
    [[noreturn]] static void ThrowJxrlibError(std::ostringstream& message, int error_code);
    static std::uint8_t GetBytesPerPel(PixelFormat pixel_format);
};
