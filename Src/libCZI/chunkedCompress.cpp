// SPDX-FileCopyrightText: 2026 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "libCZI_Config_Internal.h"
#include "libCZI_compress.h"

#if LIBCZI_EXPERIMENTAL_CHUNKED_COMPRESSION_AVAILABLE
#include <cstring>
#include <stdexcept>
#include <limits>
#include <numeric>

#include "utilities.h"
#include "compresscommon.h"

#include <zstd.h>

#include "BitmapOperations.h"
#if (ZSTD_VERSION_MAJOR >= 1 && ZSTD_VERSION_MINOR >= 5) 
#include <zstd_errors.h>
#else
#include <common/zstd_errors.h>
#endif

#if (LIBCZI_LZ4_AVAILABLE)
#include <lz4.h>
#endif

using namespace libCZI;
using namespace libCZI::detail;
using namespace std;

namespace
{
    /// Parse a 2 byte varint from the given data. The encoding is "MSB varint encoding" (cf. here https://techoverflow.net/2013/01/25/efficiently-encoding-variable-length-integers-in-cc/ 
    /// or https://developpaper.com/explain-the-principle-of-varint-coding-in-detail/):
    ///
    /// \exception	invalid_argument	Thrown when an invalid argument error condition occurs.
    ///
    /// \param 	data		The data.
    /// \param 	sizeData	Information describing the size.
    ///
    /// \returns	A tuple, where the first element is the parsed value, and the second element is the number of bytes consumed from the input data.
    tuple<uint16_t, size_t> Parse2ByteVarInt(const void* data, size_t sizeData)
    {
        if (sizeData < 1)
        {
            throw invalid_argument("sizeData must be at least 1");
        }

        const std::uint8_t* p = static_cast<const std::uint8_t*>(data);
        const std::uint16_t value = p[0] & 0x7F;
        const bool hasMore = (p[0] & 0x80) != 0;
        if (!hasMore)
        {
            return { value, 1 };
        }

        if (sizeData < 2)
        {
            throw invalid_argument("sizeData must be at least 2 when the first byte indicates that more bytes follow");
        }

        return { value | (static_cast<std::uint16_t>(p[1]) << 7), 2 };
    }

    /// Parse a 3 byte varint from the given data. The encoding is "MSB varint encoding" (cf. here https://techoverflow.net/2013/01/25/efficiently-encoding-variable-length-integers-in-cc/ 
    /// or https://developpaper.com/explain-the-principle-of-varint-coding-in-detail/):
    ///
    /// \exception	invalid_argument	Thrown when an invalid argument error condition occurs.
    ///
    /// \param 	data		The data.
    /// \param 	sizeData	Information describing the size.
    ///
    /// \returns	A tuple, where the first element is the parsed value, and the second element is the number of bytes consumed from the input data.
    tuple<uint32_t, size_t> Parse3ByteVarInt(const void* data, size_t sizeData)
    {
        if (sizeData < 1)
        {
            throw invalid_argument("sizeData must be at least 1");
        }

        const std::uint8_t* p = static_cast<const std::uint8_t*>(data);
        std::uint32_t value = p[0] & 0x7F;
        bool hasMore = (p[0] & 0x80) != 0;
        if (!hasMore)
        {
            return { value, 1 };
        }

        if (sizeData < 2)
        {
            throw invalid_argument("sizeData must be at least 2 when the first byte indicates that more bytes follow");
        }

        value |= static_cast<std::uint32_t>(p[1] & 0x7F) << 7;
        hasMore = (p[1] & 0x80) != 0;
        if (!hasMore)
        {
            return { value, 2 };
        }

        if (sizeData < 3)
        {
            throw invalid_argument("sizeData must be at least 3 when the second byte indicates that more bytes follow");
        }

        value |= static_cast<std::uint32_t>(p[2]) << 14;
        return { value, 3 };
    }

    /// Parse a 4 byte varint from the given data. The encoding is "MSB varint encoding" (cf. here https://techoverflow.net/2013/01/25/efficiently-encoding-variable-length-integers-in-cc/ 
    /// or https://developpaper.com/explain-the-principle-of-varint-coding-in-detail/):
    ///
    /// \exception	invalid_argument	Thrown when an invalid argument error condition occurs.
    ///
    /// \param 	data		The data.
    /// \param 	sizeData	Information describing the size.
    ///
    /// \returns	A tuple, where the first element is the parsed value, and the second element is the number of bytes consumed from the input data.
    tuple<uint32_t, size_t> Parse4ByteVarInt(const void* data, size_t sizeData)
    {
        if (sizeData < 1)
        {
            throw invalid_argument("sizeData must be at least 1");
        }

        const std::uint8_t* p = static_cast<const std::uint8_t*>(data);
        std::uint32_t value = p[0] & 0x7F;
        bool hasMore = (p[0] & 0x80) != 0;
        if (!hasMore)
        {
            return { value, 1 };
        }

        if (sizeData < 2)
        {
            throw invalid_argument("sizeData must be at least 2 when the first byte indicates that more bytes follow");
        }

        value |= static_cast<std::uint32_t>(p[1] & 0x7F) << 7;
        hasMore = (p[1] & 0x80) != 0;
        if (!hasMore)
        {
            return { value, 2 };
        }

        if (sizeData < 3)
        {
            throw invalid_argument("sizeData must be at least 3 when the second byte indicates that more bytes follow");
        }

        value |= static_cast<std::uint32_t>(p[2] & 0x7F) << 14;
        hasMore = (p[2] & 0x80) != 0;
        if (!hasMore)
        {
            return { value, 3 };
        }

        if (sizeData < 4)
        {
            throw invalid_argument("sizeData must be at least 4 when the third byte indicates that more bytes follow");
        }

        value |= static_cast<std::uint32_t>(p[3]) << 21;
        return { value, 4 };
    }


    size_t DetermineNumberOfBytesNeededFor4ByteVarInt(uint32_t value)
    {
        // 1 byte: values 0 to 127 (7 bits)
        if (value < (1 << 7))
        {
            return 1;
        }

        // 2 bytes: values 128 to 16383 (14 bits)
        if (value < (1 << 14))
        {
            return 2;
        }

        // 3 bytes: values 16384 to 4194303 (22 bits)
        if (value < (1 << 22))
        {
            return 3;
        }

        // 4 bytes: values 4194304 to 536870911 (29 bits)
        if (value < (1 << 29))
        {
            return 4;
        }

        // Value too large (requires more than 29 bits)
        throw invalid_argument("Value too large for 4-byte varint encoding (max 29 bits)");
    }

    size_t DetermineNumberOfBytesNeededFor3ByteVarInt(uint32_t value)
    {
        // 1 byte: values 0 to 127 (7 bits)
        if (value < (1 << 7))
        {
            return 1;
        }

        // 2 bytes: values 128 to 16383 (14 bits)
        if (value < (1 << 14))
        {
            return 2;
        }

        // 3 bytes: values 16384 to 4194303 (22 bits)
        if (value < (1 << 22))
        {
            return 3;
        }

        // Value too large (requires more than 22 bits)
        throw invalid_argument("Value too large for 3-byte varint encoding (max 22 bits)");
    }

    /// Write a 4 byte varint to the given destination. The encoding is "MSB varint encoding" (cf. here https://techoverflow.net/2013/01/25/efficiently-encoding-variable-length-integers-in-cc/ 
    /// or https://developpaper.com/explain-the-principle-of-varint-coding-in-detail/):
    ///
    /// \exception	invalid_argument	Thrown when an invalid argument error condition occurs.
    ///
    /// \param [in,out] destination		The destination buffer.
    /// \param 		   sizeDestination	Size of the destination buffer.
    /// \param 		   value		   	The value to encode.
    ///
    /// \returns	The number of bytes written to the destination buffer.
    size_t Write4ByteVarInt(void* destination, size_t sizeDestination, uint32_t value)
    {
        if (sizeDestination < 1)
        {
            throw invalid_argument("sizeDestination must be at least 1");
        }

        std::uint8_t* p = static_cast<std::uint8_t*>(destination);

        // 1 byte: values 0 to 127 (7 bits)
        if (value < (1 << 7))
        {
            p[0] = static_cast<std::uint8_t>(value);
            return 1;
        }

        // 2 bytes: values 128 to 16383 (14 bits)
        if (value < (1 << 14))
        {
            if (sizeDestination < 2)
            {
                throw invalid_argument("sizeDestination must be at least 2");
            }
            p[0] = static_cast<std::uint8_t>((value & 0x7F) | 0x80);
            p[1] = static_cast<std::uint8_t>(value >> 7);
            return 2;
        }

        // 3 bytes: values 16384 to 4194303 (22 bits)
        if (value < (1 << 22))
        {
            if (sizeDestination < 3)
            {
                throw invalid_argument("sizeDestination must be at least 3");
            }
            p[0] = static_cast<std::uint8_t>((value & 0x7F) | 0x80);
            p[1] = static_cast<std::uint8_t>(((value >> 7) & 0x7F) | 0x80);
            p[2] = static_cast<std::uint8_t>(value >> 14);
            return 3;
        }

        // 4 bytes: values 4194304 to 536870911 (29 bits)
        if (value < (1 << 29))
        {
            if (sizeDestination < 4)
            {
                throw invalid_argument("sizeDestination must be at least 4");
            }
            p[0] = static_cast<std::uint8_t>((value & 0x7F) | 0x80);
            p[1] = static_cast<std::uint8_t>(((value >> 7) & 0x7F) | 0x80);
            p[2] = static_cast<std::uint8_t>(((value >> 14) & 0x7F) | 0x80);
            p[3] = static_cast<std::uint8_t>(value >> 21);
            return 4;
        }

        // Value too large (requires more than 29 bits)
        throw invalid_argument("Value too large for 4-byte varint encoding (max 29 bits)");
    }

    /// Write a 3 byte varint to the given destination. The encoding is "MSB varint encoding" (cf. here https://techoverflow.net/2013/01/25/efficiently-encoding-variable-length-integers-in-cc/ 
    /// or https://developpaper.com/explain-the-principle-of-varint-coding-in-detail/):
    ///
    /// \exception	invalid_argument	Thrown when an invalid argument error condition occurs.
    ///
    /// \param [in,out] destination		The destination buffer.
    /// \param 		   sizeDestination	Size of the destination buffer.
    /// \param 		   value		   	The value to encode.
    ///
    /// \returns	The number of bytes written to the destination buffer.
    size_t Write3ByteVarInt(void* destination, size_t sizeDestination, uint32_t value)
    {
        if (sizeDestination < 1)
        {
            throw invalid_argument("sizeDestination must be at least 1");
        }

        std::uint8_t* p = static_cast<std::uint8_t*>(destination);

        // 1 byte: values 0 to 127 (7 bits)
        if (value < (1 << 7))
        {
            p[0] = static_cast<std::uint8_t>(value);
            return 1;
        }

        // 2 bytes: values 128 to 16383 (14 bits)
        if (value < (1 << 14))
        {
            if (sizeDestination < 2)
            {
                throw invalid_argument("sizeDestination must be at least 2");
            }
            p[0] = static_cast<std::uint8_t>((value & 0x7F) | 0x80);
            p[1] = static_cast<std::uint8_t>(value >> 7);
            return 2;
        }

        // 3 bytes: values 16384 to 4194303 (22 bits)
        if (value < (1 << 22))
        {
            if (sizeDestination < 3)
            {
                throw invalid_argument("sizeDestination must be at least 3");
            }
            p[0] = static_cast<std::uint8_t>((value & 0x7F) | 0x80);
            p[1] = static_cast<std::uint8_t>(((value >> 7) & 0x7F) | 0x80);
            p[2] = static_cast<std::uint8_t>(value >> 14);
            return 3;
        }

        // Value too large (requires more than 22 bits)
        throw invalid_argument("Value too large for 3-byte varint encoding (max 22 bits)");
    }
}

bool libCZI::ChunkedCompressionHeaderHelper::WalkCompressionHeader(const void* data, size_t sizeData, const std::function<bool(const CompressionHeaderChunk&)>& callback, size_t* bytes_consumed)
{
    if (data == nullptr)
    {
        throw invalid_argument("data must not be null");
    }

    if (!callback)
    {
        throw invalid_argument("callback must not be null");
    }

    const std::uint8_t* p = static_cast<const std::uint8_t*>(data);
    size_t offset = 0;

    for (;;)
    {
        const auto id = Parse2ByteVarInt(p + offset, sizeData - offset);
        offset += get<1>(id);

        if (get<0>(id) == static_cast<uint16_t>(HeaderChunkId::EndOfHeader))
        {
            if (bytes_consumed != nullptr)
            {
                *bytes_consumed = offset;
            }

            return true;
        }

        const auto chunkSize = Parse3ByteVarInt(p + offset, sizeData - offset);
        offset += get<1>(chunkSize);

        const size_t payloadSize = get<0>(chunkSize);
        if (payloadSize > sizeData - offset)
        {
            throw invalid_argument("Invalid chunk size in compression header.");
        }

        CompressionHeaderChunk compression_header_chunk;
        compression_header_chunk.chunkId = get<0>(id);
        compression_header_chunk.chunkSize = static_cast<std::uint32_t>(payloadSize);
        compression_header_chunk.chunkPayload = p + offset;
        compression_header_chunk.chunkPayloadSize = payloadSize;

        offset += payloadSize;

        const bool continueWalking = callback(compression_header_chunk);
        if (!continueWalking)
        {
            if (bytes_consumed != nullptr)
            {
                *bytes_consumed = offset;
            }

            return false;
        }
    }
}

/// Determines the total byte length of the compression header that begins at \p data.
///
/// The compression header is a self-describing binary structure consisting of a
/// sequence of typed chunks, each encoded as:
/// \code
///   [ id     : 1–2 byte MSB varint ]
///   [ length : 1–3 byte MSB varint ]
///   [ payload: <length> bytes      ]
/// \endcode
/// The sequence is terminated by a single EndOfHeader sentinel chunk whose id equals
/// zero, which encodes to exactly one byte and carries no length or payload field.
///
/// This function walks the chunk sequence without interpreting any payload content.
/// It counts bytes consumed until (and including) the EndOfHeader sentinel byte,
/// and returns that total count.
///
/// Typical use: given a buffer that contains a compression header immediately followed
/// by raw compressed chunk data, this function lets the caller locate the start of
/// the compressed data as:
/// \code
///   static_cast<const uint8_t*>(data) + GetCompressionHeaderSize(data, sizeData)
/// \endcode
///
/// \exception std::invalid_argument  Thrown when \p sizeData is 0 (no bytes to read),
///                                   or when a chunk's declared payload length would
///                                   extend beyond the end of the supplied buffer
///                                   (indicating a truncated or malformed header).
///
/// \param  data      Pointer to the first byte of the compression header.
/// \param  sizeData  Number of readable bytes available at \p data.
///
/// \returns  The number of bytes occupied by the complete header, including the
///           terminating EndOfHeader byte.
size_t libCZI::ChunkedCompressionHeaderHelper::GetCompressionHeaderSize(const void* data, size_t sizeData)
{
    // A well-formed header must contain at least one byte: the single-byte EndOfHeader
    // sentinel (id = 0 encodes to one byte in MSB varint).  Reject an empty buffer
    // immediately so every subsequent Parse* call can safely read at least one byte.
    if (sizeData < 1)
    {
        throw invalid_argument("sizeData must be at least 1");
    }

    // Reinterpret the caller's untyped pointer as a byte pointer so that pointer
    // arithmetic can be used to step through the header one field at a time.
    const uint8_t* p = static_cast<const uint8_t*>(data);

    // 'offset' is the running byte cursor.  It starts at zero (the first byte of the
    // header) and is advanced after each decoded field.  When the EndOfHeader sentinel
    // is found, its value equals the total number of bytes the header occupies.
    size_t offset = 0;

    // Walk the header chunk-by-chunk.  Each iteration decodes one (id, length, payload)
    // triple.  The loop has no explicit iteration count; it exits either through the
    // EndOfHeader return below, or via an exception thrown by a Parse* helper when the
    // remaining buffer is too small to decode the next field.
    for (;;)
    {
        // Decode the chunk identifier varint starting at the current cursor position.
        // Identifiers use MSB varint encoding with a 2-byte maximum:
        //   - If bit 7 of the first byte is 0, the id fits in 1 byte (values 0–127).
        //   - If bit 7 is 1, the id spans 2 bytes (values 128–16383).
        // The returned tuple carries (decoded_id_value, bytes_consumed_by_id_varint).
        const tuple<uint16_t, size_t> id = Parse2ByteVarInt(p + offset, sizeData - offset);

        // Advance the cursor past the id varint (1 or 2 bytes depending on the value).
        offset += get<1>(id);

        // Check whether this is the EndOfHeader sentinel (id == 0).
        // EndOfHeader has no length or payload field; it is just the single id byte.
        // At this point 'offset' has already moved past that byte, so it equals the
        // total byte length of the complete header — the value the caller is after.
        if (get<0>(id) == static_cast<uint16_t>(HeaderChunkId::EndOfHeader))
        {
            return offset;
        }

        // For any non-terminating chunk, the id is immediately followed by a
        // payload-length field encoded as a 3-byte-maximum MSB varint.
        // The returned tuple carries (payload_length_in_bytes, bytes_consumed_by_length_varint).
        const tuple<uint32_t, size_t> chunkSize = Parse3ByteVarInt(p + offset, sizeData - offset);

        // Advance the cursor past the payload-length varint (1, 2, or 3 bytes).
        offset += get<1>(chunkSize);

        // Validate that the declared payload length does not reach beyond the end of
        // the supplied buffer.  A well-formed header must fit entirely within 'sizeData'
        // bytes; if it does not, the buffer is either truncated or the data is corrupt.
        // check if the chunk size is valid (i.e. does not exceed the remaining data size)
        if (get<0>(chunkSize) > sizeData - offset)
        {
            throw invalid_argument("Invalid chunk size in compression header.");
        }

        // Skip over the payload bytes without reading them.  This function only
        // measures header size; interpreting payload content is the responsibility
        // of ParseCompressionHeader / WalkCompressionHeader.
        offset += get<0>(chunkSize);
    }
}

size_t libCZI::ChunkedCompressionHeaderHelper::CreateCompressionHeader(void* destination, size_t sizeDestination, const HeaderInfoForCreation& headerInfo)
{
    if (destination == nullptr)
    {
        throw invalid_argument("destination must not be null");
    }

    uint8_t* p = static_cast<uint8_t*>(destination);
    size_t offset = 0;

    // --- id=1: ChunkSizes ---
    if (!headerInfo.chunkSizes.empty())
    {
        // determine the size needed for the chunk sizes (i.e. the size of the chunk payload)
        size_t size_needed_for_offsets = 0;
        for (const uint32_t chunkSize : headerInfo.chunkSizes)
        {
            size_needed_for_offsets += DetermineNumberOfBytesNeededFor4ByteVarInt(chunkSize);
        }

        // check whether the cast in the next statement is safe (and note that DetermineNumberOfBytesNeededFor3ByteVarInt will throw if the value is too large for 3 bytes, 
        // so we do not have to check this here)
        if (size_needed_for_offsets > numeric_limits<uint32_t>::max())
        {
            throw invalid_argument("Size needed for chunk sizes exceeds maximum representable size.");
        }

        // total bytes for this header chunk: id(1) + length-varint + payload
        const size_t length_varint_size = DetermineNumberOfBytesNeededFor3ByteVarInt(static_cast<uint32_t>(size_needed_for_offsets));
        const size_t total_chunk_bytes = 1 + length_varint_size + size_needed_for_offsets;

        if (offset + total_chunk_bytes > sizeDestination)
        {
            throw invalid_argument("Destination buffer is too small for the chunk sizes header chunk.");
        }

        // write id
        *(p + offset) = static_cast<uint8_t>(HeaderChunkId::ChunkSizes);
        ++offset;

        // write payload length
        offset += Write3ByteVarInt(p + offset, sizeDestination - offset, static_cast<uint32_t>(size_needed_for_offsets));

        // write the actual chunk sizes (payload)
        for (const uint32_t chunkSize : headerInfo.chunkSizes)
        {
            offset += Write4ByteVarInt(p + offset, sizeDestination - offset, chunkSize);
        }
    }

    // --- id=3: DecompressedSizes ---
    {
        // The payload is a compact representation of the uncompressed size for each chunk.
        // It is an array of varint-encoded values with the following semantic:
        //   - If there is only one element, it gives the uncompressed size for ALL chunks.
        //   - If there are two elements, the first gives the uncompressed size for all chunks
        //     except the last, and the second gives the uncompressed size of the last chunk.
        //   - In general, values are listed in chunk order, and the second-to-last element
        //     repeats for all earlier chunks not explicitly listed.
        //     E.g. with 5 chunks and [20, 60]: chunk 0–3 = 20, chunk 4 = 60.
        //
        // HeaderInfo::uncompressedSizes encodes this as:
        //   get<0> = uncompressed size for all chunks except the last
        //   get<1> = uncompressed size of the last chunk, or 0 if same as get<0>
        const uint32_t commonSize = get<0>(headerInfo.uncompressedSizes);
        const uint32_t lastSize = get<1>(headerInfo.uncompressedSizes);
        const bool lastChunkDiffers = (lastSize != 0);

        size_t payload_size;
        if (!lastChunkDiffers)
        {
            // all chunks have the same uncompressed size — one value suffices
            payload_size = DetermineNumberOfBytesNeededFor4ByteVarInt(commonSize);
        }
        else
        {
            // two values: common (repeating) size first, then last chunk size
            payload_size = DetermineNumberOfBytesNeededFor4ByteVarInt(commonSize)
                + DetermineNumberOfBytesNeededFor4ByteVarInt(lastSize);
        }

        if (payload_size > numeric_limits<uint32_t>::max())
        {
            throw invalid_argument("Payload size for decompressed sizes exceeds maximum representable size.");
        }

        const size_t length_varint_size = DetermineNumberOfBytesNeededFor3ByteVarInt(static_cast<uint32_t>(payload_size));
        const size_t total_chunk_bytes = 1 + length_varint_size + payload_size;

        if (offset + total_chunk_bytes > sizeDestination)
        {
            throw invalid_argument("Destination buffer is too small for the decompressed sizes header chunk.");
        }

        *(p + offset) = static_cast<uint8_t>(HeaderChunkId::DecompressedSizes);
        ++offset;

        offset += Write3ByteVarInt(p + offset, sizeDestination - offset, static_cast<uint32_t>(payload_size));

        // write common (repeating) size first
        offset += Write4ByteVarInt(p + offset, sizeDestination - offset, commonSize);
        if (lastChunkDiffers)
        {
            // write last chunk's differing size second
            offset += Write4ByteVarInt(p + offset, sizeDestination - offset, lastSize);
        }
    }

    // --- id=2: CompressionMethod ---
    {
        // since zstd is the default (if this header-chunk is not present), we only write this chunk if the codec is something other than zstd
        if (headerInfo.codec != Codec::ZStd)
        {
            if (offset + 3 > sizeDestination)  // id(1) + length(1) + payload(1)
            {
                throw invalid_argument("Destination buffer is too small for the compression method header chunk.");
            }

            *(p + offset) = static_cast<uint8_t>(HeaderChunkId::CompressionMethod);
            ++offset;

            // payload length = 1 byte
            offset += Write3ByteVarInt(p + offset, sizeDestination - offset, 1);

            // payload: codec enum value
            *(p + offset) = static_cast<uint8_t>(headerInfo.codec);
            ++offset;
        }
    }

    // --- id=4: Preprocessing ---
    {
        // Write this chunk when the hi-lo byte packing preprocessing state is explicitly specified
        // (0 = not applied, 1 = applied). Other values mean unspecified and omit the chunk.
        if (headerInfo.hiLoBytePackingApplied == 1 || headerInfo.hiLoBytePackingApplied == 0)
        {
            if (offset + 3 > sizeDestination)  // id(1) + length(1) + payload(1)
            {
                throw invalid_argument("Destination buffer is too small for the preprocessing header chunk.");
            }

            *(p + offset) = static_cast<uint8_t>(HeaderChunkId::Preprocessing);
            ++offset;

            // payload length = 1 byte
            offset += Write3ByteVarInt(p + offset, sizeDestination - offset, 1);

            // payload: hiLoBytePackingApplied value (0 or 1)
            *(p + offset) = static_cast<uint8_t>(headerInfo.hiLoBytePackingApplied);
            ++offset;
        }
    }

    // --- EndOfHeader terminator ---
    if (offset + 1 > sizeDestination)
    {
        throw invalid_argument("Destination buffer is too small for the end-of-header marker.");
    }

    *(p + offset) = static_cast<uint8_t>(HeaderChunkId::EndOfHeader);
    ++offset;

    return offset;
}

size_t libCZI::ChunkedCompressionHeaderHelper::DetermineMaxSizeForCompressionHeader(const HeaderInfoForMaxSizeDetermination& headerInfo)
{
    if (headerInfo.number_of_chunks < 1)
    {
        throw invalid_argument("number_of_chunks must be greater than 0.");
    }

    // For the maximum size, we assume each varint uses its maximum encoding:
    //   - id varint:         1 byte (all current HeaderChunkId values are < 128)
    //   - length varint:     3 bytes max
    //   - chunk size varint: 4 bytes max

    size_t maxSize = 0;

    // --- id=1: ChunkSizes ---
    // id(1) + length(3) + payload(number_of_chunks * 4)
    maxSize += 1 + 3 + static_cast<size_t>(headerInfo.number_of_chunks) * 4;

    // --- id=3: DecompressedSizes ---
    // id(1) + length(3) + payload(at most 2 values, each 4 bytes max)
    maxSize += 1 + 3 + 2 * 4;

    // --- id=2: CompressionMethod ---
    // id(1) + length(3) + payload(1)
    // Only written when codec != ZStd (zstd is the default)
    if (headerInfo.codec != Codec::ZStd)
    {
        maxSize += 1 + 3 + 1;
    }

    // --- id=4: Preprocessing ---
    // id(1) + length(3) + payload(1)
    // Written when hiLoBytePackingApplied is explicitly specified as 0 or 1.
    if (headerInfo.hiLoBytePackingApplied == 0 || headerInfo.hiLoBytePackingApplied == 1)
    {
        maxSize += 1 + 3 + 1;
    }

    // --- EndOfHeader terminator ---
    // The EndOfHeader id is always 0, which is always a single byte in varint encoding.
    maxSize += 1;

    return maxSize;
}

size_t libCZI::ChunkedCompressionHeaderHelper::DetermineMaxSizeForCompressionHeader(const HeaderInfoForCreation& headerInfo)
{
    HeaderInfoForMaxSizeDetermination infoForMaxSizeDetermination;
    infoForMaxSizeDetermination.codec = headerInfo.codec;
    infoForMaxSizeDetermination.hiLoBytePackingApplied = headerInfo.hiLoBytePackingApplied;
    infoForMaxSizeDetermination.number_of_chunks = static_cast<std::uint32_t>(headerInfo.chunkSizes.size());
    return DetermineMaxSizeForCompressionHeader(infoForMaxSizeDetermination);
}

namespace
{
    std::vector<std::uint32_t> GetChunkSizesFromHeader(const ChunkedCompressionHeaderHelper::CompressionHeaderChunk& chunk)
    {
        std::vector<std::uint32_t> chunk_sizes;
        size_t i;
        for (i = 0; i < chunk.chunkPayloadSize;)
        {
            const auto value_and_size = Parse4ByteVarInt(static_cast<const uint8_t*>(chunk.chunkPayload) + i, chunk.chunkPayloadSize - i);
            i += get<1>(value_and_size);
            chunk_sizes.emplace_back(get<0>(value_and_size));
        }

        // check that the total size of the chunk sizes matches the chunk payload size exactly (i.e. that there is no "unused" data in the chunk payload)
        if (i > chunk.chunkPayloadSize)
        {
            throw invalid_argument("Invalid chunk sizes header chunk: payload size does not match the sum of the sizes of the encoded chunk sizes.");
        }

        return  chunk_sizes;
    }

    std::vector<std::uint32_t> GetUncompressedChunkSizesFromHeader(const ChunkedCompressionHeaderHelper::CompressionHeaderChunk& chunk)
    {
        std::vector<std::uint32_t> uncompressed_chunk_sizes;
        size_t i;
        for (i = 0; i < chunk.chunkPayloadSize;)
        {
            const auto value_and_size = Parse4ByteVarInt(static_cast<const uint8_t*>(chunk.chunkPayload) + i, chunk.chunkPayloadSize - i);
            i += get<1>(value_and_size);
            uncompressed_chunk_sizes.emplace_back(get<0>(value_and_size));
        }

        // check that the total size of the chunk sizes matches the chunk payload size exactly (i.e. that there is no "unused" data in the chunk payload)
        if (i > chunk.chunkPayloadSize)
        {
            throw invalid_argument("Invalid decompressed sizes header chunk: payload size does not match the sum of the sizes of the encoded uncompressed sizes.");
        }

        return  uncompressed_chunk_sizes;
    }

    std::vector< ChunkedCompressionHeaderHelper::HeaderInfo::ChunkInfo> GetChunkInfosFromCompressedAndUncompressedChunkSizes(const std::vector<std::uint32_t>& chunk_sizes, const std::vector<std::uint32_t>& uncompressed_chunk_sizes)
    {
        const size_t number_of_chunks = chunk_sizes.size();
        if (number_of_chunks == 0)
        {
            throw invalid_argument("At least one chunk size must be specified in the chunk sizes header chunk.");
        }

        const size_t number_of_uncompressed_sizes = uncompressed_chunk_sizes.size();
        if (number_of_uncompressed_sizes == 0)
        {
            throw invalid_argument("At least one uncompressed size must be specified in the decompressed sizes header chunk.");
        }

        if (number_of_uncompressed_sizes > number_of_chunks)
        {
            throw invalid_argument("Invalid decompressed sizes header chunk: more uncompressed sizes specified than the number of chunks.");
        }

        // the semantic is: 
        // * if there is only one uncompressed size, then this applies to all chunks
        // * if there are two uncompressed sizes, then the first applies to all chunks except the last, and the second applies to the last chunk
        // * if there are more than two uncompressed sizes, then the sizes are listed in chunk order, and the second-to-last element repeats for all earlier chunks not explicitly listed

        vector<ChunkedCompressionHeaderHelper::HeaderInfo::ChunkInfo> result;
        result.reserve(number_of_chunks);

        for (size_t i = 0; i < number_of_chunks; ++i)
        {
            uint32_t uncompressed_size_for_chunk;

            if (number_of_uncompressed_sizes == 1)
            {
                uncompressed_size_for_chunk = uncompressed_chunk_sizes[0];
            }
            else
            {
                // Values are interpreted as a suffix in chunk order.
                // The last value applies to the last chunk. If there are chunks before
                // the explicit suffix, they repeat the second-to-last value.
                const size_t prefix_count = number_of_chunks - number_of_uncompressed_sizes;
                if (i < prefix_count)
                {
                    uncompressed_size_for_chunk = uncompressed_chunk_sizes[number_of_uncompressed_sizes - 2];
                }
                else
                {
                    const size_t explicit_index = i - prefix_count;
                    uncompressed_size_for_chunk = uncompressed_chunk_sizes[explicit_index];
                }
            }

            ChunkedCompressionHeaderHelper::HeaderInfo::ChunkInfo chunk_info;
            chunk_info.compressedSize = chunk_sizes[i];
            chunk_info.uncompressedSize = uncompressed_size_for_chunk;
            result.emplace_back(chunk_info);
        }

        return result;
    }
}

std::tuple<size_t, ChunkedCompressionHeaderHelper::HeaderInfo> ChunkedCompressionHeaderHelper::ParseCompressionHeader(const void* data, size_t sizeData)
{
    std::vector<std::uint32_t> chunk_sizes;
    std::vector<std::uint32_t> uncompressed_sizes;
    Codec codec = Codec::Invalid;
    uint8_t preprocessing_value = 0xff;

    size_t bytes_consumed = 0;
    ChunkedCompressionHeaderHelper::WalkCompressionHeader(
        data,
        sizeData,
        [&](const CompressionHeaderChunk& chunk) -> bool
        {
            switch (static_cast<ChunkedCompressionHeaderHelper::HeaderChunkId>(chunk.chunkId))
            {
            case HeaderChunkId::ChunkSizes:
                chunk_sizes = GetChunkSizesFromHeader(chunk);
                break;
            case HeaderChunkId::DecompressedSizes:
                uncompressed_sizes = GetUncompressedChunkSizesFromHeader(chunk);
                break;
            case HeaderChunkId::CompressionMethod:
                if (chunk.chunkPayloadSize != 1)
                {
                    throw invalid_argument("Invalid compression method header chunk: payload size must be exactly 1 byte.");
                }

                codec = static_cast<Codec>(*static_cast<const uint8_t*>(chunk.chunkPayload));
                break;
            case HeaderChunkId::Preprocessing:
                if (chunk.chunkPayloadSize != 1)
                {
                    throw invalid_argument("Invalid preprocessing header chunk: payload size must be exactly 1 byte.");
                }

                preprocessing_value = *static_cast<const uint8_t*>(chunk.chunkPayload);
                if (preprocessing_value != 0 && preprocessing_value != 1)
                {
                    throw invalid_argument("Invalid preprocessing header chunk: payload value must be either 0 or 1.");
                }

                break;
            default:
                throw invalid_argument("Invalid header chunk ID in compression header.");
            }

            return true;  // continue walking through the header
        },
        &bytes_consumed);

    ChunkedCompressionHeaderHelper::HeaderInfo header_info;
    header_info.chunks = GetChunkInfosFromCompressedAndUncompressedChunkSizes(chunk_sizes, uncompressed_sizes);

    header_info.hiLoBytePackingApplied = false; // the default is "0" (i.e. not applied), and we will set this to "1" only if we encounter a preprocessing header chunk with the value "1"
    if (preprocessing_value == 1)
    {
        header_info.hiLoBytePackingApplied = true;
    }

    switch (codec)
    {
    case Codec::Invalid:
        // if the compression method chunk is not present, we assume zstd (which is the default)
        header_info.codec = Codec::ZStd;
        break;
    case Codec::ZStd:
    case Codec::Lz4:
        header_info.codec = codec;
        break;
    default:
        throw invalid_argument("Invalid codec specified in compression method header chunk.");
    }

    return make_tuple(bytes_consumed, header_info);
}

//-----------------------------------------------------------------------------

namespace
{
    class MemoryBlockWithOffset : public IMemoryBlock
    {
    private:
        void* ptr;
        size_t sizeOfData;
        size_t offset;
    public:
        MemoryBlockWithOffset() = delete;
        MemoryBlockWithOffset(const MemoryBlockWithOffset&) = delete;
        MemoryBlockWithOffset& operator=(const MemoryBlockWithOffset&) = delete;

        explicit MemoryBlockWithOffset(size_t initialSize)
            : ptr(nullptr), sizeOfData(0), offset(0)
        {
            this->ptr = malloc(initialSize);
            if (this->ptr == nullptr)
            {
                throw std::runtime_error("Failed to allocate memory block.");
            }

            this->sizeOfData = initialSize;
        }

        void* GetPtr() override { return (uint8_t*)this->ptr + this->offset; }
        size_t GetSizeOfData() const override { return this->sizeOfData - this->offset; }

        void SetOffset(size_t offset)
        {
            if (offset > this->sizeOfData)
            {
                throw invalid_argument("Offset cannot be greater than the size of the data.");
            }

            this->offset = offset;
        }

        void ReduceSize(size_t reducedSize)
        {
            //assert(reducedSize <= this->sizeOfData);
            void* new_ptr = realloc(this->ptr, reducedSize);
            if (new_ptr != nullptr)
            {
                this->ptr = new_ptr;
                this->sizeOfData = reducedSize;
            }
        }

        ~MemoryBlockWithOffset() override
        {
            free(this->ptr);
        }
    };

    struct ChunkedCompressionParameters
    {
        std::uint32_t sourceWidth;
        std::uint32_t sourceHeight;
        std::uint32_t sourceStride;
        libCZI::PixelType sourcePixeltype;
        const void* source;
        void* destination;
        size_t sizeDestination;
        std::function<void* (size_t)> allocateTempBuffer;
        std::function<void(void*)> freeTempBuffer;
        bool do_lo_hi_byte_unpacking;   // whether the pre-processing step of hi-lo byte unpacking should be applied to the chunk data before compression.
    };

    struct ChunkedCompressionOptionsZstd : public ChunkedCompressionParameters
    {
        std::uint32_t chunkSize;
        std::int32_t zstdCompressionLevel;
    };

    struct ChunkedCompressionOptionsLz4 : public ChunkedCompressionParameters
    {
        std::uint32_t chunkSize;
    };

    // This struct encapsulates the result of calculating the maximum size needed for chunked compression,
    // including both the maximum compressed data size and the maximum header size.
    struct MaxChunkedCompressionSizeResult
    {
        size_t maxCompressedSize;   ///< The maximum size of the compressed data.
        size_t maxHeaderSize;       ///< The maximum size of the compression header.
    };

    /// Calculates the maximum compressed size of all chunks combined (excluding the header).
    /// Each full chunk contributes \c ZSTD_compressBound(max_chunk_size); a partial last
    /// chunk (when \p source_data_size is not an exact multiple of \p max_chunk_size)
    /// contributes \c ZSTD_compressBound(remainder).
    ///
    /// \note  Does not include the header — use \c CalculateMaxCompressedSizeChunked for
    ///        the total buffer size (header + chunks).
    ///
    /// \exception std::invalid_argument  Thrown for unsupported \p codec values.
    ///
    /// \param  codec             The compression codec.
    /// \param  source_data_size  Total uncompressed size in bytes.
    /// \param  max_chunk_size    Maximum uncompressed size per chunk in bytes.
    ///
    /// \returns  An upper bound on the total compressed size of all chunks in bytes.
    size_t CalculateMaxSizeForChunkedCompression(ChunkedCompressionHeaderHelper::Codec codec, size_t source_data_size, uint32_t max_chunk_size)
    {
        // All chunks except (possibly) the last one have exactly max_chunk_size bytes of
        // uncompressed input.  The last chunk contains the remainder and may be smaller.
        const size_t full_chunk_count = source_data_size / max_chunk_size;
        const size_t last_chunk_size = source_data_size % max_chunk_size;

        switch (codec)
        {
        case ChunkedCompressionHeaderHelper::Codec::ZStd:
        {
            size_t total = full_chunk_count * ZSTD_compressBound(max_chunk_size);
            if (last_chunk_size > 0)
            {
                total += ZSTD_compressBound(last_chunk_size);
            }

            return total;
        }
#if (LIBCZI_LZ4_AVAILABLE)
        case ChunkedCompressionHeaderHelper::Codec::Lz4:
        {
            size_t total = full_chunk_count * static_cast<size_t>(LZ4_compressBound(static_cast<int>(max_chunk_size)));
            if (last_chunk_size > 0)
            {
                total += static_cast<size_t>(LZ4_compressBound(static_cast<int>(last_chunk_size)));
            }

            return total;
        }
#endif
        default:
            throw invalid_argument("Invalid codec specified for calculating maximum size for chunked compression.");
        }
    }

    MaxChunkedCompressionSizeResult CalculateMaxChunkedCompressionSize(size_t size,
                                                                        std::uint32_t maxChunkSize,
                                                                        ChunkedCompressionHeaderHelper::Codec codec,
                                                                        bool hiLoBytePacking)
    {
        MaxChunkedCompressionSizeResult result;
        ChunkedCompressionHeaderHelper::HeaderInfoForMaxSizeDetermination header_info_for_max_size_determination;
        header_info_for_max_size_determination.codec = codec;
        header_info_for_max_size_determination.hiLoBytePackingApplied = hiLoBytePacking;
        header_info_for_max_size_determination.number_of_chunks = static_cast<uint32_t>((size + maxChunkSize - 1) / maxChunkSize);
        result.maxHeaderSize = ChunkedCompressionHeaderHelper::DetermineMaxSizeForCompressionHeader(header_info_for_max_size_determination);

        result.maxCompressedSize = CalculateMaxSizeForChunkedCompression(codec, size, maxChunkSize);

        return result;
    }

    MaxChunkedCompressionSizeResult CalculateMaxChunkedCompressionSize(std::uint32_t sourceWidth,
                                                                        std::uint32_t sourceHeight,
                                                                        libCZI::PixelType sourcePixeltype,
                                                                        std::uint32_t maxChunkSize,
                                                                        ChunkedCompressionHeaderHelper::Codec codec,
                                                                        bool hiLoBytePacking)
    {
        const size_t line_size = sourceWidth * static_cast<size_t>(Utils::GetBytesPerPixel(sourcePixeltype));
        const size_t source_data_size = sourceHeight * line_size;
        return CalculateMaxChunkedCompressionSize(source_data_size, maxChunkSize, codec, hiLoBytePacking);
    }

#if (LIBCZI_LZ4_AVAILABLE)
    bool ChunkedCompressWithLz4(const ChunkedCompressionOptionsLz4& options, const void* source_data, size_t size_source_data, vector<uint32_t>& compressed_sizes)
    {
        const uint32_t number_of_chunks = static_cast<uint32_t>((size_source_data + options.chunkSize - 1) / options.chunkSize);

        compressed_sizes.clear();
        compressed_sizes.reserve(number_of_chunks);

        size_t offset_in_source = 0;
        size_t offset_in_destination = 0;
        for (uint32_t n = 0; n < number_of_chunks; ++n)
        {
            const int size_of_chunk = static_cast<int>(min(
                static_cast<size_t>(options.chunkSize),
                size_source_data - static_cast<size_t>(n) * options.chunkSize));

            const int r = LZ4_compress_default(
                static_cast<const char*>(source_data) + offset_in_source,
                static_cast<char*>(options.destination) + offset_in_destination,
                size_of_chunk,
                static_cast<int>(options.sizeDestination - offset_in_destination));
            if (r <= 0)
            {
                return false;
            }

            compressed_sizes.emplace_back(static_cast<uint32_t>(r));

            offset_in_source += size_of_chunk;
            offset_in_destination += r;
        }

        return true;
    }

    bool ChunkedCompressWithLz4AndLoHiBytePacking(const ChunkedCompressionOptionsLz4& options, const void* source_data, size_t size_source_data, vector<uint32_t>& compressed_sizes)
    {
        const uint32_t number_of_chunks = static_cast<uint32_t>((size_source_data + options.chunkSize - 1) / options.chunkSize);
        const size_t max_chunk_size = min(static_cast<size_t>(options.chunkSize), size_source_data);

        auto deleter = [&](void* ptr) -> void { if (ptr != nullptr) { options.freeTempBuffer(ptr); } };
        void* temp_buffer = options.allocateTempBuffer(max_chunk_size);
        if (temp_buffer == nullptr)
        {
            // allocation failed
            stringstream ss;
            ss << "Allocation of temporary buffer (of " << max_chunk_size << " bytes) failed.";
            throw runtime_error(ss.str());
        }

        compressed_sizes.clear();
        compressed_sizes.reserve(number_of_chunks);

        unique_ptr<void, decltype(deleter)> up_temp_buffer(nullptr, deleter);
        up_temp_buffer.reset(temp_buffer);

        size_t offset_in_source = 0;
        size_t offset_in_destination = 0;
        for (uint32_t n = 0; n < number_of_chunks; ++n)
        {
            const uint32_t size_of_chunk = min(
                    static_cast<uint32_t>(options.chunkSize),
                    static_cast<uint32_t>(size_source_data - static_cast<size_t>(n) * options.chunkSize));

            /*LoHiBytePackUnpack::LoHiByteUnpackStrided(
                    static_cast<const uint8_t*>(source_data) + offset_in_source,
                    size_of_chunk / 2,
                    size_of_chunk,
                    1,
                    up_temp_buffer.get());*/
            LoHiBytePackUnpack::LoHiByteUnpackByteSized(
                    static_cast<const uint8_t*>(source_data) + offset_in_source,
                    size_of_chunk,
                    up_temp_buffer.get());

            /*const int r = LZ4_compress_default(
                static_cast<const char*>(source_data) + offset_in_source,
                static_cast<char*>(options.destination) + offset_in_destination,
                size_of_chunk,
                static_cast<int>(options.sizeDestination - offset_in_destination));*/
            const int r = LZ4_compress_default(
                            static_cast<const char*>(up_temp_buffer.get()),
                            static_cast<char*>(options.destination) + offset_in_destination,
                            size_of_chunk,
                            static_cast<int>(options.sizeDestination - offset_in_destination));
            if (r <= 0)
            {
                return false;
            }

            compressed_sizes.emplace_back(static_cast<uint32_t>(r));

            offset_in_source += size_of_chunk;
            offset_in_destination += r;
        }

        return true;
    }
#endif

    bool ChunkedCompressWithZstd(const ChunkedCompressionOptionsZstd& options, const void* source_data, size_t size_source_data, vector<uint32_t>& compressed_sizes)
    {
        const uint32_t number_of_chunks = static_cast<uint32_t>((size_source_data + options.chunkSize - 1) / options.chunkSize);

        compressed_sizes.clear();
        compressed_sizes.reserve(number_of_chunks);

        size_t offset_in_source = 0;
        size_t offset_in_destination = 0;
        for (uint32_t n = 0; n < number_of_chunks; ++n)
        {
            uint32_t size_of_chunk = min(options.chunkSize, static_cast<uint32_t>(size_source_data - static_cast<size_t>(n) * options.chunkSize));

            const size_t r = ZSTD_compress(
                                    static_cast<uint8_t*>(options.destination) + offset_in_destination,
                                    options.sizeDestination - offset_in_destination,
                                    static_cast<const uint8_t*>(source_data) + offset_in_source,
                                    size_of_chunk,
                                    options.zstdCompressionLevel);
            if (ZSTD_isError(r))
            {
                return false;
            }

            // TODO(JBL): check that r does not exceed numeric_limits<uint32_t>::max() before the cast in the 
            // next statement (and handle this case appropriately, e.g. by throwing an exception), since the compressed chunk size must be representable in 4 bytes for our header format

            compressed_sizes.emplace_back(static_cast<uint32_t>(r));

            offset_in_source += size_of_chunk;
            offset_in_destination += r;
        }

        return true;
    }

    bool ChunkedCompressWithZstdAndHiLoBytePacking(const ChunkedCompressionOptionsZstd& options, const void* source_data, size_t size_source_data, vector<uint32_t>& compressed_sizes)
    {
        const uint32_t number_of_chunks = static_cast<uint32_t>((size_source_data + options.chunkSize - 1) / options.chunkSize);

        const size_t max_chunk_size = min(static_cast<size_t>(options.chunkSize), size_source_data);

        auto deleter = [&](void* ptr) -> void { if (ptr != nullptr) { options.freeTempBuffer(ptr); } };
        void* temp_buffer = options.allocateTempBuffer(max_chunk_size);
        if (temp_buffer == nullptr)
        {
            // allocation failed
            stringstream ss;
            ss << "Allocation of temporary buffer (of " << max_chunk_size << " bytes) failed.";
            throw runtime_error(ss.str());
        }

        unique_ptr<void, decltype(deleter)> up_temp_buffer(nullptr, deleter);
        up_temp_buffer.reset(temp_buffer);

        compressed_sizes.clear();
        compressed_sizes.reserve(number_of_chunks);

        size_t offset_in_source = 0;
        size_t offset_in_destination = 0;
        for (uint32_t n = 0; n < number_of_chunks; ++n)
        {
            uint32_t size_of_chunk = min(options.chunkSize, static_cast<uint32_t>(size_source_data - static_cast<size_t>(n) * options.chunkSize));

            /*LoHiBytePackUnpack::LoHiByteUnpackStrided(
                    static_cast<const uint8_t*>(source_data) + offset_in_source,
                    size_of_chunk/2,
                    size_of_chunk,
                    1,
                    up_temp_buffer.get());*/
            LoHiBytePackUnpack::LoHiByteUnpackByteSized(
                    static_cast<const uint8_t*>(source_data) + offset_in_source,
                    size_of_chunk,
                    up_temp_buffer.get());

            const size_t r = ZSTD_compress(
                                    static_cast<uint8_t*>(options.destination) + offset_in_destination,
                                    options.sizeDestination - offset_in_destination,
                                    up_temp_buffer.get(),
                                    size_of_chunk,
                                    options.zstdCompressionLevel);

            /*const size_t r = ZSTD_compress(
                                    static_cast<uint8_t*>(options.destination) + offset_in_destination,
                                    options.sizeDestination - offset_in_destination,
                                    static_cast<const uint8_t*>(source_data) + offset_in_source,
                                    size_of_chunk,
                                    options.zstdCompressionLevel);*/
            if (ZSTD_isError(r))
            {
                return false;
            }

            // TODO(JBL): check that r does not exceed numeric_limits<uint32_t>::max() before the cast in the 
            // next statement (and handle this case appropriately, e.g. by throwing an exception), since the compressed chunk size must be representable in 4 bytes for our header format

            compressed_sizes.emplace_back(static_cast<uint32_t>(r));

            offset_in_source += size_of_chunk;
            offset_in_destination += r;
        }

        return true;
    }

    std::tuple<std::uint32_t, std::uint32_t> CalculateUncompressedChunkSizesForHeader(const ChunkedCompressionOptionsZstd& options)
    {
        const size_t line_size = options.sourceWidth * static_cast<size_t>(Utils::GetBytesPerPixel(options.sourcePixeltype));
        const size_t source_data_size = options.sourceHeight * line_size;
        if (options.chunkSize >= source_data_size)
        {
            return make_tuple(static_cast<uint32_t>(source_data_size), 0);
        }
        else
        {
            const uint32_t last_chunk_size = static_cast<uint32_t>(source_data_size % options.chunkSize);
            return make_tuple(static_cast<uint32_t>(options.chunkSize), last_chunk_size);
        }
    }

    bool ChunkedCompressToDestinationBuffer(const ChunkedCompressionOptionsZstd& options, vector<uint32_t>& compressed_sizes, size_t* total_size_of_compressed_data)
    {
        const size_t bytesPerPel = Utils::GetBytesPerPixel(options.sourcePixeltype);
        const size_t line_size = options.sourceWidth * bytesPerPel;
        const size_t source_data_size = options.sourceHeight * line_size;

        const void* source_data_for_compression;

        // we use this unique_ptr to manage lifetime of the temporary buffer (if we need to allocate one),
        // to ensure that the temporary buffer is freed when we are done (even if an exception is thrown). 
        auto deleter = [&](void* ptr) -> void { if (ptr != nullptr) { options.freeTempBuffer(ptr); } };
        unique_ptr<void, decltype(deleter)> upTemp(nullptr, deleter);

        if (line_size == options.sourceStride && options.do_lo_hi_byte_unpacking == false)
        {
            // only if the stride is the minimal stride (i.e. the line size) and we do not need to apply hi-lo byte unpacking, 
            // can we directly use the source data for compression without copying it to a temporary buffer
            source_data_for_compression = options.source;
        }
        else
        {
            void* tempBuffer = options.allocateTempBuffer(source_data_size);
            if (tempBuffer == nullptr)
            {
                // allocation failed
                stringstream ss;
                ss << "Allocation of temporary buffer (of " << source_data_size << " bytes) failed.";
                throw runtime_error(ss.str());
            }

            upTemp.reset(tempBuffer);

            // copy the source data to the temporary buffer with the minimal stride (i.e. the line size), since this is required for compression, and also since this will ensure that the data is laid out in memory in a way that is optimal for compression (i.e. without "gaps" at the end of each line that would be present if the stride is larger than the line size)
            CBitmapOperations::Copy(
                options.sourcePixeltype,
                options.source,
                options.sourceStride,
                options.sourcePixeltype,
                upTemp.get(),
                line_size,
                options.sourceWidth,
                options.sourceHeight,
                false);
            
/*          if (options.do_lo_hi_byte_unpacking)
            {
                // TODO(JBL) : check requirements (line_size must be divisible by 2, etc.) for hi-lo byte unpacking, and throw if the requirements are not met
                LoHiBytePackUnpack::LoHiByteUnpackStrided(
                    options.source,
                    line_size / 2,
                    options.sourceStride,
                    options.sourceHeight,
                    upTemp.get());
            }
            else
            {
                // copy the source data to the temporary buffer with the minimal stride (i.e. the line size), since this is required for compression, and also since this will ensure that the data is laid out in memory in a way that is optimal for compression (i.e. without "gaps" at the end of each line that would be present if the stride is larger than the line size)
                CBitmapOperations::Copy(
                    options.sourcePixeltype,
                    options.source,
                    options.sourceStride,
                    options.sourcePixeltype,
                    upTemp.get(),
                    line_size,
                    options.sourceWidth,
                    options.sourceHeight,
                    false);
            }*/

            source_data_for_compression = upTemp.get();
        }

        const bool success = options.do_lo_hi_byte_unpacking ? 
                                        ChunkedCompressWithZstdAndHiLoBytePacking(options, source_data_for_compression, source_data_size, compressed_sizes) :
                                        ChunkedCompressWithZstd(options, source_data_for_compression, source_data_size, compressed_sizes);
        if (!success)
        {
            return false;
        }

        if (total_size_of_compressed_data != nullptr)
        {
            *total_size_of_compressed_data = accumulate(compressed_sizes.cbegin(), compressed_sizes.cend(), static_cast<size_t>(0));
        }

        return true;
    }

    /// This function compresses the source bitmap in chunks using zstd, prepends the chunked-compression header to the
    /// compressed data in the destination buffer, and returns the total number of bytes written (header + all compressed
    /// chunks). If the return value is 0, this indicates that compression failed because the destination buffer was not
    /// large enough to hold the compressed data.
    ///
    /// \param  options The options for chunked compression with zstd, including source/destination pointers and sizes, chunk size, and zstd compression level.
    ///
    /// \returns	The total size of the compressed data (header + chunks) in bytes, or 0 if the destination buffer was too small.
    size_t ChunkedCompressZstdAndPrependHeader(const ChunkedCompressionOptionsZstd& options)
    {
        vector<uint32_t> compressed_sizes;
        size_t total_compressed_chunks_size;
        bool success = ChunkedCompressToDestinationBuffer(options, compressed_sizes, &total_compressed_chunks_size);
        if (!success)
        {
            return 0;
        }

        // Ok - now the chunks are compressed (at the start of the destination buffer), and we have the compressed sizes for each chunk in compressed_sizes.
        // Now - we need to prepare the header information, then we need to move the compressed chunks in the destination buffer to make room for the header 
        // at the start of the buffer, and then we need to write the header at the start of the buffer. Unfortunately, we cannot write the header first and 
        // then compress the chunks after the header, since the header is variable in size, and we can only determine the size of the header after we have compressed 
        // the chunks and know the compressed sizes.
        ChunkedCompressionHeaderHelper::HeaderInfoForMaxSizeDetermination header_info_for_max_size_determination;
        header_info_for_max_size_determination.codec = ChunkedCompressionHeaderHelper::Codec::ZStd;
        header_info_for_max_size_determination.hiLoBytePackingApplied = options.do_lo_hi_byte_unpacking;
        header_info_for_max_size_determination.number_of_chunks = static_cast<std::uint32_t>(compressed_sizes.size());

        const size_t max_header_size = ChunkedCompressionHeaderHelper::DetermineMaxSizeForCompressionHeader(header_info_for_max_size_determination);
        unique_ptr<uint8_t[]> header_buffer = make_unique<uint8_t[]>(max_header_size);

        ChunkedCompressionHeaderHelper::HeaderInfoForCreation header_info_for_creation;
        header_info_for_creation.codec = ChunkedCompressionHeaderHelper::Codec::ZStd;
        header_info_for_creation.hiLoBytePackingApplied = options.do_lo_hi_byte_unpacking;
        header_info_for_creation.chunkSizes = std::move(compressed_sizes);
        header_info_for_creation.uncompressedSizes = CalculateUncompressedChunkSizesForHeader(options);

        const size_t actual_header_size = ChunkedCompressionHeaderHelper::CreateCompressionHeader(header_buffer.get(), max_header_size, header_info_for_creation);

        // now, we need to have actual_header_size bytes left in the destination buffer to write the header
        if (total_compressed_chunks_size + actual_header_size > options.sizeDestination)
        {
            return 0;  // not enough space in the destination buffer to write the header and the compressed chunks
        }

        // now, move the compressed chunks in the destination buffer to make room for the header at the start of the buffer
        memmove(static_cast<uint8_t*>(options.destination) + actual_header_size, options.destination, total_compressed_chunks_size);

        // now write the header at the start of the destination buffer
        memcpy(options.destination, header_buffer.get(), actual_header_size);

        // and - done, report the total size of the compressed data (header + compressed chunks)
        return actual_header_size + total_compressed_chunks_size;
    }

    std::tuple<std::uint32_t, std::uint32_t> CalculateUncompressedChunkSizesForLz4Header(const ChunkedCompressionOptionsLz4& options)
    {
        const size_t line_size = options.sourceWidth * static_cast<size_t>(Utils::GetBytesPerPixel(options.sourcePixeltype));
        const size_t source_data_size = options.sourceHeight * line_size;
        if (options.chunkSize >= source_data_size)
        {
            return make_tuple(static_cast<uint32_t>(source_data_size), 0);
        }
        else
        {
            const uint32_t last_chunk_size = static_cast<uint32_t>(source_data_size % options.chunkSize);
            return make_tuple(static_cast<uint32_t>(options.chunkSize), last_chunk_size);
        }
    }

#if (LIBCZI_LZ4_AVAILABLE)
    bool ChunkedCompressToDestinationBufferLz4(const ChunkedCompressionOptionsLz4& options, vector<uint32_t>& compressed_sizes, size_t* total_size_of_compressed_data)
    {
        const size_t bytesPerPel = Utils::GetBytesPerPixel(options.sourcePixeltype);
        const size_t line_size = options.sourceWidth * bytesPerPel;
        const size_t source_data_size = options.sourceHeight * line_size;

        const void* source_data_for_compression;

        //  we use this unique_ptr to manage lifetime of the temporary buffer (if we need to allocate one)
        auto deleter = [&](void* ptr) -> void {if (ptr != nullptr) { options.freeTempBuffer(ptr); } };
        unique_ptr<void, decltype(deleter)> upTemp(nullptr, deleter);

        if (line_size == options.sourceStride && options.do_lo_hi_byte_unpacking == false)
        {
            // only if the stride is the minimal stride (i.e. the line size) and we do not need to apply hi-lo byte unpacking, 
            // can we directly use the source data for compression without copying it to a temporary buffer
            source_data_for_compression = options.source;
        }
        else
        {
            void* tempBuffer = options.allocateTempBuffer(source_data_size);
            if (tempBuffer == nullptr)
            {
                stringstream ss;
                ss << "Allocation of temporary buffer (of " << source_data_size << " bytes) failed.";
                throw runtime_error(ss.str());
            }

            upTemp.reset(tempBuffer);

            //if (options.do_lo_hi_byte_unpacking)
            //{
            //    // TODO(JBL) : check requirements (line_size must be divisible by 2, etc.) for hi-lo byte unpacking, and throw if the requirements are not met
            //    LoHiBytePackUnpack::LoHiByteUnpackStrided(
            //        options.source,
            //        line_size / 2,
            //        options.sourceStride,
            //        options.sourceHeight,
            //        upTemp.get());
            //}
            //else
            //{
            //    CBitmapOperations::Copy(
            //        options.sourcePixeltype,
            //        options.source,
            //        options.sourceStride,
            //        options.sourcePixeltype,
            //        upTemp.get(),
            //        line_size,
            //        options.sourceWidth,
            //        options.sourceHeight,
            //        false);
            //}
            CBitmapOperations::Copy(
                    options.sourcePixeltype,
                    options.source,
                    options.sourceStride,
                    options.sourcePixeltype,
                    upTemp.get(),
                    line_size,
                    options.sourceWidth,
                    options.sourceHeight,
                    false);

            source_data_for_compression = upTemp.get();
        }

        const bool success = options.do_lo_hi_byte_unpacking ? 
                                    ChunkedCompressWithLz4AndLoHiBytePacking(options, source_data_for_compression, source_data_size, compressed_sizes) : 
                                    ChunkedCompressWithLz4(options, source_data_for_compression, source_data_size, compressed_sizes);
        if (!success)
        {
            return false;
        }

        if (total_size_of_compressed_data != nullptr)
        {
            *total_size_of_compressed_data = accumulate(compressed_sizes.cbegin(), compressed_sizes.cend(), static_cast<size_t>(0));
        }

        return true;
    }
#endif

#if (LIBCZI_LZ4_AVAILABLE)
    size_t ChunkedCompressLz4AndPrependHeader(const ChunkedCompressionOptionsLz4& options)
    {
        vector<uint32_t> compressed_sizes;
        size_t total_compressed_chunks_size;
        bool success = ChunkedCompressToDestinationBufferLz4(options, compressed_sizes, &total_compressed_chunks_size);
        if (!success)
        {
            return 0;
        }

        ChunkedCompressionHeaderHelper::HeaderInfoForMaxSizeDetermination header_info_for_max_size_determination;
        header_info_for_max_size_determination.codec = ChunkedCompressionHeaderHelper::Codec::Lz4;
        header_info_for_max_size_determination.hiLoBytePackingApplied = options.do_lo_hi_byte_unpacking;
        header_info_for_max_size_determination.number_of_chunks = static_cast<std::uint32_t>(compressed_sizes.size());

        const size_t max_header_size = ChunkedCompressionHeaderHelper::DetermineMaxSizeForCompressionHeader(header_info_for_max_size_determination);
        unique_ptr<uint8_t[]> header_buffer = make_unique<uint8_t[]>(max_header_size);

        ChunkedCompressionHeaderHelper::HeaderInfoForCreation header_info_for_creation;
        header_info_for_creation.codec = ChunkedCompressionHeaderHelper::Codec::Lz4;
        header_info_for_creation.hiLoBytePackingApplied = options.do_lo_hi_byte_unpacking;
        header_info_for_creation.chunkSizes = std::move(compressed_sizes);
        header_info_for_creation.uncompressedSizes = CalculateUncompressedChunkSizesForLz4Header(options);

        const size_t actual_header_size = ChunkedCompressionHeaderHelper::CreateCompressionHeader(header_buffer.get(), max_header_size, header_info_for_creation);

        if (total_compressed_chunks_size + actual_header_size > options.sizeDestination)
        {
            return 0;
        }

        memmove(static_cast<uint8_t*>(options.destination) + actual_header_size, options.destination, total_compressed_chunks_size);
        memcpy(options.destination, header_buffer.get(), actual_header_size);

        return actual_header_size + total_compressed_chunks_size;
    }
#endif

    // This function determines the maximum chunk size to use for chunked compression, based on the given parameters. If 
    // the parameters do not specify a valid chunk size, a default of 64kb is used.
    uint32_t DetermineMaxChunkSize(const ICompressParameters* parameters)
    {
        uint32_t max_chunk_size = 64 * 1024;    // use a default of 64kb

        CompressParameter parameter;
        if (parameters != nullptr && parameters->TryGetProperty(CompressionParameterKey::CHUNKEDCOMPRESSION_MAXCHUNKSIZE, &parameter) &&
            parameter.GetType() == CompressParameter::Type::Uint32)
        {
            const uint32_t value = parameter.GetUInt32();
            if (value > 0)
            {
                max_chunk_size = value;
            }
        }

        return max_chunk_size;
    }

    ChunkedCompressionHeaderHelper::Codec DetermineCompressionCodec(const ICompressParameters* parameters)
    {
        ChunkedCompressionHeaderHelper::Codec compression_method = ChunkedCompressionHeaderHelper::Codec::ZStd;
        CompressParameter parameter;
        if (parameters != nullptr && parameters->TryGetProperty(CompressionParameterKey::CHUNKEDCOMPRESSION_CODEC, &parameter) &&
            parameter.GetType() == CompressParameter::Type::Int32)
        {
            switch (parameter.GetInt32())
            {
            case static_cast<int32_t>(ChunkedCompressionHeaderHelper::Codec::ZStd):
                compression_method = ChunkedCompressionHeaderHelper::Codec::ZStd;
                break;
            case static_cast<int32_t>(ChunkedCompressionHeaderHelper::Codec::Lz4):
                compression_method = ChunkedCompressionHeaderHelper::Codec::Lz4;
                break;
            default:
                throw invalid_argument("Invalid compression method specified in the parameters for ChunkedCompress::Compress.");
            }
        }

        return compression_method;
    }

    int32_t DetermineZstdCompressionLevel(const ICompressParameters* parameters)
    {
        int32_t zstd_compression_level = 0;  // will be set to the default level later if not specified in the parameters

        CompressParameter parameter;
        if (parameters != nullptr && parameters->TryGetProperty(CompressionParameterKey::CHUNKEDCOMPRESSION_RAWCOMPRESSIONLEVEL_ZSTD, &parameter) &&
            parameter.GetType() == CompressParameter::Type::Int32)
        {
            zstd_compression_level = Utilities::clamp(parameter.GetInt32(), ZSTD_minCLevel(), ZSTD_maxCLevel());
        }

        return zstd_compression_level;
    }

    bool DetermineWhetherToUseHiLoBytePreprocessing(libCZI::PixelType source_pixel_type, const ICompressParameters* parameters)
    {
        bool do_lo_hi_byte_packing = false;
        if (parameters != nullptr &&
            (source_pixel_type == PixelType::Bgr48 || source_pixel_type == PixelType::Gray16))
        {
            CompressParameter parameter;
            if (parameters->TryGetProperty(CompressionParameterKey::CHUNKEDCOMPRESSION_DOLOHIBYTEUNPACKING, &parameter) &&
                parameter.GetType() == CompressParameter::Type::Boolean)
            {
                do_lo_hi_byte_packing = parameter.GetBoolean();
            }
        }

        return do_lo_hi_byte_packing;
    }
}

bool ChunkedCompress::Compress(
            std::uint32_t sourceWidth,
            std::uint32_t sourceHeight,
            std::uint32_t sourceStride,
            libCZI::PixelType sourcePixeltype,
            const void* source,
            void* destination,
            size_t& sizeDestination,
            const std::function<void* (size_t)>& allocateTempBuffer,
            const std::function<void(void*)>& freeTempBuffer,
            const ICompressParameters* parameters)
{
    CompressionUtilities::CheckSourceBitmapArgumentsAndThrow(sourceWidth, sourceHeight, sourceStride, sourcePixeltype, source);
    CompressionUtilities::CheckDestinationArgumentsAndThrow(destination, sizeDestination, 1);
    CompressionUtilities::CheckTempBufferAllocArgumentsAndThrow(allocateTempBuffer, freeTempBuffer);

    const ChunkedCompressionHeaderHelper::Codec compression_method = DetermineCompressionCodec(parameters);
    const uint32_t max_chunk_size = DetermineMaxChunkSize(parameters);
    const bool do_lo_hi_byte_packing = DetermineWhetherToUseHiLoBytePreprocessing(sourcePixeltype, parameters);

    switch (compression_method)
    {
    case ChunkedCompressionHeaderHelper::Codec::ZStd:
    {
        ChunkedCompressionOptionsZstd options_zstd;
        options_zstd.sourceWidth = sourceWidth;
        options_zstd.sourceHeight = sourceHeight;
        options_zstd.sourceStride = sourceStride;
        options_zstd.sourcePixeltype = sourcePixeltype;
        options_zstd.source = source;
        options_zstd.destination = destination;
        options_zstd.sizeDestination = sizeDestination;
        options_zstd.allocateTempBuffer = allocateTempBuffer;
        options_zstd.freeTempBuffer = freeTempBuffer;
        options_zstd.do_lo_hi_byte_unpacking = do_lo_hi_byte_packing;

        options_zstd.zstdCompressionLevel = DetermineZstdCompressionLevel(parameters);
        options_zstd.chunkSize = max_chunk_size;

        const size_t size_compressed = ChunkedCompressZstdAndPrependHeader(options_zstd);
        if (size_compressed == 0)
        {
            return false;  // compression failed due to insufficient destination buffer size
        }

        sizeDestination = size_compressed;
        return true;
    }
#if (LIBCZI_LZ4_AVAILABLE)
    case ChunkedCompressionHeaderHelper::Codec::Lz4:
    {
        ChunkedCompressionOptionsLz4 options_lz4;
        options_lz4.sourceWidth = sourceWidth;
        options_lz4.sourceHeight = sourceHeight;
        options_lz4.sourceStride = sourceStride;
        options_lz4.sourcePixeltype = sourcePixeltype;
        options_lz4.source = source;
        options_lz4.destination = destination;
        options_lz4.sizeDestination = sizeDestination;
        options_lz4.allocateTempBuffer = allocateTempBuffer;
        options_lz4.freeTempBuffer = freeTempBuffer;
        options_lz4.do_lo_hi_byte_unpacking = do_lo_hi_byte_packing;

        options_lz4.chunkSize = max_chunk_size;

        const size_t size_compressed = ChunkedCompressLz4AndPrependHeader(options_lz4);
        if (size_compressed == 0)
        {
            return false;
        }

        sizeDestination = size_compressed;
        return true;
    }
#endif
    default:
        throw invalid_argument("Invalid compression method specified in the parameters for ChunkedCompress::Compress.");
    }
}

bool ChunkedCompress::Compress(
            std::uint32_t sourceWidth,
            std::uint32_t sourceHeight,
            std::uint32_t sourceStride,
            libCZI::PixelType sourcePixeltype,
            const void* source,
            void* destination,
            size_t& sizeDestination,
            const ICompressParameters* parameters)
{
    return ChunkedCompress::Compress(sourceWidth, sourceHeight, sourceStride, sourcePixeltype, source, destination, sizeDestination, malloc, free, parameters);
}

std::shared_ptr<IMemoryBlock> ChunkedCompress::CompressToMemoryBlock(
            std::uint32_t sourceWidth,
            std::uint32_t sourceHeight,
            std::uint32_t sourceStride,
            libCZI::PixelType sourcePixeltype,
            const void* source,
            const ICompressParameters* parameters)
{
    return ChunkedCompress::CompressToMemoryBlock(sourceWidth, sourceHeight, sourceStride, sourcePixeltype, source, malloc, free, parameters);
}

std::shared_ptr<IMemoryBlock> ChunkedCompress::CompressToMemoryBlock(
    std::uint32_t sourceWidth,
    std::uint32_t sourceHeight,
    std::uint32_t sourceStride,
    libCZI::PixelType sourcePixeltype,
    const void* source,
    const std::function<void* (size_t)>& allocateTempBuffer,
    const std::function<void(void*)>& freeTempBuffer,
    const ICompressParameters* parameters)
{
    CompressionUtilities::CheckSourceBitmapArgumentsAndThrow(sourceWidth, sourceHeight, sourceStride, sourcePixeltype, source);
    CompressionUtilities::CheckTempBufferAllocArgumentsAndThrow(allocateTempBuffer, freeTempBuffer);

    const ChunkedCompressionHeaderHelper::Codec compression_method = DetermineCompressionCodec(parameters);
    const uint32_t max_chunk_size = DetermineMaxChunkSize(parameters);
    const bool do_lo_hi_byte_packing = DetermineWhetherToUseHiLoBytePreprocessing(sourcePixeltype, parameters);

    const auto maximum_sizes = CalculateMaxChunkedCompressionSize(sourceWidth, sourceHeight, sourcePixeltype, max_chunk_size, compression_method, do_lo_hi_byte_packing);

    auto mem_blk = make_shared<MemoryBlockWithOffset>(maximum_sizes.maxHeaderSize + maximum_sizes.maxCompressedSize);

    vector<uint32_t> compressed_sizes;
    size_t total_compressed_chunks_size;
    if (compression_method == ChunkedCompressionHeaderHelper::Codec::ZStd)
    {
        ChunkedCompressionOptionsZstd options_zstd;
        options_zstd.sourceWidth = sourceWidth;
        options_zstd.sourceHeight = sourceHeight;
        options_zstd.sourceStride = sourceStride;
        options_zstd.sourcePixeltype = sourcePixeltype;
        options_zstd.source = source;
        options_zstd.destination = static_cast<uint8_t*>(mem_blk->GetPtr()) + maximum_sizes.maxHeaderSize;
        options_zstd.sizeDestination = maximum_sizes.maxCompressedSize;
        options_zstd.allocateTempBuffer = allocateTempBuffer;
        options_zstd.freeTempBuffer = freeTempBuffer;
        options_zstd.do_lo_hi_byte_unpacking = do_lo_hi_byte_packing;

        options_zstd.zstdCompressionLevel = DetermineZstdCompressionLevel(parameters);
        options_zstd.chunkSize = max_chunk_size;

        bool success = ChunkedCompressToDestinationBuffer(options_zstd, compressed_sizes, &total_compressed_chunks_size);
        if (!success)
        {
            throw runtime_error("Compression failed due to insufficient destination buffer size. This should not happen since we have allocated a memory block of the maximum size needed for the compressed data.");
        }

        mem_blk->ReduceSize(maximum_sizes.maxHeaderSize + total_compressed_chunks_size);

        // now, prepare the header
        ChunkedCompressionHeaderHelper::HeaderInfoForCreation header_info_for_creation;
        header_info_for_creation.codec = ChunkedCompressionHeaderHelper::Codec::ZStd;
        header_info_for_creation.hiLoBytePackingApplied = do_lo_hi_byte_packing;
        header_info_for_creation.chunkSizes = std::move(compressed_sizes);
        header_info_for_creation.uncompressedSizes = CalculateUncompressedChunkSizesForHeader(options_zstd);

        const size_t actual_header_size = ChunkedCompressionHeaderHelper::CreateCompressionHeader(mem_blk->GetPtr(), maximum_sizes.maxHeaderSize, header_info_for_creation);
        if (actual_header_size < maximum_sizes.maxHeaderSize)
        {
            // if the actual header size is smaller than the maximum header size, 
            // we move the header in front of the compressed chunks
            memmove(
                static_cast<uint8_t*>(mem_blk->GetPtr()) + (maximum_sizes.maxHeaderSize - actual_header_size),
                static_cast<uint8_t*>(mem_blk->GetPtr()),
                actual_header_size);
            mem_blk->SetOffset(maximum_sizes.maxHeaderSize - actual_header_size);
        }

        return mem_blk;
    }
#if (LIBCZI_LZ4_AVAILABLE)
    else if (compression_method == ChunkedCompressionHeaderHelper::Codec::Lz4)
    {
        ChunkedCompressionOptionsLz4 options_lz4;
        options_lz4.sourceWidth = sourceWidth;
        options_lz4.sourceHeight = sourceHeight;
        options_lz4.sourceStride = sourceStride;
        options_lz4.sourcePixeltype = sourcePixeltype;
        options_lz4.source = source;
        options_lz4.destination = static_cast<uint8_t*>(mem_blk->GetPtr()) + maximum_sizes.maxHeaderSize;
        options_lz4.sizeDestination = maximum_sizes.maxCompressedSize;
        options_lz4.allocateTempBuffer = allocateTempBuffer;
        options_lz4.freeTempBuffer = freeTempBuffer;
        options_lz4.do_lo_hi_byte_unpacking = do_lo_hi_byte_packing;
        options_lz4.chunkSize = max_chunk_size;

        bool success = ChunkedCompressToDestinationBufferLz4(options_lz4, compressed_sizes, &total_compressed_chunks_size);
        if (!success)
        {
            throw runtime_error("LZ4 compression failed due to insufficient destination buffer size. This should not happen since we have allocated a memory block of the maximum size needed for the compressed data.");
        }

        mem_blk->ReduceSize(maximum_sizes.maxHeaderSize + total_compressed_chunks_size);

        ChunkedCompressionHeaderHelper::HeaderInfoForCreation header_info_for_creation;
        header_info_for_creation.codec = ChunkedCompressionHeaderHelper::Codec::Lz4;
        header_info_for_creation.hiLoBytePackingApplied = do_lo_hi_byte_packing;
        header_info_for_creation.chunkSizes = std::move(compressed_sizes);
        header_info_for_creation.uncompressedSizes = CalculateUncompressedChunkSizesForLz4Header(options_lz4);

        const size_t actual_header_size = ChunkedCompressionHeaderHelper::CreateCompressionHeader(mem_blk->GetPtr(), maximum_sizes.maxHeaderSize, header_info_for_creation);
        if (actual_header_size < maximum_sizes.maxHeaderSize)
        {
            memmove(
                static_cast<uint8_t*>(mem_blk->GetPtr()) + (maximum_sizes.maxHeaderSize - actual_header_size),
                static_cast<uint8_t*>(mem_blk->GetPtr()),
                actual_header_size);
            mem_blk->SetOffset(maximum_sizes.maxHeaderSize - actual_header_size);
        }

        return mem_blk;
    }
#endif
    else
    {
        throw invalid_argument("Unsupported compression method.");
    }
}

size_t ChunkedCompressionHeaderHelper::CalculateMaxCompressedSizeChunked(
                                                                    std::uint32_t sourceWidth,
                                                                    std::uint32_t sourceHeight,
                                                                    libCZI::PixelType sourcePixeltype,
                                                                    std::uint32_t maxChunkSize,
                                                                    Codec codec,
                                                                    bool hiLoBytePacking)
{
    const auto sizes = CalculateMaxChunkedCompressionSize(sourceWidth, sourceHeight, sourcePixeltype, maxChunkSize, codec, hiLoBytePacking);
    return sizes.maxHeaderSize + sizes.maxCompressedSize;
}
#endif
