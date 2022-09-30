# libCZI
[![License: LGPL v3](https://img.shields.io/badge/License-LGPL_v3-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0)
[![REUSE status](https://api.reuse.software/badge/github.com/ZEISS/libczi)](https://api.reuse.software/info/github.com/ZEISS/libczi)
[![CMake](https://github.com/ZEISS/libczi/actions/workflows/cmake.yml/badge.svg?branch=main&event=push)](https://github.com/ZEISS/libczi/actions/workflows/cmake.yml)
[![CodeQL](https://github.com/ZEISS/libczi/actions/workflows/codeql.yml/badge.svg?branch=main&event=push)](https://github.com/ZEISS/libczi/actions/workflows/codeql.yml)
[![GitHub Pages](https://github.com/ZEISS/libczi/actions/workflows/pages.yml/badge.svg?branch=main&event=push)](https://github.com/ZEISS/libczi/actions/workflows/pages.yml)

## What
libCZI is an Open Source Cross-Platform C++ library to read and write [CZI](https://www.zeiss.com/microscopy/en/products/software/zeiss-zen/czi-image-file-format.html).

## Why 
libCZI is a library intended for providing read and write access to [CZI](https://www.zeiss.com/microscopy/en/products/software/zeiss-zen/czi-image-file-format.html) featuring:

* reading subblocks and get the content as a bitmap
* reading subblocks which are compressed with JPEG-XR
* works with tiled images and pyramid images
* composing multi-channel images with tinting and applying a gradation curve
* access metadata
* writing subblocks and metadata

In a nutshell, it offers (almost...) the same functionality as the 2D-Viewer in [ZEN](https://www.zeiss.com/microscopy/en/products/software/zeiss-zen.html) - in terms of composing the image (including display-settings) and managing the data found in a CZI-file.

## Docs
https://zeiss.github.io/libczi/

## Related Software and Tooling
libCZI is already part of a larger ecosystem.

### OAD
The libCZI libary is part of the [Open Application Development Concept of Zeiss Microscopy](https://github.com/zeiss-microscopy/OAD).

### pylibCZIrw
[pylibCZIrw](https://pypi.org/project/pylibCZIrw/) is a python module for reading and writing CZI files by utilizing/wrapping libCZI.

## Contributing
[CONTRIBUTING.md](/CONTRIBUTING.md)

## Licensing
Carl Zeiss Microscopy GmbH provides libCZI under a dual-license model - [LGPL Version 3](https://www.gnu.org/licenses/lgpl-3.0.en.html) as well as Proprietary/Commercial. 

### LGPL Version 3
libCZI is a reader and writer for the CZI fileformat written in C++
Copyright (C) 2017 Carl Zeiss Microscopy GmbH

This program is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

See [COPYING](/COPYING) and [COPYING.LESSER](/COPYING.LESSER).

### Commercial/Proprietary
Contact github.microscopy@zeiss.com for a commercial/proprietary license in case you do not want to be subject to the [LGPL Version 3](#lgpl-version-3).  

Note: Purchasing a commercial/proprietary license does not dispense you from fulfilling all obligations that arise from [3rd Party Components](#credits-to-third-party-components) used/consumed by libCZI.

## Credits to Third Party Components
The authors and maintainers of libCZI give a big shout-out to all the [helpers](/THIRD_PARTY_LICENSES.txt) that have been part in bringing this library to where it is today.

## Disclaimer
ZEISS, ZEISS.com are registered trademarks of Carl Zeiss AG.
