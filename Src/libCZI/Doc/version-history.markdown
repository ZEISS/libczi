version history                 {#version_history}
============

 version            |  PR                                                  | comment
 ------------------ | ---------------------------------------------------- | ---------------------------------------------------
 0.53.0             | [67](https://github.com/ZEISS/libczi/pull/67)        | refactoring of the JPGXR-codec, making JPGXR-encoding accessible
 0.53.1             | [68](https://github.com/ZEISS/libczi/pull/68)        | preserve the order of entries in attachment-directory (as they were added to the writer)
 0.53.2             | [70](https://github.com/ZEISS/libczi/pull/70)        | add option to write 'duplicate' subblocks
 0.54.0             | [71](https://github.com/ZEISS/libczi/pull/71)        | introduce 'streamsLib', add curl-based stream class
 0.54.2             | [74](https://github.com/ZEISS/libczi/pull/74)        | minor bug fix
 0.54.3             | [79](https://github.com/ZEISS/libczi/pull/79)        | add option _kCurlHttp_FollowLocation_ to follow HTTP redirects tp curl_http_inputstream
 0.55.0             | [78](https://github.com/ZEISS/libczi/pull/78)        | optimization: for multi-tile-composition, check relevant tiles for visibility before loading them (and do not load/decode non-visible tiles)
 0.55.1             | [80](https://github.com/ZEISS/libczi/pull/80)        | bugfix for above optimization
 0.56.0             | [82](https://github.com/ZEISS/libczi/pull/82)        | add option "kCurlHttp_CaInfo" & "kCurlHttp_CaInfoBlob", allow to retrieve properties from a stream-class
 0.57.0             | [84](https://github.com/ZEISS/libczi/pull/84)        | add caching for accessors, update CLI11 to version 2.3.2
 0.57.1             | [86](https://github.com/ZEISS/libczi/pull/86)        | small improvement for CMake-build: allow to use an apt-provided CURL-package
 0.57.2             | [90](https://github.com/ZEISS/libczi/pull/90)        | improve thread-safety of CziReader
 0.57.3             | [91](https://github.com/ZEISS/libczi/pull/91)        | improve error-message
 0.58.0             | [92](https://github.com/ZEISS/libczi/pull/92)        | export a list with properties for streams-property-bag
 0.58.1             | [95](https://github.com/ZEISS/libczi/pull/95)        | some fixes for CziReaderWriter
 0.58.2             | [96](https://github.com/ZEISS/libczi/pull/96)        | small fixes for deficiencies reported by CodeQL
 0.58.3             | [97](https://github.com/ZEISS/libczi/pull/97)        | update zstd to [version 1.5.6](https://github.com/facebook/zstd/releases/tag/v1.5.6)
 0.58.4             | [99](https://github.com/ZEISS/libczi/pull/99)        | fix a rare issue with curl_http_inputstream which would fail to read CZIs with an attachment-directory containing zero entries
 0.59.0             | [99](https://github.com/ZEISS/libczi/pull/103)       | add a check for physical size for dimensions other than X,Y,M, they must not be >1, active for strict parsing.
 0.60.0             | [106](https://github.com/ZEISS/libczi/pull/106)      | with metadata-builder, by default copy the attributes "Id" and "Name" from the channel-node; allow to control the behavior fine-grained
 0.61.0             | [109](https://github.com/ZEISS/libczi/pull/109)      | fix behaviour of `IXmlNodeRead::GetChildNodeReadonly` (for non-existing nodes), new method `ICziWriter::GetStatistics` added
 0.61.1             | [110](https://github.com/ZEISS/libczi/pull/110)      | some code cleanup
 0.61.2             | [111](https://github.com/ZEISS/libczi/pull/111)      | update libcurl to 8.9.1 (for build with `LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_LIBCURL=OFF`), enable SChannel (on Windows) by default
 0.62.0             | [112](https://github.com/ZEISS/libczi/pull/112)      | add Azure-SDK based reader for reading from Azure Blob Storage, raise requirement to C++14 for building libCZI (previously C++11 was sufficient) because Azure-SDK requires C++14
 0.62.1             | [114](https://github.com/ZEISS/libczi/pull/114)      | improve build system fixing issues with msys2 and mingw-w64, cosmetic changes
 0.62.2             | [115](https://github.com/ZEISS/libczi/pull/115)      | enabling building with clang on windows
 0.62.3             | [116](https://github.com/ZEISS/libczi/pull/116)      | enable long paths on Windows for CZIcmd, add Windows-ARM64 build
 0.62.4             | [117](https://github.com/ZEISS/libczi/pull/117)      | fix build with private RapidJSON library
 0.62.5             | [119](https://github.com/ZEISS/libczi/pull/119)      | fix a discrepancy between code and documentation
 0.62.6             | [120](https://github.com/ZEISS/libczi/pull/120)      | fix workload identity in the azure blob inputstream