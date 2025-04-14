// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <sstream>
#include <memory>
#include <string>
#include "libCZI_Pixels.h"

namespace libCZI
{
    /// Values that represent image decoder types - used for distinguishing decoder objects
    /// created by `ISite::GetDecoder`.
    enum class ImageDecoderType
    {
        JPXR_JxrLib,    ///< Identifies a decoder capable of decoding a JPG-XR compressed image.

        ZStd0,          ///< Identifies a decoder capable of decoding a zstd compressed image (type "zstd0").

        ZStd1           ///< Identifies a decoder capable of decoding a zstd compressed image (type "zstd1").
    };

    class IBitmapData;

    /// The interface used for operating image decoder. That is the simplest possible interface at this point...
    class IDecoder
    {
    public:
        /// Passing in a block of raw data, decode the image and return a bitmap object.
        /// \remark
        /// This method is intended to be called concurrently, implementors should make no assumption
        /// about concurrency.
        /// The parameters `pixelType`, `width` and `height` are used for validation purposes only. The decoder
        /// is expected to check whether the data passed in is of the expected type and size. If not, it should throw an exception.
        /// If instead nullptr is passed for any of `pixelType`, `width` or `height`, then this means that this parameter
        /// is not to be validated. The decoder is expected to decode the data and return a bitmap object.
        ///
        /// \remark
        /// In case of an error (of whatever kind) the method is expected to throw an exception.
        /// 
        /// \param ptrData              Pointer to a block of memory (which contains the encoded image).
        /// \param size                 The size of the memory block pointed by `ptrData`.
        /// \param pixelType            If non-null, the pixel type of the expected bitmap.
        /// \param width                If non-null, the width of the expected bitmap.
        /// \param height               If non-null, the height of the expected bitmap.
        /// \param additional_arguments If non-null, additional arguments for the decoder. This is a null-terminated string, where the syntax is class-specific.
        ///
        /// \return A bitmap object with the decoded data.
        virtual std::shared_ptr<libCZI::IBitmapData> Decode(const void* ptrData, size_t size, const libCZI::PixelType* pixelType, const std::uint32_t* width, const std::uint32_t* height, const char* additional_arguments = nullptr) = 0;

        virtual ~IDecoder() = default;

        /// Decodes the specified data and returns a bitmap object. This is a convenience method, where the parameters
        /// `pixelType`, `width` and `height` are passed in as references and as required parameters (as opposed to optional).
        ///
        /// \param  ptrData             Pointer to a block of memory (which contains the encoded image).
        /// \param  size                The size of the memory block pointed by `ptrData`.
        /// \param  pixelType           The pixel type which is expected.
        /// \param  width               The width which is expected.
        /// \param  height              The height which is expected.
        /// \param additional_arguments If non-null, additional arguments for the decoder. This is a null-terminated string, where the syntax is defined class specifically.
        ///
        /// \return A bitmap object with the decoded data.
        std::shared_ptr<libCZI::IBitmapData> Decode(const void* ptrData, size_t size, libCZI::PixelType pixelType, std::uint32_t width, std::uint32_t height, const char* additional_arguments = nullptr)
        {
            return this->Decode(ptrData, size, &pixelType, &width, &height, additional_arguments);
        }
    };

    const int LOGLEVEL_CATASTROPHICERROR = 0;   ///< Identifies a catastrophic error (i.e. the program cannot continue).
    const int LOGLEVEL_ERROR = 1;               ///< Identifies a non-recoverable error.
    const int LOGLEVEL_SEVEREWARNING = 2;       ///< Identifies that a severe problem has occured. Proper operation of the module is not ensured.
    const int LOGLEVEL_WARNING = 3;             ///< Identifies that a problem has been identified. It is likely that proper operation can be kept up.
    const int LOGLEVEL_INFORMATION = 4;         ///< Identifies an informational output. It has no impact on the proper operation.
    const int LOGLEVEL_CHATTYINFORMATION = 5;   ///< Identifies an informational output which has no impact on proper operation. Use this for output which may occur with high frequency.

    /// Interface for the Site-object. It is intended for customizing the library (by injecting a
    /// custom implementation of this interface).
    class ISite
    {
    public:
        /// Query if  the specified logging level is enabled. In the case that constructing the message
        /// to be logged takes a significant amount of resources (i.e. time or memory), this method should
        /// be called before in order to determine whether the output is required at all. This also means
        /// that this method may be called very frequently, so implementors should take care that it executes
        /// reasonably fast.
        ///
        /// \param logLevel The logging level.
        ///
        /// \return True if the specified logging level is enabled, false otherwise.
        virtual bool IsEnabled(int logLevel) = 0;

        /// Output the specified string at the specified logging level.
        /// \remark
        /// The text is assumed to be ASCII - not UTF8 or any other codepage. Use only plain-ASCII.
        /// This might change...
        /// \param level The logging level.
        /// \param szMsg The message to be logged.
        virtual void Log(int level, const char* szMsg) = 0;

        /// Gets a decoder object.
        ///
        /// \param type      The type.
        /// \param arguments The arguments.
        ///
        /// \return The decoder object.
        virtual std::shared_ptr<IDecoder> GetDecoder(ImageDecoderType type, const char* arguments) = 0;

        /// Creates a bitmap object. All internal bitmap allocations are done with this method, and overloading
        /// this method allows to use an externally controlled memory management to be injected.
        ///
        /// \param pixeltype    The pixeltype of the newly allocated bitmap.
        /// \param width        The width of the newly allocated bitmap.
        /// \param height       The height of the newly allocated bitmap.
        /// \param stride       The stride of the newly allocated bitmap. If <= 0, then the method may choose an appropriate stride
        ///                     on its own. If a stride >0 is given here, then we expect that the newly created bitmap adheres to it.
        /// \param extraRows    The extra rows (not currently used, will always be 0).
        /// \param extraColumns The extra columns  (not currently used, will always be 0).
        ///
        /// \return The newly allocated bitmap.
        virtual std::shared_ptr<libCZI::IBitmapData> CreateBitmap(libCZI::PixelType pixeltype, std::uint32_t width, std::uint32_t height, std::uint32_t stride = 0, std::uint32_t extraRows = 0, std::uint32_t extraColumns = 0) = 0;

        /// Output the specified string at the specified logging level.
        /// \param level The level.
        /// \param str   The string.
        void Log(int level, const std::string& str)
        {
            this->Log(level, str.c_str());
        }

        /// Output the specified stringstream object at the specified logging level.
        /// \param level       The level.
        /// \param [in] ss The stringstream object.
        void Log(int level, std::stringstream& ss)
        {
            this->Log(level, ss.str());
        }
    };
}
