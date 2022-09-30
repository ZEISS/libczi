// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <memory>
#include <cstdint>
#include <functional>
#include <type_traits>
#include "libCZI.h"

namespace libCZI
{
    /// Values that represent well-known keys for the compression-parameters property bag. Note that
    /// the property-bag API is modelled with an int as key, which is by intention in order to allow
    /// for private keys.
    enum class CompressionParameterKey
    {
        /// This gives the "raw" zstd compression level aka "ExplicitLevel" (type: int32). If value is out-of-range, it will be clipped.
        /// This parameter is used with "zstd0" and "zstd1" compression schemes.
        ZSTD_RAWCOMPRESSIONLEVEL = 1,   

        /// Whether to do the "lo-hi-byte-packing" preprocessing (type: boolean).
        /// This parameter is used with the "zstd1" compression scheme only.
        ZSTD_PREPROCESS_DOLOHIBYTEPACKING = 2 
    };

    /// Simple variant type used for the compression-parameters-property-bag.
    struct LIBCZI_API CompressParameter
    {
        /// Values that represent the type represented by this variant.
        enum class Type
        {
            Invalid,    ///< An enum constant representing the 'invalid' type (so this instance has no value).
            Int32,      ///< An enum constant representing the 'int32' type.
            Uint32,     ///< An enum constant representing the 'uint32' type.
            Boolean     ///< An enum constant representing the 'boolean' type.
        };

        /// Default constructor - setting the variant to 'invalid'.
        CompressParameter() : type(Type::Invalid)
        {
        }

        /// Constructor for initializing the 'int32' type.
        ///
        /// \param  v   The value to set the variant to.
        CompressParameter(std::int32_t v)
        {
            this->SetInt32(v);
        }

        /// Constructor for initializing the 'uint32' type.
        ///
        /// \param  v   The value to set the variant to.
        CompressParameter(std::uint32_t v)
        {
            this->SetUInt32(v);
        }

        /// Constructor for initializing the 'bool' type.
        ///
        /// \param  v   The value to set the variant to.
        CompressParameter(bool v)
        {
            this->SetBoolean(v);
        }

        /// Sets the type of the variant to "int32" and the value to the specified value.
        ///
        /// \param  v   The value to be set.
        void SetInt32(std::int32_t v)
        {
            this->type = Type::Int32;
            this->int32Value = v;
        }

        /// Sets the type of the variant to "uint32" and the value to the specified value.
        ///
        /// \param  v   The value to be set.
        void SetUInt32(std::uint32_t v)
        {
            this->type = Type::Uint32;
            this->uint32Value = v;
        }

        /// Sets the type of the variant to "boolean" and the value to the specified value.
        ///
        /// \param  v   The value to be set.
        void SetBoolean(bool v)
        {
            this->type = Type::Boolean;
            this->boolValue = v;
        }

        /// Gets the type which is represented by the variant.
        ///
        /// \returns    The type.
        Type GetType() const { return this->type; }

        /// If the type of the variant is "Int32", then this value is returned. Otherwise, an exception (of type "runtime_error") is thrown.
        ///
        /// \returns    The value of the variant (of type "Int32").
        std::int32_t GetInt32() const
        {
            this->ThrowIfTypeIsUnequalTo(Type::Int32);
            return this->int32Value;
        }

        /// If the type of the variant is "Uint32", then this value is returned. Otherwise, an exception (of type "runtime_error") is thrown.
        ///
        /// \returns    The value of the variant (of type "Uint32").
        std::uint32_t GetUInt32() const
        {
            this->ThrowIfTypeIsUnequalTo(Type::Uint32);
            return this->uint32Value;
        }

        /// If the type of the variant is "Boolean", then this value is returned. Otherwise, an exception (of type "runtime_error") is thrown.
        ///
        /// \returns    The value of the variant (of type "Boolean").
        bool GetBoolean() const
        {
            this->ThrowIfTypeIsUnequalTo(Type::Boolean);
            return this->boolValue;
        }
    private:
        Type type;  ///< The type which is represented by the variant.

        union
        {
            std::int32_t int32Value;
            std::uint32_t uint32Value;
            bool boolValue;
        };

        void ThrowIfTypeIsUnequalTo(Type typeToCheck) const
        {
            if (this->type != typeToCheck)
            {
                throw std::runtime_error("Unexpected type encountered.");
            }
        }
    };

    /// This interface is used for representing "compression parameters". It is a simply property bag.
    /// Possible values for the key are defined in the "CompressionParameter" class.
    class LIBCZI_API ICompressParameters
    {
    public:
        /// Attempts to get the property for the specified key from the property bag.
        ///
        /// \param          key         The key.
        /// \param [in,out] parameter   If non-null and the key is found, then the value is put here.
        ///
        /// \returns    True if the key is found in the property bag; false otherwise.
        virtual bool TryGetProperty(int key, CompressParameter* parameter) const = 0;

        virtual ~ICompressParameters() = default;

        /// Attempts to get the property for the specified key from the property bag. This helper is
        /// casting the enum to int, facilitating the use with the enum type.
        ///
        /// \param          key         The key.
        /// \param [in,out] parameter   If non-null and the key is found, then the value is put here.
        ///
        /// \returns    True if the key is found in the property bag; false otherwise.
        bool TryGetProperty(libCZI::CompressionParameterKey key, CompressParameter* parameter) const
        {
            return this->TryGetProperty(static_cast<typename std::underlying_type<libCZI::CompressionParameterKey>::type>(key), parameter);
        }
    };

    /// Interface representing a "block of memory". It is used to hold the result of a compression-operation.
    class LIBCZI_API IMemoryBlock
    {
    public:
        /// Gets pointer to the memory block. This memory is owned by this object instance
        /// (i. e. the memory is valid as long as this object lives). The size of this
        /// memory block is given by "GetSizeOfData".
        ///
        /// \returns    Pointer to the memory block.
        virtual void* GetPtr() = 0;

        /// Gets size of the data (for which a pointer can be retrieved by calling "GetPtr"). 
        ///
        /// \returns    The size of data in bytes.
        virtual size_t GetSizeOfData() const = 0;

        virtual ~IMemoryBlock() = default;
    };

    /// The functions found here deal with zstd-compression (the compression-part in particular).
    /// TThose functions are rather low-level, and the common theme is - given a source bitmap, create a blob
    /// (containing the compressed bitmap data) which is suitable to be placed in a subblock's data.
    /// Several overloads are provided, for performance critical scenarios we provide functions which write
    /// directly into caller-provided memory, and there are versions which use caller-provided functions for
    /// internal allocations. The latter may be beneficial in high-performance scenarios where pre-allocation
    /// and buffer-reuse can be leveraged in order to avoid repeated heap-allocations.
    class LIBCZI_API ZstdCompress
    {
    public:
        /// Calculates the maximum size which might be required (for the output buffer) when calling
        /// into "CompressZStd0". The guarantee here is : if calling into "CompressZStd0" with an
        /// output buffer of the size as determined here, the call will NEVER fail (for insufficient
        /// output buffer size). Note that this upper limit may be larger than the actual needed size
        /// by a huge factor (10 times or more), and it is of the order of the input size.
        ///
        /// \param  sourceWidth     The width of the bitmap in pixels.
        /// \param  sourceHeight    The height of the bitmap in pixels.
        /// \param  sourcePixeltype The pixeltype of the bitmap.
        ///
        /// \returns    The calculated maximum compressed size.
        static size_t CalculateMaxCompressedSizeZStd0(std::uint32_t sourceWidth, std::uint32_t sourceHeight, libCZI::PixelType sourcePixeltype);

        /// Compress the specified bitmap in "zstd0"-format. This method will compress the specified source-bitmap according to the "ZEN-zstd0-scheme" to
        /// a caller supplied block of memory. If successful, the used size of the memory block is returned, and the data is suitable to be put into a subblock. 
        /// Details of the operation are:
        /// - (under certain conditions) a temporary buffer is required, the size of this temporary buffer is width*size_of_pixel*height. This method allows to pass   
        ///    in functions for allocating/freeing this temp-buffer. For performance reasons, some type of buffer-pooling or reuse can be applied here.
        /// - A pointer to an output buffer must be supplied, and its size is to be given. The required size of the output buffer is in general not known (and  
        ///    not knowable) beforehand. It is only possible to query an upper limit for the output-buffer (CalculateMaxCompressedSizeZStd1). If the output buffer
        ///    size is insufficient, this method return 'false'. 
        /// - On input, the parameter 'sizeDestination' gives the size of the output buffer; on return of the function, the value is overwritten with the actual  
        ///     used size (which is always less than the size on input).
        /// - There are only two possible outcomes of this function - either the operation completed successfully and returns true, the data is put into the output buffer and  
        ///     the argument 'sizeDestination' gives the used size in the output buffer. Or, false is returned, meaning that the output buffer size is found to be
        ///     insufficient - however, note that the required size is not given, so 'sizeDestination' is unchanged in this case (and there is no indication about 
        ///     how big an output buffer is required).
        /// - All other error conditions (like e. g. invalid arguments) result in an exception being thrown.
        /// \param          sourceWidth         Width of the source bitmap in pixels.
        /// \param          sourceHeight        Height of the source bitmap in pixels.
        /// \param          sourceStride        The stride of the source bitmap in bytes.
        /// \param          sourcePixeltype     The pixeltype of the source bitmap.
        /// \param          source              Pointer to the source bitmap.
        /// \param          allocateTempBuffer  This functor is called when it is necessary to allocate a temporary buffer. The argument specifies the
        ///                                     size in bytes for the buffer. This argument must not be null.
        ///                                     If this functor returns null, then this method exception is left with an exception(of type runtime_error).
        /// \param          freeTempBuffer      This functor is called when the temporary buffer is to be released. It is guaranteed that this free-functor is called for
        ///                                     every temp-buffer-allocation before this method returns. This argument must not be null.
        /// \param [in,out] destination         The pointer to the output buffer.
        /// \param [in,out] sizeDestination     On input, this gives the size of the destination buffer in bytes. On return of this method (and provided the return value is 'true'), this gives
        ///                                     the actual used size (which is always less or equal to the value on input).
        /// \param          parameters          Property bag containing parameters controlling the operation. This argument can be null, in which case default parameters are used.
        ///
        /// \returns    True if it succeeds, and in this case the argument 'sizeDestination' will contain the size actual used in the output buffer.
        ///             False is returned in the case that the output buffer size was insufficient.
        static bool CompressZStd0(
            std::uint32_t sourceWidth,
            std::uint32_t sourceHeight,
            std::uint32_t sourceStride,
            libCZI::PixelType sourcePixeltype,
            const void* source,
            const std::function<void* (size_t)>& allocateTempBuffer,
            const std::function<void(void*)>& freeTempBuffer,
            void* destination,
            size_t& sizeDestination,
            const ICompressParameters* parameters);

        /// Compress the specified bitmap in "zstd0"-format. This method will compress the specified source-bitmap according to the "ZEN-zstd0-scheme" to newly allocated memory, 
        /// and return a blob of memory containing the data suitable to be put into a subblock. Details of the operation are:
        /// - (under certain conditions) a temporary buffer is required, the size of this temporary buffer is width*size_of_pixel*height. This method allows to pass   
        ///    in functions for allocating/freeing this temp-buffer. For performance reasons, some type of buffer-pooling or reuse can be applied here.
        /// - A pointer to an output buffer must be supplied, and its size is to be given. The required size of the output buffer is in general not known (and  
        ///    not knowable) beforehand. It is only possible to query an upper limit for the output-buffer (CalculateMaxCompressedSizeZStd1). If the output buffer
        ///    size is insufficient, this method return 'false'. 
        /// - On input, the parameter 'sizeDestination' gives the size of the output buffer; on return of the function, the value is overwritten with the actual  
        ///     used size (which is always less than the size on input).
        /// - All error conditions (like e. g. invalid arguments) result in an exception being thrown.
        /// \param          sourceWidth         Width of the source bitmap in pixels.
        /// \param          sourceHeight        Height of the source bitmap in pixels.
        /// \param          sourceStride        The stride of the source bitmap in bytes.
        /// \param          sourcePixeltype     The pixeltype of the source bitmap.
        /// \param          source              Pointer to the source bitmap.
        /// \param          allocateTempBuffer  This functor is called when it is necessary to allocate a temporary buffer. The argument specifies the
        ///                                     size in bytes for the buffer. This argument must not be null.
        ///                                     If this functor returns null, then this method exception is left with an exception(of type runtime_error).
        /// \param          freeTempBuffer      This functor is called when the temporary buffer is to be released. It is guaranteed that this free-functor is called for
        ///                                     every temp-buffer-allocation before this method returns. This argument must not be null.
        /// \param          parameters          Property bag containing parameters controlling the operation. This argument can be null, in which case default parameters are used.
        ///
        /// \returns    A shared pointer to an object representing and owning a block of memory.
        static std::shared_ptr<IMemoryBlock> CompressZStd0Alloc(
            std::uint32_t sourceWidth, 
            std::uint32_t sourceHeight, 
            std::uint32_t sourceStride, 
            libCZI::PixelType sourcePixeltype, 
            const void* source, 
            const std::function<void* (size_t)>& allocateTempBuffer, 
            const std::function<void(void*)>& freeTempBuffer, 
            const ICompressParameters* parameters);

        /// Compress the specified bitmap in "zstd0"-format. This method will compress the specified source-bitmap according to the "ZEN-zstd0-scheme" to
        /// a caller supplied block of memory. If successful, the used size of the memory block is returned, and the data is suitable to be put into a subblock. 
        /// Details of the operation are:
        /// - (under certain conditions) a temporary buffer is required, and this memory is then allocated internally (and freed) from the standard heap.
        /// - A pointer to an output buffer must be supplied, and its size is to be given. The required size of the output buffer is in general not known (and  
        ///    not knowable) beforehand. It is only possible to query an upper limit for the output-buffer (CalculateMaxCompressedSizeZStd1). If the output buffer
        ///    size is insufficient, this method return 'false'. 
        /// - On input, the parameter 'sizeDestination' gives the size of the output buffer; on return of the function, the value is overwritten with the actual  
        ///     used size (which is always less than the size on input).
        /// - There are only two possible outcomes of this function - either the operation completed successfully and returns true, the data is put into the output buffer and  
        ///     the argument 'sizeDestination' gives the used size in the output buffer. Or, false is returned, meaning that the output buffer size is found to be
        ///     insufficient - however, note that the required size is not given, so 'sizeDestination' is unchanged in this case (and there is no indication about 
        ///     how big an output buffer is required).
        /// - All other error conditions (like e. g. invalid arguments) result in an exception being thrown.
        /// \param          sourceWidth         Width of the source bitmap in pixels.
        /// \param          sourceHeight        Height of the source bitmap in pixels.
        /// \param          sourceStride        The stride of the source bitmap in bytes.
        /// \param          sourcePixeltype     The pixeltype of the source bitmap.
        /// \param          source              Pointer to the source bitmap.
        /// \param [in,out] destination         The pointer to the output buffer.
        /// \param [in,out] sizeDestination     On input, this gives the size of the destination buffer in bytes. On return of this method (and provided the return value is 'true'), this gives
        ///                                     the actual used size (which is always less or equal to the value on input).
        /// \param          parameters          Property bag containing parameters controlling the operation. This argument can be null, in which case default parameters are used.
        ///
        /// \returns    True if it succeeds, and in this case the argument 'sizeDestination' will contain the size actual used in the output buffer.
        ///             False is returned in the case that the output buffer size was insufficient.
        static bool CompressZStd0(
            std::uint32_t sourceWidth, 
            std::uint32_t sourceHeight, 
            std::uint32_t sourceStride, 
            libCZI::PixelType sourcePixeltype, 
            const void* source, 
            void* destination, 
            size_t& sizeDestination, 
            const ICompressParameters* parameters);

        /// Compress the specified bitmap in "zstd0"-format. This method will compress the specified source-bitmap according to the "ZEN-zstd0-scheme" to
        /// a caller supplied block of memory. If successful, the used size of the memory block is returned, and the data is suitable to be put into a subblock. 
        /// Details of the operation are:
        /// - (under certain conditions) a temporary buffer is required, and this memory is then allocated internally (and freed) from the standard heap.
        /// - A pointer to an output buffer must be supplied, and its size is to be given. The required size of the output buffer is in general not known (and  
        ///    not knowable) beforehand. It is only possible to query an upper limit for the output-buffer (CalculateMaxCompressedSizeZStd1). If the output buffer
        ///    size is insufficient, this method return 'false'. 
        /// - On input, the parameter 'sizeDestination' gives the size of the output buffer; on return of the function, the value is overwritten with the actual  
        ///     used size (which is always less than the size on input).
        /// - There are only two possible outcomes of this function - either the operation completed successfully and returns true, the data is put into the output buffer and  
        ///     the argument 'sizeDestination' gives the used size in the output buffer. Or, false is returned, meaning that the output buffer size is found to be
        ///     insufficient - however, note that the required size is not given, so 'sizeDestination' is unchanged in this case (and there is no indication about 
        ///     how big an output buffer is required).
        /// - All other error conditions (like e. g. invalid arguments) result in an exception being thrown.
        /// \param          sourceWidth         Width of the source bitmap in pixels.
        /// \param          sourceHeight        Height of the source bitmap in pixels.
        /// \param          sourceStride        The stride of the source bitmap in bytes.
        /// \param          sourcePixeltype     The pixeltype of the source bitmap.
        /// \param          source              Pointer to the source bitmap.
        /// \param          parameters          Property bag containing parameters controlling the operation. This argument can be null, in which case default parameters are used.
        ///
        /// \returns    True if it succeeds, and in this case the argument 'sizeDestination' will contain the size actual used in the output buffer.
        ///             False is returned in the case that the output buffer size was insufficient.
        static std::shared_ptr<IMemoryBlock> CompressZStd0Alloc(
            std::uint32_t sourceWidth, 
            std::uint32_t sourceHeight, 
            std::uint32_t sourceStride, 
            libCZI::PixelType sourcePixeltype, 
            const void* source, 
            const ICompressParameters* parameters);

        /// Calculates the maximum size which might be required (for the output buffer) when calling into "CompressZStd0".
        /// The guarantee here is : if calling into "CompressZStd0" with a output buffer of the size as determined here, the
        /// call will NEVER fail (for insufficient output buffer size).
        /// Note that this upper limit may be larger than the actual needed size by a huge factor (10 times or more), and it is of
        /// the order of the input size.
        ///
        /// \param  sourceWidth     The width of the bitmap in pixels. 
        /// \param  sourceHeight    The height of the bitmap in pixels.
        /// \param  sourcePixeltype The pixeltype of the bitmap.
        ///
        /// \returns    The calculated maximum compressed size.
        static size_t CalculateMaxCompressedSizeZStd1(std::uint32_t sourceWidth, std::uint32_t sourceHeight, libCZI::PixelType sourcePixeltype);

        /// Compress the specified bitmap in "zstd1"-format. This method will compress the specified source-bitmap according to the "ZEN-zstd1-scheme" to
        /// a caller supplied block of memory. If successful, the used size of the memory block is returned, and the data is suitable to be put into a subblock.
        /// Details of the operation are:
        /// - (under certain conditions) a temporary buffer is required, the size of this temporary buffer is width*size_of_pixel*height. This method allows to pass   
        ///    in functions for allocating/freeing this temp-buffer. For performance reasons, some type of buffer-pooling or reuse can be applied here.
        /// - A pointer to an output buffer must be supplied, and its size is to be given. The required size of the output buffer is in general not known (and  
        ///    not knowable) beforehand. It is only possible to query an upper limit for the output-buffer (CalculateMaxCompressedSizeZStd1). If the output buffer
        ///    size is insufficient, this method return 'false'. 
        /// - On input, the parameter 'sizeDestination' gives the size of the output buffer; on return of the function, the value is overwritten with the actual  
        ///     used size (which is always less than the size on input).
        /// - There are only two possible outcomes of this function - either the operation completed successfully and returns true, the data is put into the output buffer and  
        ///     the argument 'sizeDestination' gives the used size in the output buffer. Or, false is returned, meaning that the output buffer size is found to be
        ///     insufficient - however, note that the required size is not given, so 'sizeDestination' is unchanged in this case (and there is no indication about 
        ///     how big an output buffer is required).
        /// - All other error conditions (like e. g. invalid arguments) result in an exception being thrown.
        /// \param          sourceWidth                          Width of the source bitmap in pixels.
        /// \param          sourceHeight                         Height of the source bitmap in pixels.
        /// \param          sourceStride                         The stride of the source bitmap in bytes.
        /// \param          sourcePixeltype                      The pixeltype of the source bitmap.
        /// \param          source                               Pointer to the source bitmap.
        /// \param          allocateTempBuffer                   This functor is called when it is necessary to allocate a temporary buffer. The argument specifies the
        ///                                                      size in bytes for the buffer. This argument must not be null.
        ///                                                      If this functor returns null, then this method exception is left with an exception (of type runtime_error).
        /// \param          freeTempBuffer                       This functor is called when the temporary buffer is to be released. It is guaranteed that this free-functor is called for
        ///                                                      every temp-buffer-allocation before this method returns. This argument must not be null.
        /// \param [in,out] destination                          The pointer to the output buffer.
        /// \param [in,out] sizeDestination                      On input, this gives the size of the destination buffer in bytes. On return of this method (and provided the return value is 'true'), this gives
        ///                                                      the actual used size (which is always less or equal to the value on input).
        /// \param          parameters                           Property bag containing parameters controlling the operation. This argument can be null, in which case default parameters are used.                       
        /// \returns True if it succeeds, and in this case the argument 'sizeDestination' will contain the size actual used in the output buffer. 
        ///          False is returned in the case that the output buffer size was insufficient.
        static bool CompressZStd1(
            std::uint32_t sourceWidth,
            std::uint32_t sourceHeight,
            std::uint32_t sourceStride,
            libCZI::PixelType sourcePixeltype,
            const void* source,
            const std::function<void* (size_t)>& allocateTempBuffer,
            const std::function<void(void*)>& freeTempBuffer,
            void* destination,
            size_t& sizeDestination,
            const ICompressParameters* parameters);

        /// Compress the specified bitmap in "zstd1"-format. This method will compress the specified source-bitmap according to the "ZEN-zstd1-scheme" to newly allocated memory,
        /// and return a blob of memory containing the data suitable to be put into a subblock. Details of the operation are:
        /// Details of the operation are:
        /// - (under certain conditions) a temporary buffer is required, the size of this temporary buffer is width*size_of_pixel*height. This method allows to pass   
        ///    in functions for allocating/freeing this temp-buffer. For performance reasons, some type of buffer-pooling or reuse can be applied here.
        /// - A pointer to an output buffer must be supplied, and its size is to be given. The required size of the output buffer is in general not known (and  
        ///    not knowable) beforehand. It is only possible to query an upper limit for the output-buffer (CalculateMaxCompressedSizeZStd1). If the output buffer
        ///    size is insufficient, this method return 'false'. 
        /// - On input, the parameter 'sizeDestination' gives the size of the output buffer; on return of the function, the value is overwritten with the actual  
        ///     used size (which is always less than the size on input).
        /// - There are only two possible outcomes of this function - either the operation completed successfully and returns true, the data is put into the output buffer and  
        ///     the argument 'sizeDestination' gives the used size in the output buffer. Or, false is returned, meaning that the output buffer size is found to be
        ///     insufficient - however, note that the required size is not given, so 'sizeDestination' is unchanged in this case (and there is no indication about 
        ///     how big an output buffer is required).
        /// - All error conditions (like e. g. invalid arguments) result in an exception being thrown.
        /// \param          sourceWidth                          Width of the source bitmap in pixels.
        /// \param          sourceHeight                         Height of the source bitmap in pixels.
        /// \param          sourceStride                         The stride of the source bitmap in bytes.
        /// \param          sourcePixeltype                      The pixeltype of the source bitmap.
        /// \param          source                               Pointer to the source bitmap.
        /// \param          allocateTempBuffer
        /// This functor is called when it is necessary to allocate a temporary buffer. The argument specifies the
        /// size in bytes for the buffer. This argument must not be null.
        /// If this functor returns null, then this method exception is left with an exception (of type runtime_error).
        /// \param          freeTempBuffer                       This functor is called when the temporary buffer is to be released. It is guaranteed that this free-functor is called for
        ///                                                      every temp-buffer-allocation before this method returns. This argument must not be null.
        /// \param          parameters                           Property bag containing parameters controlling the operation. This argument can be null, in which case default parameters are used.                       
        /// \returns    A shared pointer to an object representing and owning a block of memory.
        static std::shared_ptr<IMemoryBlock> CompressZStd1Alloc(
            std::uint32_t sourceWidth, 
            std::uint32_t sourceHeight, 
            std::uint32_t sourceStride, 
            libCZI::PixelType sourcePixeltype, 
            const void* source, 
            const std::function<void* (size_t)>& allocateTempBuffer, 
            const std::function<void(void*)>& freeTempBuffer, 
            const ICompressParameters* parameters);

        /// Compress the specified bitmap in "zstd1"-format. This method will compress the specified source-bitmap according to the "ZEN-zstd1-scheme" to
        /// a caller supplied block of memory. If successful, the used size of the memory block is returned, and the data is suitable to be put into a subblock.
        /// Details of the operation are:
        /// - (under certain conditions) a temporary buffer is required, and this memory is then allocated internally (and freed) from the standard heap.
        /// - A pointer to an output buffer must be supplied, and its size is to be given. The required size of the output buffer is in general not known (and  
        ///    not knowable) beforehand. It is only possible to query an upper limit for the output-buffer (CalculateMaxCompressedSizeZStd1). If the output buffer
        ///    size is insufficient, this method return 'false'. 
        /// - On input, the parameter 'sizeDestination' gives the size of the output buffer; on return of the function, the value is overwritten with the actual  
        ///     used size (which is always less than the size on input).
        /// - There are only two possible outcomes of this function - either the operation completed successfully and returns true, the data is put into the output buffer and  
        ///     the argument 'sizeDestination' gives the used size in the output buffer. Or, false is returned, meaning that the output buffer size is found to be
        ///     insufficient - however, note that the required size is not given, so 'sizeDestination' is unchanged in this case (and there is no indication about 
        ///     how big an output buffer is required).
        /// - All other error conditions (like e. g. invalid arguments) result in an exception being thrown.
        /// \param          sourceWidth                          Width of the source bitmap in pixels.
        /// \param          sourceHeight                         Height of the source bitmap in pixels.
        /// \param          sourceStride                         The stride of the source bitmap in bytes.
        /// \param          sourcePixeltype                      The pixeltype of the source bitmap.
        /// \param          source                               Pointer to the source bitmap.
        /// \param [in,out] destination                          The pointer to the output buffer.
        /// \param [in,out] sizeDestination                      On input, this gives the size of the destination buffer in bytes. On return of this method (and provided the return value is 'true'), this gives
        ///                                                      the actual used size (which is always less or equal to the value on input).
        /// \param          parameters                           Property bag containing parameters controlling the operation. This argument can be null, in which case default parameters are used.                       
        /// \returns        True if it succeeds, and in this case the argument 'sizeDestination' will contain the size actual used in the output buffer. 
        ///                 False is returned in the case that the output buffer size was insufficient.
        static bool CompressZStd1(
            std::uint32_t sourceWidth, 
            std::uint32_t sourceHeight, 
            std::uint32_t sourceStride, 
            libCZI::PixelType sourcePixeltype, 
            const void* source, 
            void* destination, 
            size_t& sizeDestination, 
            const ICompressParameters* parameters);

        /// Compress the specified bitmap in "zstd1"-format. This method will compress the specified source-bitmap according to the "ZEN-zstd1-scheme" to newly allocated memory,
        /// and return a blob of memory containing the data suitable to be put into a subblock. Details of the operation are:
        /// Details of the operation are:
        /// - (under certain conditions) a temporary buffer is required, and this memory is then allocated internally (and freed) from the standard heap.
        /// - A pointer to an output buffer must be supplied, and its size is to be given. The required size of the output buffer is in general not known (and  
        ///    not knowable) beforehand. It is only possible to query an upper limit for the output-buffer (CalculateMaxCompressedSizeZStd1). If the output buffer
        ///    size is insufficient, this method return 'false'. 
        /// - On input, the parameter 'sizeDestination' gives the size of the output buffer; on return of the function, the value is overwritten with the actual  
        ///     used size (which is always less than the size on input).
        /// - There are only two possible outcomes of this function - either the operation completed successfully and returns true, the data is put into the output buffer and  
        ///     the argument 'sizeDestination' gives the used size in the output buffer. Or, false is returned, meaning that the output buffer size is found to be
        ///     insufficient - however, note that the required size is not given, so 'sizeDestination' is unchanged in this case (and there is no indication about 
        ///     how big an output buffer is required).
        /// - All error conditions (like e. g. invalid arguments) result in an exception being thrown.
        /// \param          sourceWidth                          Width of the source bitmap in pixels.
        /// \param          sourceHeight                         Height of the source bitmap in pixels.
        /// \param          sourceStride                         The stride of the source bitmap in bytes.
        /// \param          sourcePixeltype                      The pixeltype of the source bitmap.
        /// \param          source                               Pointer to the source bitmap.
        /// \param          parameters                           Property bag containing parameters controlling the operation. This argument can be null, in which case default parameters are used.
        /// \returns    A shared pointer to an object representing and owning a block of memory.    
        static std::shared_ptr<IMemoryBlock> CompressZStd1Alloc(
            std::uint32_t sourceWidth, 
            std::uint32_t sourceHeight, 
            std::uint32_t sourceStride, 
            libCZI::PixelType sourcePixeltype, 
            const void* source, 
            const ICompressParameters* parameters);
    };

    /// Simplistic implementation of the compression-parameters property bag. Note that for high-performance scenarios
    /// it might be a good idea to re-use instances of this, or have a custom implementation without heap-allocation
    /// penalty.
    class LIBCZI_API CompressParametersOnMap : public ICompressParameters
    {
    public:
        std::map<int, CompressParameter> map;   ///< The key-value map containing "compression parameters".

        /// Attempts to get the property for the specified key from the property bag.
        ///
        /// \param          key         The key.
        /// \param [in,out] parameter   If non-null and the key is found, then the value is put here.
        ///
        /// \returns    True if the key is found in the property bag; false otherwise.
        bool TryGetProperty(int key, CompressParameter* parameter) const override
        {
            const auto it = this->map.find(key);
            if (it != this->map.cend())
            {
                if (parameter != nullptr)
                {
                    *parameter = it->second;
                }

                return true;
            }

            return false;
        }
    };
}