// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <rapidjson/document.h>
#include <libCZI.h>
#include "../inc/misc_types.h"
#include "../inc/accessor_options_interop.h"
#include "../inc/composition_channel_info_interop.h"

#include <map>
#include <string>

/// Here we gather some utilities, roughly centering around parameter conversion as used in the libCZIApi-implementation.
class ParameterHelpers
{
public:
    static void* AllocateMemory(size_t size);
    static void FreeMemory(void* ptr);
    static char* AllocString(const std::string& text);

    static bool TryParseInputStreamCreationPropertyBag(const std::string& s, std::map<int, libCZI::StreamsFactory::Property>* property_bag);

    static rapidjson::Document ConvertLibCZIPyramidStatisticsToJson(const libCZI::PyramidStatistics& pyramid_statistics);
    static std::string ConvertLibCZIPyramidStatisticsToJsonString(const libCZI::PyramidStatistics& pyramid_statistics);

    /// Copies a UTF-8 encoded string into a provided buffer, safely truncating if necessary.
    /// This function ensures that the UTF-8 string copied into the destination buffer is always
    /// properly zero-terminated and never truncates a multibyte character. If the input string
    /// exceeds the provided buffer size, the function truncates it at the nearest valid UTF-8
    /// character boundary. The destination buffer will always be null-terminated if
    /// size_destination is greater than 0.
    ///
    /// \param      input               The input UTF-8 encoded string to copy.
    /// \param [ut] destination         Pointer to the destination buffer.
    /// \param      size_destination    Size of the destination buffer in bytes.
    ///
    /// \returns    True if truncation occurred, false otherwise.
    static bool CopyUtf8StringTruncate(const std::string& input, char* destination, size_t size_destination);

    static bool TryParseCZIWriterOptions(const char* json_text,libCZI::CZIWriterOptions& czi_writer_options);
    static bool TryParseCZIWriterInfo(const char* json_text, std::shared_ptr<libCZI::ICziWriterInfo>& czi_writer_info);

    /// Attempts to parse a GUID from the given string. The string has to have the form 
    /// "cfc4a2fe-f968-4ef8-b685-e73d1b77271a" or "{cfc4a2fe-f968-4ef8-b685-e73d1b77271a}".
    ///
    /// \param          str      The string.
    /// \param [in,out] out_guid If non-null, the Guid will be put here if successful.
    ///
    /// \return True if it succeeds, false if it fails.
    static bool TryParseGuid(const std::string& str, libCZI::GUID* out_guid);

    /// Trims whitespaces from the beginning and end of the specified string.
    ///
    /// \param  str         The string to be trimmed.
    /// \param  whitespace  (Optional) The characters in the string define what is regarded as whitespace.
    ///
    /// \returns    The trimmed string.
    static std::string trim(const std::string& str, const std::string& whitespace = " \t");

    /// Convert coordinate from interop-coordinate-representation to libCZI-representation.
    ///
    /// \param  coordinate  The coordinate to be converted.
    ///
    /// \returns    The converted coordinate.
    static libCZI::CDimCoordinate ConvertCoordinateInteropToDimCoordinate(const CoordinateInterop& coordinate);

    static CoordinateInterop ConvertIDimCoordinateToCoordinateInterop(const libCZI::IDimCoordinate* coordinate);

    static libCZI::ISingleChannelScalingTileAccessor::Options ConvertSingleChannelScalingTileAccessorOptionsInteropToLibCZI(const AccessorOptionsInterop* options);

    static libCZI::Compositors::ChannelInfo ConvertCompositionChannelInfoInteropToChannelInfo(const CompositionChannelInfoInterop* composition_channel_info_interop);

    static rapidjson::Document FormatGeneralDocumentInfoAsJson(const libCZI::GeneralDocumentInfo& general_document_info);
    static std::string FormatGeneralDocumentInfoAsJsonString(const libCZI::GeneralDocumentInfo& general_document_info);

    static rapidjson::Document FormatZDimensionInfoAsJson(const libCZI::IDimensionZInfo* z_dimension_info);
    static std::string FormatZDimensionInfoAsJsonString(const libCZI::IDimensionZInfo* z_dimension_info);

    static rapidjson::Document FormatTDimensionInfoAsJson(const libCZI::IDimensionTInfo* t_dimension_info);
    static std::string FormatTDimensionInfoAsJsonString(const libCZI::IDimensionTInfo* t_dimension_info);

    static rapidjson::Document FormatCDimensionInfoAsJson(const libCZI::IDimensionsChannelsInfo* channel_dimension_info);
    static std::string FormatCDimensionInfoAsJsonString(const libCZI::IDimensionsChannelsInfo* channel_dimension_info);
    static rapidjson::Value FormatIDimensionChannelInfoAsJson(const libCZI::IDimensionChannelInfo* channel_info, rapidjson::Document::AllocatorType& allocator);
    static rapidjson::Value FormatSpectrumCharacteristicsAsJson(const libCZI::SpectrumCharacteristics& spectrum_characteristics, rapidjson::Document::AllocatorType& allocator);

    static void FillOutCompositionChannelInfoFromDisplaySettings(const libCZI::IDisplaySettings* display_settings, int channel_index, bool sixteen_or_eight_bits_lut, CompositionChannelInfoInterop& composition_channel_info);
};
