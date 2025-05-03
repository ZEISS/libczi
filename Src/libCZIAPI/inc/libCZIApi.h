// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include "importexport.h"
#include "errorcodes.h"
#include "ObjectHandles.h"
#include "versioninfo_structs.h"
#include "inputstream_class_info_struct.h"
#include "external_input_stream_struct.h"
#include "external_output_stream_struct.h"
#include "reader_open_info_struct.h"
#include "subblock_statistics_struct.h"
#include "MetadataAsXml_struct.h"
#include "bitmap_structs.h"
#include "subblock_info_interop.h"
#include "attachment_info_interop.h"
#include "fileheader_info_interop.h"
#include "add_subblock_info_interop.h"
#include "add_attachment_info_interop.h"
#include "write_metadata_info_interop.h"
#include "accessor_options_interop.h"
#include "composition_channel_info_interop.h"
#include "scaling_info_interop.h"

#include <cstdint>

/// Release the memory - this function is to be used for freeing memory allocated by the libCZIApi-library
/// (and returned to the caller).
///
/// \param  data    Pointer to the memory to be freed.
EXTERNALLIBCZIAPI_API(void) libCZI_Free(void* data);

/// Allocate memory of the specified size.
///
/// \param          size    The size of the memory block to be allocated in bytes.
/// \param [out]    data    If successful, a pointer to the allocated memory is put here. The memory must be freed using 'libCZI_Free'.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_AllocateMemory(std::uint64_t size, void** data);

/// Get version information about the libCZIApi-library.
///
/// \param [out] version_info    If successful, the version information is put here.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_GetLibCZIVersionInfo(LibCZIVersionInfoInterop* version_info);

/// Get information about the build of the libCZIApi-library.
///
/// \param [out] build_info  If successful, the build information is put here. Note that all strings must be freed by the caller (using 'libCZI_Free').
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_GetLibCZIBuildInformation(LibCZIBuildInformationInterop* build_info);

// ****************************************************************************************************
// CZI-reader functions begin here

/// Create a new CZI-reader object.
///
/// \param [out] reader_object If the operation is successful, a handle to the newly created reader object is put here.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_CreateReader(CziReaderObjectHandle* reader_object);

/// Instruct the specified reader-object to open a CZI-document. The 'open_info' parameter contains
/// a handle to a stream-object which is used to read the document.
///
/// \param  reader_object A handle representing the reader-object.
/// \param  open_info     Parameters controlling the operation.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_ReaderOpen(CziReaderObjectHandle reader_object, const ReaderOpenInfoInterop* open_info);

/// Get information about the file-header of the CZI document. The information is put into the 'file_header_info_interop' structure.
/// This file_header_info_interop structure contains the GUID of the CZI document and the version levels of CZI.
///
/// \param          reader_object               The reader object.
/// \param [out]    file_header_info_interop    If successful, the retrieved information is put here.
///
/// \returns An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_ReaderGetFileHeaderInfo(CziReaderObjectHandle reader_object, FileHeaderInfoInterop* file_header_info_interop);

/// Reads the sub-block identified by the specified index. If there is no sub-block present (for the
/// specified index) then the function returns 'LibCZIApi_ErrorCode_OK', but the 'sub_block_object' 
/// is set to 'kInvalidObjectHandle'.
///
/// \param          reader_object       The reader object.
/// \param          index               Index of the sub-block.
/// \param [out]    sub_block_object    If successful, a handle to the sub-block object is put here; otherwise 'kInvalidObjectHandle'.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_ReaderReadSubBlock(CziReaderObjectHandle reader_object, std::int32_t index, SubBlockObjectHandle* sub_block_object);

/// Get statistics about the sub-blocks in the CZI-document. This function provides a simple version of the statistics, the
/// information retrieved does not include the per-scene statistics.
///
/// \param          reader_object   The reader object.
/// \param [out]    statistics      If non-null, the simple statistics will be put here.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_ReaderGetStatisticsSimple(CziReaderObjectHandle reader_object, SubBlockStatisticsInterop* statistics);

/// Get extended statistics about the sub-blocks in the CZI-document. This function provides a more detailed version of the statistics,
/// including the per-scene statistics. Note that the statistics is of variable size, and the semantic is as follows:
/// - On input, the argument 'number_of_per_channel_bounding_boxes' must point to an integer which describes the size of the argument 'statistics'.  
///   This number gives how many elements the array 'per_scenes_bounding_boxes' in 'SubBlockStatisticsInteropEx' can hold. Only that number of
///   per-scene information elements will be put into the 'statistics' structure at most, in any case.
/// - On output, the argument 'number_of_per_channel_bounding_boxes' will be set to the number of per-channel bounding boxes that were actually  
///   available.
/// - In the returned 'SubBlockStatisticsInteropEx' structure, the 'number_of_per_scenes_bounding_boxes' field will be set to the number of per-scene  
///   information that is put into this struct (which may be less than number of scenes that are available).
/// So, the caller is expected to check the returned 'number_of_per_channel_bounding_boxes' to see how many per-channel bounding boxes are available.
/// If this number is greater than the number of elements (given with the 'number_of_per_scenes_bounding_boxes' value in the 'statistics' structure),
/// then the caller should allocate a larger 'statistics' structure and call this function again (with an increased 'number_of_per_scenes_bounding_boxes').
///
/// \param          reader_object                           The reader object.
/// \param [out]    statistics                              If non-null, the statistics will be put here.
/// \param [in,out] number_of_per_channel_bounding_boxes    On input, it gives the number of elements that can be put into the 'per_scenes_bounding_boxes' array.
///                                                         On output, it gives the number of elements which are available.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_ReaderGetStatisticsEx(CziReaderObjectHandle reader_object, SubBlockStatisticsInteropEx* statistics, std::int32_t* number_of_per_channel_bounding_boxes);

/// Get "pyramid-statistics" about the CZI-document. This function provides a JSON-formatted string which contains information about the pyramid.
/// The JSON-schema is as follows:
/// \code
/// {
///     "scenePyramidStatistics": {
///         "<sceneIndex>": [
///         {
///             "layerInfo": {
///             "minificationFactor": <number>,
///             "pyramidLayerNo" : <number>
///         },
///         "count" : <number>
///         }
///     ]}
/// }
/// \endcode
/// It resembles the corresponding C++-structure 'PyramidStatistics' in the libCZI-library.
///
/// \param          reader_object              The reader object.
/// \param [out]    pyramid_statistics_as_json If successful, a pointer to a JSON-formatted string is placed here. The caller 
///                                             is responsible for freeing this memory (by calling libCZI_Free).
///
/// \returns An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_ReaderGetPyramidStatistics(CziReaderObjectHandle reader_object, char** pyramid_statistics_as_json);

/// Create a metadata-segment object from the reader-object. The metadata-segment object can be used to retrieve the XML-metadata of the CZI-document.
///
/// \param          reader_object           The reader object.
/// \param [out]    metadata_segment_object If successful, a handle to the metadata-segment object is put here.
///
/// \returns An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_ReaderGetMetadataSegment(CziReaderObjectHandle reader_object, MetadataSegmentObjectHandle* metadata_segment_object);

/// Get the number of attachments available.
///
/// \param          reader_object           The reader object.
/// \param [out]    count                   The number of available attachments is put here.
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_ReaderGetAttachmentCount(CziReaderObjectHandle reader_object, std::int32_t* count);

/// Get information about the attachment at the specified index. The information is put into the 'attachment_info_interop' structure.
/// If the index is not valid, then the function returns 'LibCZIApi_ErrorCode_IndexOutOfRange'.
///
/// \param          reader_object           The reader object.
/// \param          index                   The index of the attachment to query information for.
/// \param [out]    attachment_info_interop If successful, the retrieved information is put here.
///
/// \returns An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_ReaderGetAttachmentInfoFromDirectory(CziReaderObjectHandle reader_object, std::int32_t index, AttachmentInfoInterop* attachment_info_interop);

/// Read the attachment with the specified index and create an attachment object representing it. If the specified index
/// is invalid, then the returned attachment-object handle will have the value 'kInvalidObjectHandle'.
/// \param       reader_object              The reader object.
/// \param       index                      The index of the attachment to get.
/// \param [out] attachment_object          If successful and index is valid, a handle representing the attachment object is put here. If the index is
///                                         invalid, then the handle will have the value 'kInvalidObjectHandle'.
/// \returns  An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_ReaderReadAttachment(CziReaderObjectHandle reader_object, std::int32_t index, AttachmentObjectHandle* attachment_object);

/// Release the specified reader-object. After this function is called, the handle is no
/// longer valid.
///
/// \param  reader_object   The reader object.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_ReleaseReader(CziReaderObjectHandle reader_object);

// CZI-reader functions end here
// ****************************************************************************************************

// ****************************************************************************************************
// Stream functions begin here

/// Get the number of available stream classes.
///
/// \param [out] count The number of available stream classes it put here.
///
/// \returns An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_GetStreamClassesCount(std::int32_t* count);

/// Get information about the stream class at the specified index.
///
/// \param          index                   Zero-based index of the stream class to query information about.
/// \param [out]    input_stream_class_info If successful, information about the stream class is put here. Note that the strings in the structure
///                                         must be freed (by the caller) using 'libCZI_Free'.
///
/// \returns An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_GetStreamClassInfo(std::int32_t index, InputStreamClassInfoInterop* input_stream_class_info);

/// Create an input stream object of the specified type, using the specified JSON-formatted property bag and
/// the specified file identifier as input.
///
/// \param          stream_class_name       Name of the stream class to be instantiated.
/// \param          creation_property_bag   JSON formatted string (containing additional parameters for the stream creation) in UTF8-encoding.
/// \param          stream_identifier       The filename (or, more generally, a URI of some sort) identifying the file to be opened in UTF8-encoding.                                        
/// \param [out]    stream_object           If successful, a handle representing the newly created stream object is put here.
///
/// \returns    An error-code that indicates whether the operation is successful or not.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_CreateInputStream(const char* stream_class_name, const char* creation_property_bag, const char* stream_identifier, InputStreamObjectHandle* stream_object);

/// Create an input stream object for a file identified by its filename, which is given as a wide string. Note that wchar_t on
/// Windows is 16-bit wide, and on Unix-like systems it is 32-bit wide.
/// 
/// \param  [in]    filename        Filename of the file which is to be opened (zero terminated wide string). Note that on Windows, this 
///                                 is a string with 16-bit code units, and on Unix-like systems it is typically a string with 32-bit code units.
///                 
/// \param  [out]   stream_object   The output stream object that will hold the created stream.
/// \return         An error-code that indicates whether the operation is successful or not. Non-positive values indicates successful, positive values
///                 indicates unsuccessful operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_CreateInputStreamFromFileWide(const wchar_t* filename, InputStreamObjectHandle* stream_object);

/// Create an input stream object for a file identified by its filename, which is given as an UTF8-encoded string.
/// 
/// \param  [in]    filename        Filename of the file which is to be opened (in UTF8 encoding).
/// \param  [out]   stream_object   The output stream object that will hold the created stream.
/// \return         An error-code that indicates whether the operation is successful or not. Non-positive values indicates successful, positive values
///                 indicates unsuccessful operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_CreateInputStreamFromFileUTF8(const char* filename, InputStreamObjectHandle* stream_object);

/// Create an input stream object which is using externally provided functions for operation
/// and reading the data. Please refer to the documentation of
/// 'ExternalInputStreamStructInterop' for more information.
///
/// \param          external_input_stream_struct    Structure containing the information about the externally provided functions.
/// \param [out]    stream_object                   If successful, the handle to the newly created input stream object is put here.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_CreateInputStreamFromExternal(const ExternalInputStreamStructInterop* external_input_stream_struct, InputStreamObjectHandle* stream_object);

/// Release the specified input stream object. After this function is called, the handle is no
/// longer valid. Note that calling this function will only decrement the usage count of the
/// underlying object; whereas the object itself (and the resources it holds) will only be
/// released when the usage count reaches zero.
///
/// \param  stream_object   The input stream object.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_ReleaseInputStream(InputStreamObjectHandle stream_object);

// Stream functions end here
// ****************************************************************************************************

// ****************************************************************************************************
// sub-block functions begin here

/// Create a bitmap object from the specified sub-block object. The bitmap object can be used to access the pixel 
/// data contained in the sub-block. If the subblock contains compressed data, then decompression will be performed
/// in this call.
///
/// \param          sub_block_object The sub-block object.
/// \param [out]    bitmap_object    If successful, the handle to the newly created bitmap object is put here.
///
/// \returns An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_SubBlockCreateBitmap(SubBlockObjectHandle sub_block_object, BitmapObjectHandle* bitmap_object);

/// Get Information about the sub-block.
///
/// \param       sub_block_object The sub-block object.
/// \param [out] sub_block_info   If successful, information about the sub-block object is put here.
///
/// \returns An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_SubBlockGetInfo(SubBlockObjectHandle sub_block_object, SubBlockInfoInterop* sub_block_info);

/// Copy the raw data from the specified sub-block object to the specified memory buffer. The value of the 'size' parameter
/// on input is the size of the buffer pointed to by 'data'. On output, the value of 'size' is the actual size of the data. At most
/// the initial value of 'size' bytes are copied to the buffer. If the initial value of 'size' is zero (0) or 'data' is null, then 
/// no data is copied. 
/// For the 'type' parameter, the following values are valid: 0 (data) and 1 (metadata).
/// For 0 (data), the data is the raw pixel data of the bitmap. This data may be compressed.
/// For 1 (metadata), the data is the raw metadata in XML-format (UTF8-encoded).
///
/// \param          sub_block_object    The sub block object.
/// \param          type                The type - 0 for "pixel-data", 1 for "sub-block metadata".
/// \param [in,out] size                On input, the size of the memory block pointed to by 'data', on output the actual size of the available data.
/// \param [out]    data                Pointer where the data is to be copied to. At most the initial content of 'size' bytes are copied.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_SubBlockGetRawData(SubBlockObjectHandle sub_block_object, std::int32_t type, std::uint64_t* size, void* data);

/// Release the specified sub-block object.
///
/// \param  sub_block_object The sub block object to be released.
///
/// \returns An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_ReleaseSubBlock(SubBlockObjectHandle sub_block_object);

// sub-block functions end here
// ****************************************************************************************************

// ****************************************************************************************************
// attachment functions begin here

/// Get information about the specified attachment object.
/// \param attachment_object            The attachment object.
/// \param [out]    attachment_info     Information about the attachment.
/// \returns     An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_AttachmentGetInfo(AttachmentObjectHandle attachment_object, AttachmentInfoInterop* attachment_info);

/// Copy the raw data from the specified attachment object to the specified memory buffer. The value of the 'size' parameter
/// on input is the size of the buffer pointed to by 'data'. On output, the value of 'size' is the actual size of the data. At most
/// the initial value of 'size' bytes are copied to the buffer. If the initial value of 'size' is zero (0) or 'data' is null, then 
/// no data is copied. 
/// \param          attachment_object   The attachment object.
/// \param [in,out] size                On input, the size of the memory block pointed to by 'data', on output the actual size of the available data.
/// \param [out]    data                Pointer where the data is to be copied to. At most the initial content of 'size' bytes are copied.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_AttachmentGetRawData(AttachmentObjectHandle attachment_object, std::uint64_t* size, void* data);

/// Release the specified attachment object.
///
/// \param  attachment_object The attachment object to be released.
///
/// \returns An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_ReleaseAttachment(AttachmentObjectHandle attachment_object);

// attachment functions end here
// ****************************************************************************************************

// ****************************************************************************************************
// bitmap functions begin here

/// Get information about the specified bitmap object.
///
/// \param          bitmap_object The bitmap object.
/// \param [out]    bitmap_info   If successful, information about the bitmap object is put here.
///
/// \returns An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_BitmapGetInfo(BitmapObjectHandle bitmap_object, BitmapInfoInterop* bitmap_info);

/// Locks the bitmap object. Once the bitmap is locked, the pixel data can be accessed. Memory access to the
/// pixel data must only occur while the bitmap is locked. The lock must be released by calling 'libCZI_BitmapUnlock'.
/// It is a fatal error if the bitmap is destroyed while still being locked. Calls to Lock and Unlock are counted, and
/// they must be balanced.
///
/// \param          bitmap_object The bitmap object.
/// \param [out]    lockInfo      If successful, information about how to access the pixel data is put here.
///
/// \returns An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_BitmapLock(BitmapObjectHandle bitmap_object, BitmapLockInfoInterop* lockInfo);

/// Unlock the bitmap object. Once the bitmap is unlocked, the pixel data must not be accessed anymore.
///
/// \param  bitmap_object The bitmap object.
///
/// \returns An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_BitmapUnlock(BitmapObjectHandle bitmap_object);

/// Copy the pixel data from the specified bitmap object to the specified memory buffer. The specified
/// destination bitmap must have same width, height and pixel type as the source bitmap.
///
/// \param          bitmap_object The bitmap object.
/// \param          width         The width of the destination bitmap.
/// \param          height        The height of the destination bitmap.
/// \param          pixel_type    The pixel type.
/// \param          stride        The stride (given in bytes).
/// \param [out]    ptr           Pointer to the memory location where the bitmap is to be copied to.
///
/// \returns A LibCZIApiErrorCode.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_BitmapCopyTo(BitmapObjectHandle bitmap_object, std::uint32_t width, std::uint32_t height, std::int32_t pixel_type, std::uint32_t stride, void* ptr);

/// Release the specified bitmap object.
/// It is a fatal error trying to release a bitmap object that is still locked.
///
/// \param  bitmap_object The bitmap object.
///
/// \returns An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_ReleaseBitmap(BitmapObjectHandle bitmap_object);

// bitmap functions end here
// ****************************************************************************************************

// ****************************************************************************************************
// MetadataSegment functions begin here

/// Get the XML-metadata information from the specified metadata-segment object.
/// Note that the XML-metadata is returned as a pointer to the data (in the 'data' field of the 'MetadataAsXmlInterop' structure), which
/// must be freed by the caller using 'libCZI_Free'.
///
/// \param          metadata_segment_object The metadata segment object.
/// \param [out]    metadata_as_xml_interop If successful, the XML-metadata information is put here.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_MetadataSegmentGetMetadataAsXml(MetadataSegmentObjectHandle metadata_segment_object, MetadataAsXmlInterop* metadata_as_xml_interop);

/// Create a CZI-document-information object from the specified metadata-segment object.
///
/// \param          metadata_segment_object The metadata segment object.
/// \param [in,out] czi_document_info       If successful, a handle to the newly created CZI-document-info object is put here.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_MetadataSegmentGetCziDocumentInfo(MetadataSegmentObjectHandle metadata_segment_object, CziDocumentInfoHandle* czi_document_info);

/// Release the specified metadata-segment object.
///
/// \param  metadata_segment_object The metadata-segment object to be released.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_ReleaseMetadataSegment(MetadataSegmentObjectHandle metadata_segment_object);

// MetadataSegment functions end here
// ****************************************************************************************************

// ****************************************************************************************************
// CziDocumentInfo functions begin here

/// Get "general document information" from the specified czi-document information object. The information is returned as a JSON-formatted string.
/// The JSON returned is an object, with the following possible key-value pairs:
/// "name" : "<name of the document>", type string
/// "title" : "<title of the document>", type string
/// "user_name" : "<user name>", type string
/// "description" : "<description>", type string
/// "comment" : "<comment>", type string
/// "keywords" : "<keyword1>,<keyword2>,...", type string
/// "rating" : "<rating>", type integer
/// "creation_date" : "<creation date>", type string, conforming to ISO 8601
///
/// \param          czi_document_info           The CZI-document-info object.
/// \param [out]    general_document_info_json  If successful, the general document information is put here. Note that the data must be freed using 'libCZI_Free' by the caller.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_CziDocumentInfoGetGeneralDocumentInfo(CziDocumentInfoHandle czi_document_info, void** general_document_info_json);

/// Get scaling information from the specified czi-document information object. The information gives the size of an image pixels.
///
/// \param          czi_document_info           Handle to the CZI-document-info object from which the scaling information will be retrieved.
/// \param [out]    scaling_info_interop        If successful, the scaling information is put here.
///
/// \returns        An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_CziDocumentInfoGetScalingInfo(CziDocumentInfoHandle czi_document_info, ScalingInfoInterop* scaling_info_interop);

/// Retrieve the set of dimensions for which "dimension info" data is available. The argument 'available_dimensions_count' indicates the number of
/// elements available, and this should be 'kMaxDimensionCount+1' at least. If the number of available dimensions is insufficient, the function will
/// return an error (LibCZIApi_ErrorCode_InvalidArgument). The 'available_dimensions' array is filled with the available dimensions, and the list is
/// terminated with a value of 'kInvalidDimensionIndex'.
/// 
/// \param          czi_document_info           The CZI-document-info object.
/// \param          available_dimensions_count  Number of elements available in the 'available_dimensions' array.
/// \param [in,out] available_dimensions        If successful, the available dimensions are put here. The list is terminated with a value of 'kInvalidDimensionIndex'.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_CziDocumentInfoGetAvailableDimension(CziDocumentInfoHandle czi_document_info, std::uint32_t available_dimensions_count, std::uint32_t*  available_dimensions);

/// Get the display-settings from the document's XML-metadata. The display-settings are returned in the form of an object,
/// for which a handle is returned.
///
/// \param          czi_document_info       The CZI-document-info object.
/// \param [in,out] display_settings_handle If successful, a handle to the display-settings object is put here.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_CziDocumentInfoGetDisplaySettings(CziDocumentInfoHandle czi_document_info, DisplaySettingsHandle* display_settings_handle);

/// Get the dimension information from the document's XML-metadata. The information is returned as a JSON-formatted string.
///
/// \param          czi_document_info       Handle to the CZI-document-info object from which the dimension information will be retrieved.
/// \param          dimension_index         Index of the dimension.
/// \param [out]    dimension_info_json     If successful, the information is put here as JSON format. Note that the data must be freed using 'libCZI_Free' by the caller.
///
/// \returns        An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_CziDocumentInfoGetDimensionInfo(CziDocumentInfoHandle czi_document_info, std::uint32_t dimension_index, void** dimension_info_json);

/// Release the specified CZI-document-info object.
///
/// \param  czi_document_info The CZI-document-info object.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_ReleaseCziDocumentInfo(CziDocumentInfoHandle czi_document_info);

// CziDocumentInfo functions end here
// ****************************************************************************************************

// ****************************************************************************************************
// Outputstream functions begin here

/// Create an output stream object for a file identified by its filename, which is given as a wide string. Note that wchar_t on
/// Windows is 16-bit wide, and on Unix-like systems it is 32-bit wide.
/// 
/// \param          filename                Filename of the file which is to be opened (zero terminated wide string). Note that on Windows, this 
///                                         is a string with 16-bit code units, and on Unix-like systems it is typically a string with 32-bit code units.
/// \param          overwrite               Indicates whether the file should be overwritten.
/// \param [out]    output_stream_object    The output stream object that will hold the created stream.
/// 
/// \return         An error-code that indicates whether the operation is successful or not. Non-positive values indicates successful, positive values
///                 indicates unsuccessful operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_CreateOutputStreamForFileWide(const wchar_t* filename, bool overwrite, OutputStreamObjectHandle* output_stream_object);

/// Create an input stream object for a file identified by its filename, which is given as an UTF8 - encoded string.
/// 
/// \param          filename                Filename of the file which is to be opened (in UTF8 encoding).
/// \param          overwrite               Indicates whether the file should be overwritten.
/// \param [out]    output_stream_object    The output stream object that will hold the created stream.
/// 
/// \return         An error-code that indicates whether the operation is successful or not. Non-positive values indicates successful, positive values
///                 indicates unsuccessful operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_CreateOutputStreamForFileUTF8(const char* filename, bool overwrite, OutputStreamObjectHandle* output_stream_object);

/// Release the specified output stream object. After this function is called, the handle is no
/// longer valid. Note that calling this function will only decrement the usage count of the
/// underlying object; whereas the object itself (and the resources it holds) will only be
/// released when the usage count reaches zero.
///
/// \param  output_stream_object   The output stream object.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_ReleaseOutputStream(OutputStreamObjectHandle output_stream_object);

/// Create an output stream object which is using externally provided functions for operation
/// and writing the data. Please refer to the documentation of
/// 'ExternalOutputStreamStructInterop' for more information.
///
/// \param          external_output_stream_struct    Structure containing the information about the externally provided functions.
/// \param [out]    output_stream_object             If successful, the handle to the newly created output stream object is put here.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_CreateOutputStreamFromExternal(const ExternalOutputStreamStructInterop* external_output_stream_struct, OutputStreamObjectHandle* output_stream_object);

// Outputstream functions end here
// ****************************************************************************************************

// ****************************************************************************************************
// CziWriter functions begin here

/// Create a writer object for authoring a document in CZI-format. The options string is a JSON-formatted string, here
/// is an example:
/// \code
/// {
/// "allow_duplicate_subblocks" : true
/// }
/// \endcode
///
/// \param [out] writer_object If the operation is successful, a handle to the newly created writer object is put here.
/// \param       options       A JSON-formatted zero-terminated string (in UTF8-encoding) containing options for the writer creation.
///
/// \returns An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_CreateWriter(CziWriterObjectHandle* writer_object, const char* options);

/// Initializes the writer object with the specified output stream object. The options string is a JSON-formatted string, here
/// is an example:
/// \code
/// {
/// "file_guid" : "123e4567-e89b-12d3-a456-426614174000",
/// "reserved_size_attachments_directory" : 4096,
/// "reserved_size_metadata_segment" : 50000,
/// "minimum_m_index" : 0,
/// "maximum_m_index" : 100
/// }
/// \endcode
///
/// \param [out] writer_object If the operation is successful, a handle to the newly created writer object is put here.
/// \param       output_stream_object The output stream object to be used for writing the CZI data.
/// \param       parameters       A JSON-formatted zero-terminated string (in UTF8-encoding) containing options for the writer initialization.
///
/// \returns An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_WriterCreate(CziWriterObjectHandle writer_object, OutputStreamObjectHandle output_stream_object, const char* parameters);

/// Add the specified sub-block to the writer object. The sub-block information is provided in the 'add_sub_block_info_interop' structure.
///
/// \param  writer_object               The writer object.
/// \param  add_sub_block_info_interop  Information describing the sub-block to be added.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_WriterAddSubBlock(CziWriterObjectHandle writer_object, const AddSubBlockInfoInterop* add_sub_block_info_interop);

/// Add the specified attachment to the writer object. The attachment is provided in the 'add_attachment_info_interop' structure.
///
/// \param  writer_object               The writer object.
/// \param  add_attachment_info_interop Information describing the attachment to be added.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_WriterAddAttachment(CziWriterObjectHandle writer_object, const AddAttachmentInfoInterop* add_attachment_info_interop);

// TODO(JBL): EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_WriterGetPreparedMetadata(CziWriterObjectHandle writer_object, CziMetadataBuilderHandle* metadata_builder, const PrepareMetadataInfoInterop* prepare_metadata_info_interop);

/// Add the specified metadata to the writer object. The metadata is provided in the 'write_metadata_info_interop' structure.
///
/// \param  writer_object               Handle to the writer object to which the metadata will be added.
/// \param  write_metadata_info_interop Information describing the metadata to be added.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_WriterWriteMetadata(CziWriterObjectHandle writer_object, const WriteMetadataInfoInterop* write_metadata_info_interop);

/// Finalizes the CZI (i.e. writes out the final directory-segments) and closes the file.
/// Note that this method must be called explicitly in order to get a valid CZI - calling 'libCZI_ReleaseWriter' without
/// a prior call to this method will close the file immediately without finalization.
///
/// \param  writer_object   Handle to the writer object that is to be closed.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_WriterClose(CziWriterObjectHandle writer_object);

/// Release the specified writer object.
///
/// \param  writer_object Handle to the writer object that is to be released.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_ReleaseWriter(CziWriterObjectHandle writer_object);

// CziWriter functions end here
// ****************************************************************************************************

// ****************************************************************************************************
// SingleChannelScalingTileAccessor functions begin here

/// Create a single channel scaling tile accessor. 
/// 
/// \param reader_object            A handle representing the reader-object.
/// \param accessor_object [out]    If the operation is successful, a handle to the newly created single-channel-scaling-tile-accessor is put here.
/// 
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_CreateSingleChannelTileAccessor(CziReaderObjectHandle reader_object, SingleChannelScalingTileAccessorObjectHandle* accessor_object);

/// Gets the size information of the specified tile accessor based on the region of interest and zoom factor.
/// 
/// \param  accessor_object     Handle to the tile accessor object for which the size is to be calculated. This object is responsible for managing the access to the tiles within the specified plane.
/// \param  roi                 The region of interest that defines the region of interest within the plane for which the size is to be calculated.
/// \param  zoom                A floating-point value representing the zoom factor.
/// \param  size [out]          The size of the tile accessor. It contains width and height information.            
/// 
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_SingleChannelTileAccessorCalcSize(SingleChannelScalingTileAccessorObjectHandle accessor_object, const IntRectInterop* roi, float zoom, IntSizeInterop* size);

/// Gets the tile bitmap of the specified plane and the specified roi with the specified zoom factor. 
/// 
/// \param  accessor_object         Handle to the tile accessor object. This object is responsible for managing the access to the tiles within the specified plane.
/// \param  coordinate              Pointer to a `CoordinateInterop` structure that specifies the coordinates within the plane from which the tile bitmap is to be retrieved.
/// \param  roi                     The region of interest that defines within the plane for which the tile bitmap is requested.
/// \param  zoom                    A floating-point value representing the zoom factor.
/// \param  options                 A pointer to an AccessorOptionsInterop structure that may contain additional options for accessing the tile bitmap.
/// \param  bitmap_object [out]     If the operation is successful, the created bitmap object will be put here.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_SingleChannelTileAccessorGet(SingleChannelScalingTileAccessorObjectHandle accessor_object, const CoordinateInterop* coordinate, const IntRectInterop* roi, float zoom, const AccessorOptionsInterop* options, BitmapObjectHandle* bitmap_object);

/// Release the specified accessor object.
///
/// \param  accessor_object      The accessor object.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_ReleaseCreateSingleChannelTileAccessor(SingleChannelScalingTileAccessorObjectHandle accessor_object);

// SingleChannelScalingTileAccessor  functions end here
// ****************************************************************************************************

// ****************************************************************************************************
// Compositor functions begin here

/// Given a display-settings object and the channel-number, this function fills out the 
/// composition-channel-information which is needed for the multi-channel-composition.
/// Note that in the returned 'CompositionChannelInfoInterop' structure, the 'lut' field is a pointer to the LUT-data, 
/// which must be freed with 'libCZI_Free' by the caller.
///
/// \param          display_settings_handle             The display settings handle.
/// \param          channel_index                       The channel-index (referring to the display settings object) we are concerned with.
/// \param          sixteen_or_eight_bits_lut           True for generating a 16-bit LUT; if false, then an 8-bit LUT is generated.
/// \param [out]    composition_channel_info_interop    The composition channel information is put here.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_CompositorFillOutCompositionChannelInfoInterop(
                                                                    DisplaySettingsHandle display_settings_handle, 
                                                                    int channel_index,
                                                                    bool sixteen_or_eight_bits_lut,
                                                                    CompositionChannelInfoInterop* composition_channel_info_interop);

/// Perform a multi-channel-composition operation. The source bitmaps are provided in the 'source_bitmaps' array, and the
/// array of 'CompositionChannelInfoInterop' structures provide the information needed for the composition. The resulting bitmap
/// is then put into the 'bitmap_object' handle.
///
/// \param       channelCount       The number of channels - this defines the size of the 'source_bitmaps' and 'channel_info' arrays.
/// \param       source_bitmaps     The array of source bitmaps.
/// \param       channel_info       The array of channel information.
/// \param [out] bitmap_object      The resulting bitmap is put here.
///
/// \return     An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_CompositorDoMultiChannelComposition(
                                                                                    std::int32_t channelCount,
                                                                                    const BitmapObjectHandle* source_bitmaps,
                                                                                    const CompositionChannelInfoInterop* channel_info,
                                                                                    BitmapObjectHandle* bitmap_object);

// Compositor functions end here
// ****************************************************************************************************

// ****************************************************************************************************
// DisplaySettings functions begin here

EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_DisplaySettingsGetChannelDisplaySettings(DisplaySettingsHandle display_settings_handle, int channel_id, ChannelDisplaySettingsHandle* channel_display_setting);

/// Release the specified display settings object.
///
/// \param  display_settings_handle      The display settings object.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_ReleaseDisplaySettings(DisplaySettingsHandle display_settings_handle);

// DisplaySettings functions end here
// ****************************************************************************************************

// ****************************************************************************************************
// ChannelDisplaySettings functions begin here

/// Release the specified channel-display settings object.
///
/// \param  channel_display_settings_handle      The channel-display settings object.
///
/// \returns    An error-code indicating success or failure of the operation.
EXTERNALLIBCZIAPI_API(LibCZIApiErrorCode) libCZI_ReleaseChannelDisplaySettings(ChannelDisplaySettingsHandle channel_display_settings_handle);

// ChannelDisplaySettings functions end here
// // ****************************************************************************************************
