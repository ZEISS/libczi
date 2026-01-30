Version history
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
 0.62.7             | [122](https://github.com/ZEISS/libczi/pull/122)      | documentation update
 0.63.0             | [123](https://github.com/ZEISS/libczi/pull/123)      | introduce "frames-of-reference"
 0.63.1             | [128](https://github.com/ZEISS/libczi/pull/128)      | fix for CZICmd (command "ExtractAttachment"), improve UTF8-handling (on Windows)
 0.63.2             | [129](https://github.com/ZEISS/libczi/pull/129)      | update zstd to [version 1.5.7](https://github.com/facebook/zstd/releases/tag/v1.5.7)
 0.64.0             | [130](https://github.com/ZEISS/libczi/pull/130)      | define & implement "Resolution Protocol for Ambiguous or Contradictory Information"
 0.65.0             | [134](https://github.com/ZEISS/libczi/pull/134)      | introduce "libCZIAPI", use Sphinx for documentation
 0.65.1             | [136](https://github.com/ZEISS/libczi/pull/136)      | improve error handling in libCZIAPI (for "external streams")
 0.66.0             | [138](https://github.com/ZEISS/libczi/pull/138)      | add TryGetSubBlockInfoForIndex in libCZIAPI
 0.66.1             | [142](https://github.com/ZEISS/libczi/pull/142)      | update on CMake build system for vcpkg support
 0.66.2             | [143](https://github.com/ZEISS/libczi/pull/143)      | additional updates on CMake build system towards vcpkg support
 0.66.3             | [144](https://github.com/ZEISS/libczi/pull/144)      | add (initial and experimental) support for building for UWP (Universal Windows Platform)
 0.66.4             | [145](https://github.com/ZEISS/libczi/pull/145)      | have embedded pugixml in its own namespace, replace md5sum implementation, update 3rd-party license texts
 0.66.5             | [146](https://github.com/ZEISS/libczi/pull/146)      | have jxrlib-symbols mangled to prevent conflicts with other libraries
 0.66.6             | [149](https://github.com/ZEISS/libczi/pull/149)      | prepare for vcpkg features
 0.66.7             | [151](https://github.com/ZEISS/libczi/pull/151)      | fix for missing tiles with scaling compositor in rare circumstances
 0.67.0             | [153](https://github.com/ZEISS/libczi/pull/153)      | add mask support
 0.67.1             | [157](https://github.com/ZEISS/libczi/pull/157)      | fix compilation issue on macOS
 0.67.2             | [155](https://github.com/ZEISS/libczi/pull/155)      | code cleanup
 0.67.3             | [158](https://github.com/ZEISS/libczi/pull/158)      | have all internal code in its own namespace `libCZI::detail`, update vendored pugixml to version 1.15, fix issue with big-endian-machines
 0.67.4             | [158](https://github.com/ZEISS/libczi/pull/159)      | remove version requirement for external eigen3 package
