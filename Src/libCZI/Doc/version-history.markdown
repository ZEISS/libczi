version history                 {#version_history}
============

 version            |  PR                                                  | comment
 ------------------ | ---------------------------------------------------- | ---------------------------------------------------
 0.53.0             | [67](https://github.com/ZEISS/libczi/pull/67)        | refactoring of the JPGXR-codec, making JPGXR-encoding accessible
 0.53.1             | [68](https://github.com/ZEISS/libczi/pull/68)        | preserve the order of entries in attachment-directory (as they were added to the writer)
 0.53.2             | [70](https://github.com/ZEISS/libczi/pull/70)        | add option to write 'duplicate' subblocks
 0.54.0             | [71](https://github.com/ZEISS/libczi/pull/71)        | introduce 'streamsLib', add curl-based stream class
 0.54.2             | [74](https://github.com/ZEISS/libczi/pull/74)        | minor bug fix
 0.54.3             | [78](https://github.com/ZEISS/libczi/pull/78)        | optimization: for multi-tile-composition, check relevant tiles for visibility before loading them (and do not load/decode non-visible tiles)