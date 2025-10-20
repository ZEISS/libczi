// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <sstream>
#include <zstd.h>
#include <cassert>  
#if (ZSTD_VERSION_MAJOR >= 1 && ZSTD_VERSION_MINOR >= 5) 
#include <zstd_errors.h>
#else
#include <common/zstd_errors.h>
#endif
#include "BitmapOperations.h"
#include "libCZI_compress.h"
#include "utilities.h"

using namespace libCZI;
using namespace libCZI::detail;
using namespace std;

class MemoryBlock : public IMemoryBlock
{
private:
    void* ptr;
    size_t sizeOfData;
public:
    MemoryBlock() = delete;
    MemoryBlock(const MemoryBlock&) = delete;
    MemoryBlock& operator=(const MemoryBlock&) = delete;

    explicit MemoryBlock(size_t initialSize)
        : ptr(nullptr), sizeOfData(0)
    {
        this->ptr = malloc(initialSize);
        this->sizeOfData = initialSize;
    }

    void* GetPtr() override { return this->ptr; }
    size_t GetSizeOfData() const override { return this->sizeOfData; }

    void ReduceSize(size_t reducedSize)
    {
        assert(reducedSize <= this->sizeOfData);
        this->sizeOfData = reducedSize;
    }

    ~MemoryBlock() override { free(this->ptr); }
};

static bool CompressZstd(const void* source, size_t sizeSource, void* destination, size_t& sizeDestination, int zstdCompressionLevel)
{
    if (source == nullptr || sizeSource == 0 || destination == nullptr || sizeDestination == 0)
    {
        throw invalid_argument("invalid arguments");
    }

    if (zstdCompressionLevel < ZSTD_minCLevel() || zstdCompressionLevel > ZSTD_maxCLevel())
    {
        stringstream ss;
        ss << "zstdCompressionLevel must be between " << ZSTD_minCLevel() << " and " << ZSTD_maxCLevel() << ", whereas " << zstdCompressionLevel << " was specified.";
        throw invalid_argument(ss.str());
    }

    const size_t r = ZSTD_compress(destination, sizeDestination, source, sizeSource, zstdCompressionLevel);

    if (ZSTD_isError(r))
    {
        // the only thing that can go wrong here is that "sizeDestination" is too small
        return false;
    }

    sizeDestination = r;
    return true;
}

static bool CompressZstd(const void* source, size_t sizeSource, void* destination, size_t& sizeDestination, const ICompressParameters* parameters)
{
    // TODO: check what the default is/should be - should be the same as in ZEN I'd reckon
    int zstdCompressionLevel = 0;
    if (parameters != nullptr)
    {
        CompressParameter propBagParameter;
        if (parameters->TryGetProperty(CompressionParameterKey::ZSTD_RAWCOMPRESSIONLEVEL, &propBagParameter) &&
            propBagParameter.GetType() == CompressParameter::Type::Int32)
        {
            zstdCompressionLevel = Utilities::clamp(propBagParameter.GetInt32(), ZSTD_minCLevel(), ZSTD_maxCLevel());
        }
    }

    return CompressZstd(source, sizeSource, destination, sizeDestination, zstdCompressionLevel);
}

static void CheckSourceBitmapArgumentsAndThrow(std::uint32_t sourceWidth, std::uint32_t sourceHeight, std::uint32_t sourceStride, libCZI::PixelType sourcePixeltype, const void* source)
{
    if (sourceWidth == 0)
    {
        throw invalid_argument("width must be greater than zero");
    }

    if (sourceHeight == 0)
    {
        throw invalid_argument("height must be greater than zero");
    }

    // note: GetBytesPerPixel will throw (an invalid_argument-exception) in case of an invalid enum value
    if (sourceStride < sourceWidth * Utils::GetBytesPerPixel(sourcePixeltype))
    {
        stringstream ss;
        ss << "stride is illegal, for width=" << sourceWidth << " and pixeltype=" << Utils::PixelTypeToInformalString(sourcePixeltype) << " the minimum stride is "
            << sourceWidth * Utils::GetBytesPerPixel(sourcePixeltype) << " whereas " << sourceStride << " was specified.";
        throw invalid_argument(ss.str());
    }

    if (source == nullptr)
    {
        throw invalid_argument("source must not be null");
    }
}

static void CheckDestinationArgumentsAndThrow(const void* destination, size_t sizeDestination, size_t minSizeOfDestination)
{
    if (destination == nullptr)
    {
        throw invalid_argument("destination must not be null.");
    }

    if (sizeDestination < minSizeOfDestination)
    {
        stringstream ss;
        ss << "sizeDestination must be greater than or equal to " << minSizeOfDestination << ", whereas " << sizeDestination << " was specified.";
        throw invalid_argument(ss.str());
    }
}

static void CheckTempBufferAllocArgumentsAndThrow(const std::function<void* (size_t)>& allocateTempBuffer, const std::function<void(void*)>& freeTempBuffer)
{
    if (!allocateTempBuffer)
    {
        throw invalid_argument("A function for allocating temp memory must be given.");
    }

    if (!freeTempBuffer)
    {
        throw invalid_argument("A function for freeing temp memory must be given.");
    }
}

size_t libCZI::ZstdCompress::CalculateMaxCompressedSizeZStd0(std::uint32_t sourceWidth, std::uint32_t sourceHeight, libCZI::PixelType sourcePixeltype)
{
    const size_t sizeSource = static_cast<size_t>(sourceWidth) * Utils::GetBytesPerPixel(sourcePixeltype) * sourceHeight;
    if (sizeSource == 0)
    {
        throw invalid_argument("'sizeSrcData' must be a positive number");
    }

    const size_t maxSize = ZSTD_compressBound(sizeSource);
    return maxSize;
}

size_t libCZI::ZstdCompress::CalculateMaxCompressedSizeZStd1(std::uint32_t sourceWidth, std::uint32_t sourceHeight, libCZI::PixelType sourcePixeltype)
{
    // for the time being, the header we put in front of the compressed data is 3 bytes fixed, so let's simply add those 3 bytes
    return 3 + CalculateMaxCompressedSizeZStd0(sourceWidth, sourceHeight, sourcePixeltype);
}

bool libCZI::ZstdCompress::CompressZStd0(
    std::uint32_t sourceWidth,
    std::uint32_t sourceHeight,
    std::uint32_t sourceStride,
    libCZI::PixelType sourcePixeltype,
    const void* source,
    const std::function<void* (size_t)>& allocateTempBuffer,
    const std::function<void(void*)>& freeTempBuffer,
    void* destination,
    size_t& sizeDestination,
    const ICompressParameters* parameters)
{
    CheckSourceBitmapArgumentsAndThrow(sourceWidth, sourceHeight, sourceStride, sourcePixeltype, source);
    CheckDestinationArgumentsAndThrow(destination, sizeDestination, 1);
    CheckTempBufferAllocArgumentsAndThrow(allocateTempBuffer, freeTempBuffer);

    // that's the size of the input bitmap with "minimal stride" (-> stride = width * bytes_per_pel)
    const size_t requiredSizeSource = static_cast<size_t>(sourceWidth) * Utils::GetBytesPerPixel(sourcePixeltype) * sourceHeight;
    if (requiredSizeSource == static_cast<size_t>(sourceStride) * sourceHeight)
    {
        // in this case, the input bitmap is already "with minimal stride", so we can use the source right away
        return CompressZstd(source, requiredSizeSource, destination, sizeDestination, parameters);
    }
    else
    {
        // now we need to "stride-convert" the input first, for which we require a temporary buffer
        void* tempBuffer = allocateTempBuffer(requiredSizeSource);
        if (tempBuffer == nullptr)
        {
            stringstream ss;
            ss << "Allocation of temporary buffer (of " << requiredSizeSource << " bytes) failed.";
            throw runtime_error(ss.str());
        }

        auto deleter = [&](void* ptr) -> void {freeTempBuffer(ptr); };
        const unique_ptr<void, decltype(deleter)> upTemp(tempBuffer, deleter);
        CBitmapOperations::Copy(sourcePixeltype, source, sourceStride, sourcePixeltype, upTemp.get(), sourceWidth * Utils::GetBytesPerPixel(sourcePixeltype), sourceWidth, sourceHeight, false);

        return CompressZstd(upTemp.get(), requiredSizeSource, destination, sizeDestination, parameters);
    }
}

std::shared_ptr<IMemoryBlock> libCZI::ZstdCompress::CompressZStd0Alloc(std::uint32_t sourceWidth, std::uint32_t sourceHeight, std::uint32_t sourceStride, libCZI::PixelType sourcePixeltype, const void* source, const std::function<void* (size_t)>& allocateTempBuffer, const std::function<void(void*)>& freeTempBuffer, const ICompressParameters* parameters)
{
    CheckSourceBitmapArgumentsAndThrow(sourceWidth, sourceHeight, sourceStride, sourcePixeltype, source);
    CheckTempBufferAllocArgumentsAndThrow(allocateTempBuffer, freeTempBuffer);

    // allocate a memory-block which is "large enough under all circumstances"
    const size_t size = ZstdCompress::CalculateMaxCompressedSizeZStd0(sourceWidth, sourceHeight, sourcePixeltype);
    auto memBlk = make_shared<MemoryBlock>(size);

    size_t sizeOfOutput = size;
    const bool b = ZstdCompress::CompressZStd0(sourceWidth, sourceHeight, sourceStride, sourcePixeltype, source, allocateTempBuffer, freeTempBuffer, memBlk->GetPtr(), sizeOfOutput, parameters);
    if (!b)
    {
        return std::shared_ptr<MemoryBlock>();
    }

    memBlk->ReduceSize(sizeOfOutput);
    return memBlk;
}

bool libCZI::ZstdCompress::CompressZStd0(std::uint32_t sourceWidth, std::uint32_t sourceHeight, std::uint32_t sourceStride, libCZI::PixelType sourcePixeltype, const void* source, void* destination, size_t& sizeDestination, const ICompressParameters* parameters)
{
    return ZstdCompress::CompressZStd0(sourceWidth, sourceHeight, sourceStride, sourcePixeltype, source, malloc, free, destination, sizeDestination, parameters);
}

std::shared_ptr<IMemoryBlock> libCZI::ZstdCompress::CompressZStd0Alloc(std::uint32_t sourceWidth, std::uint32_t sourceHeight, std::uint32_t sourceStride, libCZI::PixelType sourcePixeltype, const void* source, const ICompressParameters* parameters)
{
    return ZstdCompress::CompressZStd0Alloc(sourceWidth, sourceHeight, sourceStride, sourcePixeltype, source, malloc, free, parameters);
}

bool libCZI::ZstdCompress::CompressZStd1(std::uint32_t sourceWidth, std::uint32_t sourceHeight, std::uint32_t sourceStride, libCZI::PixelType sourcePixeltype, const void* source, const std::function<void* (size_t)>& allocateTempBuffer, const std::function<void(void*)>& freeTempBuffer, void* destination, size_t& sizeDestination, const ICompressParameters* parameters)
{
    CheckSourceBitmapArgumentsAndThrow(sourceWidth, sourceHeight, sourceStride, sourcePixeltype, source);
    CheckDestinationArgumentsAndThrow(destination, sizeDestination, 3 + 1);
    CheckTempBufferAllocArgumentsAndThrow(allocateTempBuffer, freeTempBuffer);

    bool doLoHiBytePacking = false;
    if (parameters != nullptr &&
        (sourcePixeltype == PixelType::Bgr48 || sourcePixeltype == PixelType::Gray16))
    {
        CompressParameter parameter;
        if (parameters->TryGetProperty(CompressionParameterKey::ZSTD_PREPROCESS_DOLOHIBYTEPACKING, &parameter) &&
            parameter.GetType() == CompressParameter::Type::Boolean)
        {
            doLoHiBytePacking = parameter.GetBoolean();
        }
    }

    const size_t bytesPerPel = Utils::GetBytesPerPixel(sourcePixeltype);
    size_t actualSizeDestination = sizeDestination - 3; // we need three bytes for the "zstd1"-header (in its current state), we subtract this here (and ensured above that sizeDestination is >3)
    bool b;
    if (doLoHiBytePacking)
    {
        const size_t requiredSizeTemp = sourceWidth * bytesPerPel * sourceHeight;
        void* tempBuffer = allocateTempBuffer(requiredSizeTemp);

        auto deleter = [&](void* ptr) -> void {freeTempBuffer(ptr); };
        const unique_ptr<void, decltype(deleter)> upTemp(tempBuffer, deleter);

        LoHiBytePackUnpack::LoHiByteUnpackStrided(source, static_cast<uint32_t>(sourceWidth * bytesPerPel / 2), sourceStride, sourceHeight, upTemp.get());

        // ok, so now we use CompressZStd to do the actual zstd-compression
        b = CompressZstd(upTemp.get(), requiredSizeTemp, 3 + static_cast<char*>(destination), actualSizeDestination, parameters);
    }
    else
    {
        if (sourceStride == sourceWidth * bytesPerPel)
        {
            b = CompressZstd(source, sourceWidth * bytesPerPel * sourceHeight, 3 + static_cast<char*>(destination), actualSizeDestination, parameters);
        }
        else
        {
            const size_t requiredSizeTemp = sourceWidth * bytesPerPel * sourceHeight;
            void* tempBuffer = allocateTempBuffer(requiredSizeTemp);
            if (tempBuffer == nullptr)
            {
                stringstream ss;
                ss << "Allocation of temporary buffer (of " << requiredSizeTemp << " bytes) failed.";
                throw runtime_error(ss.str());
            }

            auto deleter = [&](void* ptr) -> void {freeTempBuffer(ptr); };
            const unique_ptr<void, decltype(deleter)> upTemp(tempBuffer, deleter);

            CBitmapOperations::Copy(sourcePixeltype, source, sourceStride, sourcePixeltype, upTemp.get(), static_cast<uint32_t>(sourceWidth * bytesPerPel), sourceWidth, sourceHeight, false);

            b = CompressZstd(upTemp.get(), requiredSizeTemp, 3 + static_cast<char*>(destination), actualSizeDestination, parameters);
        }
    }

    if (!b)
    {
        return false;
    }

    // so, now put the correct "header" in front of the compressed data

    /*
            The syntax for the header is:

            [size]

            [chunk] ---<---+
            |           |   - 0 or more of them
            |           |
            +----->-----+

            The size-field gives the size of the header (starting counting at offset 0, i. e. including the size-field itself).
            For the size-field we use "MSB varint encoding" (cf. here https://techoverflow.net/2013/01/25/efficiently-encoding-variable-length-integers-in-cc/ or https://developpaper.com/explain-the-principle-of-varint-coding-in-detail/):
            - The most-significant bit (MSB) in a byte indicates whether the next byte is part of the size.
            - For numbers <128, the size field is just one byte long (and the most-significant bit is not set)
            - For number >=128, the field extends into the second byte (and byte 0 gives the least significant 7 bytes of the result),
            so we have ( ([0]&127) + ([1]&127)*128 as the value (provided that the MSB of [1] is zero)
            - if the MSB of the second byte is also one, then we extend to the third byte, the result is then given by
            ([0]&127) + ([1]&127)*128 + [2]*16384
            - We do not extend beyond the third byte (even if its MSB is one), so the MSB of the third byte is a "valid bit for the number".
            - The max number which can be encoded like this therefore is: 127 + 127*128 + 255*16384 = 4194303 = 0x400000 - 1

            A chunk is composed of a number (encoded like the size field) and payload. This number identifies the "type" of
            the chunk, and the size of the chunk must be derivable from the data. So, it can either be a fixed size chunk or
            the size must be encoded in the payload (in a unambiguous way).
            The sum of the sizes of the chunks must exactly match the size given in the size-field (plus the size of the size-field itself), there
            must be no "unused" data.

            Currently, we have only a chunk of type "1" with a fixed size of 1 byte. The first bit of this 1 byte payload
            indicates whether hi-lo-byte unpacking was applied (as a preprocessing step). Default (if this chunk is not present)
            is "no hi-lo-byte unpacking".

            So, the header looks like this (for unpacking was applied) : 0x03 0x01 0x01
                                    and if unpacking was _not_ applied : 0x03 0x01 0x00

            Note that we could make use of the definition of the default to shorten the header to: 0x01 - for the case "no unpacking".
            We leave this optimization to some interested collaborator.
    */

    // for the time being, we have this simple implementation here (since we only have to distinguish two cases)
    static_cast<uint8_t*>(destination)[0] = 0x03;
    static_cast<uint8_t*>(destination)[1] = 0x01;

    if (doLoHiBytePacking)
    {
        static_cast<uint8_t*>(destination)[2] = 0x01;
    }
    else
    {
        static_cast<uint8_t*>(destination)[2] = 0x00;
    }

    sizeDestination = actualSizeDestination + 3;
    return true;
}

std::shared_ptr<IMemoryBlock> libCZI::ZstdCompress::CompressZStd1Alloc(std::uint32_t sourceWidth, std::uint32_t sourceHeight, std::uint32_t sourceStride, libCZI::PixelType sourcePixeltype, const void* source, const std::function<void* (size_t)>& allocateTempBuffer, const std::function<void(void*)>& freeTempBuffer, const ICompressParameters* parameters)
{
    CheckSourceBitmapArgumentsAndThrow(sourceWidth, sourceHeight, sourceStride, sourcePixeltype, source);
    CheckTempBufferAllocArgumentsAndThrow(allocateTempBuffer, freeTempBuffer);

    // allocate a memory-block which is "large enough under all circumstances"
    const size_t size = ZstdCompress::CalculateMaxCompressedSizeZStd1(sourceWidth, sourceHeight, sourcePixeltype);
    auto memBlk = make_shared<MemoryBlock>(size);

    size_t sizeOfOutput = size;
    const bool b = ZstdCompress::CompressZStd1(sourceWidth, sourceHeight, sourceStride, sourcePixeltype, source, allocateTempBuffer, freeTempBuffer, memBlk->GetPtr(), sizeOfOutput, parameters);
    if (!b)
    {
        return std::shared_ptr<MemoryBlock>();
    }

    memBlk->ReduceSize(sizeOfOutput);
    return memBlk;
}

bool libCZI::ZstdCompress::CompressZStd1(std::uint32_t sourceWidth, std::uint32_t sourceHeight, std::uint32_t sourceStride, libCZI::PixelType sourcePixeltype, const void* source, void* destination, size_t& sizeDestination, const ICompressParameters* parameters)
{
    return ZstdCompress::CompressZStd1(sourceWidth, sourceHeight, sourceStride, sourcePixeltype, source, malloc, free, destination, sizeDestination, parameters);
}

std::shared_ptr<IMemoryBlock> libCZI::ZstdCompress::CompressZStd1Alloc(std::uint32_t sourceWidth, std::uint32_t sourceHeight, std::uint32_t sourceStride, libCZI::PixelType sourcePixeltype, const void* source, const ICompressParameters* parameters)
{
    return ZstdCompress::CompressZStd1Alloc(sourceWidth, sourceHeight, sourceStride, sourcePixeltype, source, malloc, free, parameters);
}
