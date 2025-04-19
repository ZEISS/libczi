// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

/// Defines an alias representing a "handle to some object (created and used by the libCZIApi)".
typedef std::intptr_t ObjectHandle;

/// (Immutable) Reserved value indicating an invalid object handle.
const ObjectHandle kInvalidObjectHandle = 0;

/// Defines an alias representing the handle of a CZI-reader object.
typedef ObjectHandle CziReaderObjectHandle;

/// Defines an alias representing the handle of a sub-block object.
typedef ObjectHandle SubBlockObjectHandle;

/// Defines an alias representing the handle of an input stream object.
typedef ObjectHandle InputStreamObjectHandle;

/// Defines an alias representing the handle of an output stream object.
typedef ObjectHandle OutputStreamObjectHandle;

/// Defines an alias representing the handle of a memory allocation object - which is "a pointer to a memory block", which must be
/// freed with 'libCZI_Free'.
/// TODO(JBL): this is not really used so far, should be removed I guess.
typedef ObjectHandle MemoryAllocationObjectHandle;

/// Defines an alias representing the handle of a bitmap object.
typedef ObjectHandle BitmapObjectHandle;

/// Defines an alias representing the handle of a metadata segment object.
typedef ObjectHandle MetadataSegmentObjectHandle;

/// Defines an alias representing the handle of an attachment object.
typedef ObjectHandle AttachmentObjectHandle;

/// Defines an alias representing the handle of a writer object.
typedef ObjectHandle CziWriterObjectHandle;

/// Defines an alias representing the handle of a single-channel-scaling-tile-accessor.
typedef ObjectHandle SingleChannelScalingTileAccessorObjectHandle;

/// Defines an alias representing the handle of a "document info" object.
typedef ObjectHandle CziDocumentInfoHandle;

/// Defines an alias representing the handle of a "display settings" object.
typedef ObjectHandle DisplaySettingsHandle;

/// Defines an alias representing the handle of a "channel display settings" object.
typedef ObjectHandle ChannelDisplaySettingsHandle;
