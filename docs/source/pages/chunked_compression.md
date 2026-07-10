Experimental chunked compression
================================

## Status

Chunked compression is an experimental CZI subblock compression mode. Its
binary format and libCZI API surface may still change, and support is only
available in builds where the experimental feature is enabled.

The CZI compression mode is `CompressionMode::ChunkedExtensible` with the raw
compression-mode value `7`. The informal compression string used by libCZI is
`chunked`.

Builds that enable the feature define
`LIBCZI_EXPERIMENTAL_CHUNKED_COMPRESSION_AVAILABLE` to `1` in
`libCZI_Config.h`. Builds that disable the feature define it to `0`; callers
should use this macro before referencing chunked-compression-only APIs.

## Motivation

The existing zstd compression modes store a subblock as one compressed payload.
Chunked compression instead divides the logical pixel stream into bounded,
independently compressed chunks and stores the chunk sizes in a small header.

This is intended to support implementations that work concurrently at tile or
chunk level. It also makes it easier to use hardware-accelerated codec backends,
for example GPU-accelerated zstd or LZ4 implementations such as nvCOMP, where
batched compression and decompression of many independent chunks is a natural
execution model.

The format is a container around an actual per-chunk codec. The currently
defined per-chunk codecs are zstd and LZ4.

## Build configuration

Chunked compression is controlled by two CMake options:

- `LIBCZI_BUILD_ENABLE_EXPERIMENTAL_FUNCTIONALITY`: Global policy for
  experimental features. The supported values are `AUTO`, `ON`, and `OFF`.
  `AUTO` is the conservative default and keeps chunked compression disabled
  unless the feature-specific option is set.
- `LIBCZI_BUILD_EXPERIMENTAL_CHUNKED_COMPRESSION`: Feature-specific switch for
  chunked compression. When this option is set explicitly, it takes precedence
  over the global experimental policy.

For example:

```bash
cmake .. -DLIBCZI_BUILD_EXPERIMENTAL_CHUNKED_COMPRESSION=ON
```

or, to enable all experimental features by default:

```bash
cmake .. -DLIBCZI_BUILD_ENABLE_EXPERIMENTAL_FUNCTIONALITY=ON
```

When chunked compression is enabled, libCZI also needs LZ4. The option
`LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_LZ4` controls whether CMake should use an
externally provided LZ4 package or fetch and build a private copy.

## Using the mode

The compression options parser accepts the informal mode name `chunked`. The
chunked-specific options are:

- `ChunkedMaxChunkSize`: Maximum uncompressed chunk size in bytes.
- `ChunkedCodec`: Per-chunk codec. Supported values are `zstd` and `lz4`.
- `ExplicitLevel`: zstd compression level when `ChunkedCodec=zstd`.
- `PreProcess=HiLoByteUnpack`: Request hi/lo byte packing preprocessing where
  applicable.

Examples:

```text
chunked:ChunkedCodec=zstd;ChunkedMaxChunkSize=65536
chunked:ChunkedCodec=lz4;ChunkedMaxChunkSize=65536
chunked:ChunkedCodec=zstd;ChunkedMaxChunkSize=65536;ExplicitLevel=3
```

## Format overview

A chunked-compressed CZI subblock payload consists of a chunked-compression
header followed immediately by the compressed chunk data:

```text
+-----------------------------+
| chunked-compression header  |
+-----------------------------+
| compressed data chunk 0     |
+-----------------------------+
| compressed data chunk 1     |
+-----------------------------+
| ...                         |
+-----------------------------+
| compressed data chunk n     |
+-----------------------------+
```

The compressed chunks have no per-chunk frame in the payload. Chunk boundaries
are derived from the compressed sizes stored in the header. The uncompressed
sizes stored in the header describe the destination layout after decompression
and before any size-mismatch resolution protocol is applied.

The logical input stream is the image data described by width, height, and pixel
type, using the minimal row stride `width * bytes-per-pixel`. Any padding in a
caller-provided source stride is not part of the logical stream. The compressor
splits this logical stream into chunks of at most `ChunkedMaxChunkSize`; chunk
boundaries do not need to align to image rows.

## Integer encoding

Header fields use a little-endian seven-bit-group varint encoding. Each byte
contributes seven payload bits. The high bit indicates that another byte follows
except on the final byte. The least significant seven bits are stored first.

The implementation uses bounded variants of this encoding:

| Field | Encoded size | Value range |
| --- | ---: | ---: |
| Header chunk ID | 1-2 bytes | 0 to 16,383 |
| Header chunk payload length | 1-3 bytes | 0 to 4,194,303 |
| Compressed chunk size | 1-4 bytes | 0 to 536,870,911 |
| Decompressed chunk size | 1-4 bytes | 0 to 536,870,911 |

## Header structure

The header is a sequence of header chunks terminated by an end marker.

For every header chunk except the end marker, the layout is:

```text
header-chunk-id | payload-length | payload
```

`header-chunk-id` is a 1-2 byte varint. `payload-length` is a 1-3 byte varint
giving the byte size of the payload. The end marker is the header chunk ID `0`
encoded by itself; it has no length and no payload.

The currently defined header chunk IDs are:

| ID | Name | Required | Payload |
| ---: | --- | --- | --- |
| 0 | `EndOfHeader` | yes | none |
| 1 | `ChunkSizes` | yes | compressed chunk sizes |
| 2 | `CompressionMethod` | no | per-chunk codec |
| 3 | `DecompressedSizes` | yes | uncompressed chunk sizes |
| 4 | `Preprocessing` | no | preprocessing method |

libCZI's writer currently emits header chunks in this order:

```text
ChunkSizes
DecompressedSizes
CompressionMethod, only when the codec is not zstd
Preprocessing, when explicitly specified
EndOfHeader
```

The parser interprets chunks by ID, not by position. Unknown header chunk IDs
are rejected by the current implementation.

## ChunkSizes header chunk

`ChunkSizes` has ID `1` and is required. Its payload is a sequence of 1-4 byte
varints, one per compressed data chunk, in chunk order:

```text
compressed-size[0], compressed-size[1], ..., compressed-size[n]
```

The number of values in this header chunk is the number of compressed data
chunks following the header.

## DecompressedSizes header chunk

`DecompressedSizes` has ID `3` and is required. Its payload is a compact
sequence of 1-4 byte varints describing uncompressed chunk sizes.

The common forms are:

```text
[C]
```

All chunks decompress to `C` bytes.

```text
[C, L]
```

All chunks except the last decompress to `C` bytes. The last chunk decompresses
to `L` bytes.

The parser also supports the more general suffix form used by the implementation:
values are listed in chunk order, and if fewer values are present than chunks,
the second-to-last value repeats for the earlier chunks that are not explicitly
listed. The writer uses the one-value or two-value forms.

## CompressionMethod header chunk

`CompressionMethod` has ID `2` and is optional. Its payload is exactly one byte.

| Value | Codec |
| ---: | --- |
| 0 | zstd |
| 1 | LZ4 |

If the header chunk is absent, the codec defaults to zstd. A chunked-compressed
subblock uses one codec for all chunks in that subblock.

## Preprocessing header chunk

`Preprocessing` has ID `4` and is optional. Its payload is exactly one byte.

| Value | Meaning |
| ---: | --- |
| 0 | no preprocessing |
| 1 | hi/lo byte packing was applied before compression |

If the header chunk is absent, no preprocessing is assumed.

Hi/lo byte packing is used for pixel formats where byte-plane reordering can
improve compression, such as `Gray16` and `Bgr48`. A decoder first decompresses
the chunks and then reverses this preprocessing step before returning bitmap
data.

## Decoder validation

A decoder should:

1. Walk the header until `EndOfHeader`.
2. Parse `ChunkSizes` and `DecompressedSizes`.
3. Apply defaults for omitted optional chunks: zstd for `CompressionMethod` and
   no preprocessing for `Preprocessing`.
4. Validate that the codec and preprocessing values are supported.
5. Verify that the compressed chunk sizes fit within the subblock payload.
6. Decompress each chunk independently in chunk order.
7. Reverse hi/lo byte packing when requested by the header.

For normal data written by libCZI, the sum of the decompressed chunk sizes should
match the expected bitmap size computed from the subblock pixel type, width, and
height. libCZI also contains a compatibility path for selected size-mismatch
cases, controlled by the existing chunked-compression data-size mismatch option.
