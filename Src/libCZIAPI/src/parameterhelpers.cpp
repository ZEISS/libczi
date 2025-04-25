// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#include "parameterhelpers.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <regex>
#include <iomanip>

using namespace std;

namespace
{
    std::uint8_t HexCharToInt(char c)
    {
        switch (c)
        {
        case '0':return 0;
        case '1':return 1;
        case '2':return 2;
        case '3':return 3;
        case '4':return 4;
        case '5':return 5;
        case '6':return 6;
        case '7':return 7;
        case '8':return 8;
        case '9':return 9;
        case 'A':case 'a':return 10;
        case 'B':case 'b':return 11;
        case 'C':case 'c':return 12;
        case 'D':case 'd':return 13;
        case 'E':case 'e':return 14;
        case 'F':case 'f':return 15;
        }

        return 0xff;
    }

    bool ConvertHexStringToInteger(const char* cp, std::uint32_t* value)
    {
        if (*cp == '\0')
        {
            return false;
        }

        std::uint32_t v = 0;
        int cntOfSignificantDigits = 0;
        for (; *cp != '\0'; ++cp)
        {
            std::uint8_t x = HexCharToInt(*cp);
            if (x == 0xff)
            {
                return false;
            }

            if (v > 0)
            {
                if (++cntOfSignificantDigits > 7)
                {
                    return false;
                }
            }

            v = v * 16 + x;
        }

        if (value != nullptr)
        {
            *value = v;
        }

        return true;
    }

    string ConvertToJsonString(const std::function< rapidjson::Document()>& f)
    {
        rapidjson::Document document = f();
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        document.Accept(writer);
        return buffer.GetString();
    }

    // Helper function to convert Rgb8Color to #RRGGBB string
    std::string ConvertColorToHexString(const libCZI::Rgb8Color& color)
    {
        std::ostringstream oss;
        oss << "#" << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(color.r)
            << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(color.g)
            << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(color.b);
        return oss.str();
    }
}

//////////////////////////////////////////////////////////////////////////

/*static*/void* ParameterHelpers::AllocateMemory(size_t size)
{
    return malloc(size);
}

/*static*/void ParameterHelpers::FreeMemory(void* ptr)
{
    free(ptr);
}

/*static*/char* ParameterHelpers::AllocString(const string& text)
{
    const size_t size = text.size();
    if (size == 0)
    {
        return nullptr;
    }

    char* result = static_cast<char*>(AllocateMemory(size + 1));
    memcpy(result, text.c_str(), size);
    result[size] = '\0';
    return result;
}

/*static*/bool ParameterHelpers::TryParseInputStreamCreationPropertyBag(const std::string& s, std::map<int, libCZI::StreamsFactory::Property>* property_bag)
{
    // Here we parse the JSON-formatted string that contains the property bag for the input stream and
    //  construct a map<int, libCZI::StreamsFactory::Property> from it.

    int property_info_count;
    const libCZI::StreamsFactory::StreamPropertyBagPropertyInfo* property_infos = libCZI::StreamsFactory::GetStreamPropertyBagPropertyInfo(&property_info_count);

    rapidjson::Document document;
    document.Parse(s.c_str());
    if (document.HasParseError() || !document.IsObject())
    {
        return false;
    }

    for (rapidjson::Value::ConstMemberIterator itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr)
    {
        if (!itr->name.IsString())
        {
            return false;
        }

        string name = itr->name.GetString();
        size_t index_of_key = numeric_limits<size_t>::max();
        for (size_t i = 0; i < static_cast<size_t>(property_info_count); ++i)
        {
            if (name == property_infos[i].property_name)
            {
                index_of_key = i;
                break;
            }
        }

        if (index_of_key == numeric_limits<size_t>::max())
        {
            return false;
        }

        switch (property_infos[index_of_key].property_type)
        {
        case libCZI::StreamsFactory::Property::Type::String:
            if (!itr->value.IsString())
            {
                return false;
            }

            if (property_bag != nullptr)
            {
                property_bag->insert(std::make_pair(property_infos[index_of_key].property_id, libCZI::StreamsFactory::Property(itr->value.GetString())));
            }

            break;
        case libCZI::StreamsFactory::Property::Type::Boolean:
            if (!itr->value.IsBool())
            {
                return false;
            }

            if (property_bag != nullptr)
            {
                property_bag->insert(std::make_pair(property_infos[index_of_key].property_id, libCZI::StreamsFactory::Property(itr->value.GetBool())));
            }

            break;
        case libCZI::StreamsFactory::Property::Type::Int32:
            if (!itr->value.IsInt())
            {
                return false;
            }

            if (property_bag != nullptr)
            {
                property_bag->insert(std::make_pair(property_infos[index_of_key].property_id, libCZI::StreamsFactory::Property(itr->value.GetInt())));
            }

            break;
        default:
            // this actually indicates an internal error - the table property_infos contains a not yet implemented property type
            return false;
        }
    }

    return true;
}

/*static*/rapidjson::Document ParameterHelpers::ConvertLibCZIPyramidStatisticsToJson(const libCZI::PyramidStatistics& pyramid_statistics)
{
    // Create the root JSON document
    rapidjson::Document document;
    document.SetObject();

    // Create an allocator for memory management
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

    // Create the "scenePyramidStatistics" object
    rapidjson::Value scene_pyramid_statistics(rapidjson::kObjectType);

    for (const auto& scene_entry : pyramid_statistics.scenePyramidStatistics)
    {
        // Scene index (key)
        int scene_index = scene_entry.first;

        // List of PyramidLayerStatistics (value)
        const std::vector<libCZI::PyramidStatistics::PyramidLayerStatistics>& layer_stats = scene_entry.second;

        // Create an array to hold PyramidLayerStatistics for this scene
        rapidjson::Value layer_array(rapidjson::kArrayType);

        for (const auto& layer_stat : layer_stats)
        {
            // Create an object for each PyramidLayerStatistics entry
            rapidjson::Value layer_stat_object(rapidjson::kObjectType);

            // Add "layerInfo" object
            rapidjson::Value layer_info_object(rapidjson::kObjectType);
            layer_info_object.AddMember("minificationFactor", layer_stat.layerInfo.minificationFactor, allocator);
            layer_info_object.AddMember("pyramidLayerNo", layer_stat.layerInfo.pyramidLayerNo, allocator);
            layer_stat_object.AddMember("layerInfo", layer_info_object, allocator);

            // Add "count"
            layer_stat_object.AddMember("count", layer_stat.count, allocator);

            // Add this layer_stat_object to the array
            layer_array.PushBack(layer_stat_object, allocator);
        }

        // Add this scene's array to the scene_pyramid_statistics object
        scene_pyramid_statistics.AddMember(
            rapidjson::Value(std::to_string(scene_index).c_str(), allocator).Move(),
            layer_array, allocator);
    }

    // Add "scenePyramidStatistics" to the root document
    document.AddMember("scenePyramidStatistics", scene_pyramid_statistics, allocator);

    return document;
}

/*static*/std::string ParameterHelpers::ConvertLibCZIPyramidStatisticsToJsonString(const libCZI::PyramidStatistics& pyramid_statistics)
{
    /*
    JSON Schema:
    ============

    {
        "scenePyramidStatistics": {
            "<sceneIndex>": [
                {
                    "layerInfo": {
                        "minificationFactor": <number>,
                        "pyramidLayerNo": <number>
                },
                "count": <number>
            }]
        }
    }

    Example JSON output:
    ====================

    {
        "scenePyramidStatistics": {
            "0": [
                {
                    "layerInfo": {
                        "minificationFactor": 2,
                        "pyramidLayerNo": 0
                    },
                    "count": 50
                },
                {
                    "layerInfo": {
                        "minificationFactor": 2,
                        "pyramidLayerNo": 1
                    },
                    "count": 30
                }],
            "1": [
                {
                    "layerInfo": {
                        "minificationFactor": 3,
                        "pyramidLayerNo": 0
                    },
                    "count": 10
                }]
        }
    }
    */

    rapidjson::Document document = ConvertLibCZIPyramidStatisticsToJson(pyramid_statistics);

    // Convert the document to a JSON string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    return buffer.GetString();
}

/*static*/bool ParameterHelpers::CopyUtf8StringTruncate(const std::string& input, char* destination, size_t size_destination)
{
    if (size_destination == 0)
    {
        return !input.empty();
    }

    if (size_destination == 1)
    {
        destination[0] = '\0';
        return !input.empty();
    }

    size_t copyLength = (input.size() < size_destination - 1) ? input.size() : size_destination - 1;
    bool truncated = copyLength < input.size();

    // UTF-8 continuation bytes always start with bits 10xxxxxx
    while (copyLength > 0 && (static_cast<unsigned char>(input[copyLength]) & 0xC0) == 0x80)
    {
        --copyLength;
        truncated = true;
    }

    std::memcpy(destination, input.data(), copyLength);
    destination[copyLength] = '\0';

    return truncated;
}

/*static*/bool ParameterHelpers::TryParseCZIWriterOptions(const char* json_text, libCZI::CZIWriterOptions& czi_writer_options)
{
    if (json_text == nullptr)
    {
        return false;
    }

    rapidjson::Document document;
    document.Parse(json_text);
    if (document.HasParseError() || !document.IsObject())
    {
        return false;
    }

    bool information_successfully_parsed = false;

    static const char* kKeyAllowDuplicateSubblocks = "allow_duplicate_subblocks";

    // Check for 'allow_duplicate_subblocks' bool member
    if (document.HasMember(kKeyAllowDuplicateSubblocks) && document[kKeyAllowDuplicateSubblocks].IsBool())
    {
        czi_writer_options.allow_duplicate_subblocks = document[kKeyAllowDuplicateSubblocks].GetBool();
        information_successfully_parsed = true;
    }

    return information_successfully_parsed;
}

/*static*/bool ParameterHelpers::TryParseCZIWriterInfo(const char* json_text, std::shared_ptr<libCZI::ICziWriterInfo>& czi_writer_info)
{
    if (json_text == nullptr)
    {
        return false;
    }

    rapidjson::Document document;
    document.Parse(json_text);
    if (document.HasParseError() || !document.IsObject())
    {
        return false;
    }

    static const char* kKeyCziFileGuid = "file_guid";
    static const char* kKeyReservedSizeAttachmentsDirectory = "reserved_size_attachments_directory";
    static const char* kKeyReservedSizeSubBlockDirectory = "reserved_size_subblock_directory";
    static const char* kKeyReservedSizeMetadataSegment = "reserved_size_metadata_segment";
    static const char* kKeyMinimumMIndex = "minimum_m_index";
    static const char* kKeyMaximumMIndex = "maximum_m_index";

    bool information_successfully_parsed = false;

    auto czi_writer_info_parsed = std::make_shared<libCZI::CCziWriterInfo>();

    if (document.HasMember(kKeyCziFileGuid) && document[kKeyCziFileGuid].IsString())
    {
        std::string file_guid_string = document[kKeyCziFileGuid].GetString();
        libCZI::GUID file_guid;
        if (ParameterHelpers::TryParseGuid(file_guid_string.c_str(), &file_guid))
        {
            czi_writer_info_parsed->SetFileGuid(file_guid);
            information_successfully_parsed = true;
        }
    }

    if (document.HasMember(kKeyReservedSizeAttachmentsDirectory) && document[kKeyReservedSizeAttachmentsDirectory].IsUint())
    {
        czi_writer_info_parsed->SetReservedSizeForAttachmentsDirectory(true, document[kKeyReservedSizeAttachmentsDirectory].GetUint());
        information_successfully_parsed = true;
    }

    if (document.HasMember(kKeyReservedSizeSubBlockDirectory) && document[kKeyReservedSizeSubBlockDirectory].IsUint())
    {
        czi_writer_info_parsed->SetReservedSizeForSubBlockDirectory(true, document[kKeyReservedSizeSubBlockDirectory].GetUint());
        information_successfully_parsed = true;
    }

    if (document.HasMember(kKeyReservedSizeMetadataSegment) && document[kKeyReservedSizeMetadataSegment].IsUint())
    {
        czi_writer_info_parsed->SetReservedSizeForMetadataSegment(true, document[kKeyReservedSizeMetadataSegment].GetUint());
        information_successfully_parsed = true;
    }

    int min_m_index = -1, max_m_index = -1;
    bool min_m_index_valid = false, max_m_index_valid = false;
    if (document.HasMember(kKeyMinimumMIndex) && document[kKeyMinimumMIndex].IsInt())
    {
        min_m_index = document[kKeyMinimumMIndex].GetInt();
        min_m_index_valid = true;
    }

    if (document.HasMember(kKeyMaximumMIndex) && document[kKeyMaximumMIndex].IsInt())
    {
        max_m_index = document[kKeyMaximumMIndex].GetInt();
        max_m_index_valid = true;
    }

    if (min_m_index_valid && max_m_index_valid)
    {
        czi_writer_info_parsed->SetMIndexBounds(min_m_index, max_m_index);
        information_successfully_parsed = true;
    }

    if (information_successfully_parsed)
    {
        czi_writer_info = czi_writer_info_parsed;
    }

    return information_successfully_parsed;
}


bool ParameterHelpers::TryParseGuid(const std::string& str, libCZI::GUID* out_guid)
{
    auto strTrimmed = trim(str);
    if (strTrimmed.empty() || strTrimmed.length() < 2)
    {
        return false;
    }

    if (strTrimmed[0] == '{' && strTrimmed[strTrimmed.length() - 1] == '}')
    {
        strTrimmed = strTrimmed.substr(1, strTrimmed.length() - 2);
    }

    std::regex guidRegex(R"([0-9A-Fa-f]{8}[-]([0-9A-Fa-f]{4}[-]){3}[0-9A-Fa-f]{12})");
    if (std::regex_match(strTrimmed, guidRegex))
    {
        libCZI::GUID g;
        uint32_t value;
        char sz[9];
        for (int i = 0; i < 8; ++i)
        {
            sz[i] = strTrimmed[i];
        }

        sz[8] = '\0';
        bool b = ConvertHexStringToInteger(sz, &value);
        if (!b) { return false; }
        g.Data1 = value;

        for (int i = 0; i < 4; ++i)
        {
            sz[i] = strTrimmed[i + 9];
        }

        sz[4] = '\0';
        b = ConvertHexStringToInteger(sz, &value);
        if (!b)
        {
            return false;
        }

        g.Data2 = static_cast<unsigned short>(value);

        for (int i = 0; i < 4; ++i)
        {
            sz[i] = strTrimmed[i + 14];
        }

        b = ConvertHexStringToInteger(sz, &value);
        if (!b)
        {
            return false;
        }

        g.Data3 = static_cast<unsigned short>(value);

        sz[2] = '\0';
        static const uint8_t positions[] = { 19,21,24,26,  28,30,32,34 };

        for (int p = 0; p < 8; ++p)
        {
            for (int i = 0; i < 2; ++i)
            {
                sz[i] = strTrimmed[i + positions[p]];
            }

            b = ConvertHexStringToInteger(sz, &value);
            if (!b)
            {
                return false;
            }

            g.Data4[p] = static_cast<unsigned char>(value);
        }

        if (out_guid != nullptr)
        {
            *out_guid = g;
        }

        return true;
    }

    return false;
}

std::string ParameterHelpers::trim(const std::string& str, const std::string& whitespace)
{
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::wstring::npos)
    {
        return ""; // no content
    }

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

/*static*/libCZI::CDimCoordinate ParameterHelpers::ConvertCoordinateInteropToDimCoordinate(const CoordinateInterop& coordinate)
{
    libCZI::CDimCoordinate result;
    int value_index = 0;
    for (int i = 0; i < kMaxDimensionCount; ++i)
    {
        if ((coordinate.dimensions_valid & (1 << i)) == 0)
        {
            continue;
        }

        result.Set(static_cast<libCZI::DimensionIndex>(1 + i), coordinate.value[value_index]);
        ++value_index;
    }

    return result;
}

/*static*/CoordinateInterop ParameterHelpers::ConvertIDimCoordinateToCoordinateInterop(const libCZI::IDimCoordinate* coordinate)
{
    CoordinateInterop result = {};
    int result_index = 0;
    for (int i = static_cast<int>(libCZI::DimensionIndex::MinDim); i <= static_cast<int>(libCZI::DimensionIndex::MaxDim); ++i)
    {
        int value;
        if (coordinate->TryGetPosition(static_cast<libCZI::DimensionIndex>(i), &value))
        {
            int index = i - static_cast<int>(libCZI::DimensionIndex::MinDim);
            result.dimensions_valid |= (1 << index);
            result.value[result_index] = value;
            ++result_index;
        }
    }

    return result;
}

/*static*/libCZI::ISingleChannelScalingTileAccessor::Options ParameterHelpers::ConvertSingleChannelScalingTileAccessorOptionsInteropToLibCZI(const AccessorOptionsInterop* options)
{
    libCZI::ISingleChannelScalingTileAccessor::Options single_channel_tile_accessor_options;
    single_channel_tile_accessor_options.Clear();

    if (options == nullptr)
    {
        return single_channel_tile_accessor_options;
    }

    single_channel_tile_accessor_options.backGroundColor.r = options->back_ground_color_r;
    single_channel_tile_accessor_options.backGroundColor.g = options->back_ground_color_g;
    single_channel_tile_accessor_options.backGroundColor.b = options->back_ground_color_b;
    single_channel_tile_accessor_options.sortByM = options->sort_by_m;
    single_channel_tile_accessor_options.useVisibilityCheckOptimization = options->use_visibility_check_optimization;

    return single_channel_tile_accessor_options;
}

/*static*/libCZI::Compositors::ChannelInfo ParameterHelpers::ConvertCompositionChannelInfoInteropToChannelInfo(const CompositionChannelInfoInterop* composition_channel_info_interop)
{
    libCZI::Compositors::ChannelInfo channel_info;
    channel_info.Clear();
    channel_info.weight = composition_channel_info_interop->weight;
    channel_info.enableTinting = composition_channel_info_interop->enable_tinting;
    channel_info.tinting.color.r = composition_channel_info_interop->tinting_color_r;
    channel_info.tinting.color.g = composition_channel_info_interop->tinting_color_g;
    channel_info.tinting.color.b = composition_channel_info_interop->tinting_color_b;
    channel_info.blackPoint = composition_channel_info_interop->black_point;
    channel_info.whitePoint = composition_channel_info_interop->white_point;
    channel_info.lookUpTableElementCount = composition_channel_info_interop->look_up_table_element_count;
    channel_info.ptrLookUpTable = composition_channel_info_interop->ptr_look_up_table;
    return channel_info;
}

/*static*/rapidjson::Document ParameterHelpers::FormatGeneralDocumentInfoAsJson(const libCZI::GeneralDocumentInfo& general_document_info)
{
    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

    static const struct
    {
        bool libCZI::GeneralDocumentInfo::* valid_member;
        const std::wstring libCZI::GeneralDocumentInfo::* string_member;
        const char* json_key;
    } kDataToBeConverted[] =
    {
        { &libCZI::GeneralDocumentInfo::name_valid, &libCZI::GeneralDocumentInfo::name, "name" },
        { &libCZI::GeneralDocumentInfo::title_valid, &libCZI::GeneralDocumentInfo::title, "title" },
        { &libCZI::GeneralDocumentInfo::userName_valid, &libCZI::GeneralDocumentInfo::userName, "user_name" },
        { &libCZI::GeneralDocumentInfo::description_valid, &libCZI::GeneralDocumentInfo::description, "description" },
        { &libCZI::GeneralDocumentInfo::comment_valid, &libCZI::GeneralDocumentInfo::comment, "comment" },
        { &libCZI::GeneralDocumentInfo::keywords_valid, &libCZI::GeneralDocumentInfo::keywords, "keywords" },
        { &libCZI::GeneralDocumentInfo::creationDateTime_valid, &libCZI::GeneralDocumentInfo::creationDateTime, "creation_date_time" }
    };

    for (const auto& member : kDataToBeConverted)
    {
        if (general_document_info.*member.valid_member)
        {
            const string utf8_string = libCZI::Utils::ConvertToUtf8(general_document_info.*member.string_member);
            document.AddMember(
                rapidjson::Value(member.json_key, allocator).Move(),
                rapidjson::Value(utf8_string.c_str(), allocator).Move(),
                allocator);
        }
    }

    if (general_document_info.rating_valid)
    {
        document.AddMember("rating", general_document_info.rating, allocator);
    }

    return document;
}

/*static*/std::string ParameterHelpers::FormatGeneralDocumentInfoAsJsonString(const libCZI::GeneralDocumentInfo& general_document_info)
{
    return ConvertToJsonString([&]() { return FormatGeneralDocumentInfoAsJson(general_document_info); });
}

/*static*/std::string ParameterHelpers::FormatZDimensionInfoAsJsonString(const libCZI::IDimensionZInfo* z_dimension_info)
{
    return ConvertToJsonString([=]() { return FormatZDimensionInfoAsJson(z_dimension_info); });
}

/*static*/rapidjson::Document ParameterHelpers::FormatZDimensionInfoAsJson(const libCZI::IDimensionZInfo* z_dimension_info)
{
    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

    {
        double d;
        if (z_dimension_info->TryGetReferencePosition(&d))
        {
            document.AddMember("reference_position", d, allocator);
        }
    }

    {
        double offset, increment;
        if (z_dimension_info->TryGetIntervalDefinition(&offset, &increment))
        {
            rapidjson::Value interval_definition(rapidjson::kArrayType);
            interval_definition.PushBack(offset, allocator);
            interval_definition.PushBack(increment, allocator);
            document.AddMember("interval_definition", interval_definition, allocator);
        }
        else
        {
            std::vector<double> positions;
            if (z_dimension_info->TryGetPositionList(&positions))
            {
                rapidjson::Value position_list(rapidjson::kArrayType);
                for (const auto& position : positions)
                {
                    position_list.PushBack(position, allocator);
                }

                document.AddMember("position_list", position_list, allocator);
            }
        }
    }

    {
        libCZI::IDimensionZInfo::XyzHandedness xyz_handedness;
        if (z_dimension_info->TryGetXyzHandedness(&xyz_handedness))
        {
            const char* handedness_string = nullptr;
            switch (xyz_handedness)
            {
            case libCZI::IDimensionZInfo::XyzHandedness::LeftHanded:
                handedness_string = "left_handed";
                break;
            case libCZI::IDimensionZInfo::XyzHandedness::RightHanded:
                handedness_string = "right_handed";
                break;
            case libCZI::IDimensionZInfo::XyzHandedness::Undefined:
                handedness_string = "undefined";
                break;
            }

            if (handedness_string != nullptr)
            {
                document.AddMember("xyz_handedness", rapidjson::Value(handedness_string, allocator).Move(), allocator);
            }
        }
    }

    {
        libCZI::IDimensionZInfo::ZaxisDirection z_axis_direction;
        if (z_dimension_info->TryGetZAxisDirection(&z_axis_direction))
        {
            const char* direction_string = nullptr;
            switch (z_axis_direction)
            {
            case libCZI::IDimensionZInfo::ZaxisDirection::FromSpecimenToObjective:
                direction_string = "from_specimen_to_objective";
                break;
            case libCZI::IDimensionZInfo::ZaxisDirection::FromObjectiveToSpecimen:
                direction_string = "from_objective_to_specimen";
                break;
            case libCZI::IDimensionZInfo::ZaxisDirection::Undefined:
                direction_string = "undefined";
                break;
            }

            if (direction_string != nullptr)
            {
                document.AddMember("z_axis_direction", rapidjson::Value(direction_string, allocator).Move(), allocator);
            }
        }
    }

    {
        libCZI::IDimensionZInfo::ZDriveMode z_drive_mode;
        if (z_dimension_info->TryGetZDriveMode(&z_drive_mode))
        {
            const char* drive_mode_string = nullptr;
            switch (z_drive_mode)
            {
            case libCZI::IDimensionZInfo::ZDriveMode::Continuous:
                drive_mode_string = "continuous";
                break;
            case libCZI::IDimensionZInfo::ZDriveMode::Step:
                drive_mode_string = "step";
                break;
            }

            if (drive_mode_string != nullptr)
            {
                document.AddMember("z_drive_mode", rapidjson::Value(drive_mode_string, allocator).Move(), allocator);
            }
        }
    }

    {
        double z_drive_speed;
        if (z_dimension_info->TryZDriveSpeed(&z_drive_speed))
        {
            document.AddMember("z_drive_speed", z_drive_speed, allocator);
        }
    }

    return document;
}

/*static*/std::string ParameterHelpers::FormatTDimensionInfoAsJsonString(const libCZI::IDimensionTInfo* t_dimension_info)
{
    return ConvertToJsonString([=]() { return FormatTDimensionInfoAsJson(t_dimension_info); });
}


/*static*/rapidjson::Document ParameterHelpers::FormatTDimensionInfoAsJson(const libCZI::IDimensionTInfo* t_dimension_info)
{
    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

    {
        libCZI::XmlDateTime dateTime;
        if (t_dimension_info->TryGetStartTime(&dateTime))
        {
            document.AddMember("start_time", rapidjson::Value(dateTime.ToXmlString().c_str(), allocator).Move(), allocator);
        }
    }

    {
        double offset, increment;
        if (t_dimension_info->TryGetIntervalDefinition(&offset, &increment))
        {
            rapidjson::Value interval_definition(rapidjson::kArrayType);
            interval_definition.PushBack(offset, allocator);
            interval_definition.PushBack(increment, allocator);
            document.AddMember("interval_definition", interval_definition, allocator);
        }
        else
        {
            std::vector<double> positions;
            if (t_dimension_info->TryGetOffsetsList(&positions))
            {
                rapidjson::Value position_list(rapidjson::kArrayType);
                for (const auto& position : positions)
                {
                    position_list.PushBack(position, allocator);
                }

                document.AddMember("offset_list", position_list, allocator);
            }
        }
    }

    return document;
}

/*static*/std::string ParameterHelpers::FormatCDimensionInfoAsJsonString(const libCZI::IDimensionsChannelsInfo* channel_dimension_info)
{
    return ConvertToJsonString([=]() { return FormatCDimensionInfoAsJson(channel_dimension_info); });
}

/*static*/rapidjson::Document ParameterHelpers::FormatCDimensionInfoAsJson(const libCZI::IDimensionsChannelsInfo* channel_dimension_info)
{
    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

    const int channel_count = channel_dimension_info->GetChannelCount();
    for (int i = 0; i < channel_count; ++i)
    {
        auto channel_info = channel_dimension_info->GetChannel(i);
        rapidjson::Value channel_json_object = FormatIDimensionChannelInfoAsJson(channel_info.get(), allocator);

        // Add this channel's JSON object to the channels object with the index as key
        document.AddMember(
            rapidjson::Value(std::to_string(i).c_str(), allocator).Move(),
            channel_json_object,
            allocator);
    }

    return document;
}

/*static*/rapidjson::Value ParameterHelpers::FormatIDimensionChannelInfoAsJson(const libCZI::IDimensionChannelInfo* channel_info, rapidjson::Document::AllocatorType& allocator)
{
    rapidjson::Value channel_object(rapidjson::kObjectType);

    {
        wstring id;
        if (channel_info->TryGetAttributeId(&id))
        {
            const string utf8_string = libCZI::Utils::ConvertToUtf8(id);
            channel_object.AddMember("attribute_id", rapidjson::Value(utf8_string.c_str(), allocator).Move(), allocator);
        }
    }

    {
        wstring name;
        if (channel_info->TryGetAttributeName(&name))
        {
            const string utf8_string = libCZI::Utils::ConvertToUtf8(name);
            channel_object.AddMember("attribute_name", rapidjson::Value(utf8_string.c_str(), allocator).Move(), allocator);
        }
    }

    {
        libCZI::DimensionChannelChannelType channel_type;
        if (channel_info->TryGetChannelType(&channel_type))
        {
            const char* channel_type_string = nullptr;
            switch (channel_type)
            {
            case libCZI::DimensionChannelChannelType::Heightmap:
                channel_type_string = "Heightmap";
                break;
            case libCZI::DimensionChannelChannelType::PalHR:
                channel_type_string = "PalHR";
                break;
            case libCZI::DimensionChannelChannelType::PalWidefield:
                channel_type_string = "PalWidefield";
                break;
            case libCZI::DimensionChannelChannelType::SimHR:
                channel_type_string = "SimHR";
                break;
            case libCZI::DimensionChannelChannelType::SimWidefield:
                channel_type_string = "SimWidefield";
                break;
            case libCZI::DimensionChannelChannelType::SimDWF:
                channel_type_string = "SimDWF";
                break;
            case libCZI::DimensionChannelChannelType::AiryScanSum:
                channel_type_string = "AiryScanSum";
                break;
            case libCZI::DimensionChannelChannelType::AiryScanRawSr:
                channel_type_string = "AiryScanRawSr";
                break;
            case libCZI::DimensionChannelChannelType::AiryScanRaw:
                channel_type_string = "AiryScanRaw";
                break;
            case libCZI::DimensionChannelChannelType::AiryScanSr:
                channel_type_string = "AiryScanSr";
                break;
            case libCZI::DimensionChannelChannelType::AiryScanVp:
                channel_type_string = "AiryScanVp";
                break;
            case libCZI::DimensionChannelChannelType::AiryScanMb:
                channel_type_string = "AiryScanMb";
                break;
            case libCZI::DimensionChannelChannelType::AiryScanRingSheppardSum:
                channel_type_string = "AiryScanRingSheppardSum";
                break;
            case libCZI::DimensionChannelChannelType::OnlineUnmixing:
                channel_type_string = "OnlineUnmixing";
                break;
            case libCZI::DimensionChannelChannelType::Unspecified:
                channel_type_string = "Unspecified";
                break;
            }

            if (channel_type_string != nullptr)
            {
                channel_object.AddMember("channel_type", rapidjson::Value(channel_type_string, allocator).Move(), allocator);
            }
        }

        {
            wstring unit;
            if (channel_info->TryGetChannelUnit(&unit))
            {
                const string utf8_string = libCZI::Utils::ConvertToUtf8(unit);
                channel_object.AddMember("channel_unit", rapidjson::Value(utf8_string.c_str(), allocator).Move(), allocator);
            }
        }

        {
            libCZI::PixelType pixel_type;
            if (channel_info->TryGetPixelType(&pixel_type))
            {
                const char* pixel_type_string = nullptr;
                switch (pixel_type)
                {
                case libCZI::PixelType::Gray8:
                    pixel_type_string = "Gray8";
                    break;
                case libCZI::PixelType::Gray16:
                    pixel_type_string = "Gray16";
                    break;
                case libCZI::PixelType::Gray32Float:
                    pixel_type_string = "Gray32Float";
                    break;
                case libCZI::PixelType::Bgr24:
                    pixel_type_string = "Bgr24";
                    break;
                case libCZI::PixelType::Bgr48:
                    pixel_type_string = "Bgr48";
                    break;
                case libCZI::PixelType::Bgr96Float:
                    pixel_type_string = "Bgr96Float";
                    break;
                case libCZI::PixelType::Bgra32:
                    pixel_type_string = "Bgra32";
                    break;
                case libCZI::PixelType::Gray64ComplexFloat:
                    pixel_type_string = "Gray64ComplexFloat";
                    break;
                case libCZI::PixelType::Bgr192ComplexFloat:
                    pixel_type_string = "Bgr192ComplexFloat";
                    break;
                case libCZI::PixelType::Gray32:
                    pixel_type_string = "Gray32";
                    break;
                case libCZI::PixelType::Gray64Float:
                    pixel_type_string = "Gray64Float";
                    break;
                }

                if (pixel_type_string != nullptr)
                {
                    channel_object.AddMember("pixel_type", rapidjson::Value(pixel_type_string, allocator).Move(), allocator);
                }
            }
        }
    }

    {
        int component_bit_count;
        if (channel_info->TryGetComponentBitCount(&component_bit_count))
        {
            channel_object.AddMember("component_bit_count", component_bit_count, allocator);
        }
    }

    {
        libCZI::DimensionChannelAcquisitionMode acquisition_mode;
        if (channel_info->TryGetAcquisitionMode(&acquisition_mode))
        {
            const char* acquisition_mode_string = nullptr;
            switch (acquisition_mode)
            {
            case libCZI::DimensionChannelAcquisitionMode::WideField:
                acquisition_mode_string = "WideField";
                break;
            case libCZI::DimensionChannelAcquisitionMode::LaserScanningConfocalMicroscopy:
                acquisition_mode_string = "LaserScanningConfocalMicroscopy";
                break;
            case libCZI::DimensionChannelAcquisitionMode::SpinningDiskConfocal:
                acquisition_mode_string = "SpinningDiskConfocal";
                break;
            case libCZI::DimensionChannelAcquisitionMode::SlitScanConfocal:
                acquisition_mode_string = "SlitScanConfocal";
                break;
            case libCZI::DimensionChannelAcquisitionMode::MultiPhotonMicroscopy:
                acquisition_mode_string = "MultiPhotonMicroscopy";
                break;
            case libCZI::DimensionChannelAcquisitionMode::StructuredIllumination:
                acquisition_mode_string = "StructuredIllumination";
                break;
            case libCZI::DimensionChannelAcquisitionMode::SingleMoleculeImaging:
                acquisition_mode_string = "SingleMoleculeImaging";
                break;
            case libCZI::DimensionChannelAcquisitionMode::TotalInternalReflection:
                acquisition_mode_string = "TotalInternalReflection";
                break;
            case libCZI::DimensionChannelAcquisitionMode::FluorescenceLifetime:
                acquisition_mode_string = "FluorescenceLifetime";
                break;
            case libCZI::DimensionChannelAcquisitionMode::SpectralImaging:
                acquisition_mode_string = "SpectralImaging";
                break;
            case libCZI::DimensionChannelAcquisitionMode::FluorescenceCorrelationSpectroscopy:
                acquisition_mode_string = "FluorescenceCorrelationSpectroscopy";
                break;
            case libCZI::DimensionChannelAcquisitionMode::NearFieldScanningOpticalMicroscopy:
                acquisition_mode_string = "NearFieldScanningOpticalMicroscopy";
                break;
            case libCZI::DimensionChannelAcquisitionMode::SecondHarmonicGenerationImaging:
                acquisition_mode_string = "SecondHarmonicGenerationImaging";
                break;
            case libCZI::DimensionChannelAcquisitionMode::PALM:
                acquisition_mode_string = "PALM";
                break;
            case libCZI::DimensionChannelAcquisitionMode::STORM:
                acquisition_mode_string = "STORM";
                break;
            case libCZI::DimensionChannelAcquisitionMode::STED:
                acquisition_mode_string = "STED";
                break;
            case libCZI::DimensionChannelAcquisitionMode::TIRF:
                acquisition_mode_string = "TIRF";
                break;
            case libCZI::DimensionChannelAcquisitionMode::FSM:
                acquisition_mode_string = "FSM";
                break;
            case libCZI::DimensionChannelAcquisitionMode::LCM:
                acquisition_mode_string = "LCM";
                break;
            case libCZI::DimensionChannelAcquisitionMode::SPIM:
                acquisition_mode_string = "SPIM";
                break;
            case libCZI::DimensionChannelAcquisitionMode::SEM:
                acquisition_mode_string = "SEM";
                break;
            case libCZI::DimensionChannelAcquisitionMode::FIB:
                acquisition_mode_string = "FIB";
                break;
            case libCZI::DimensionChannelAcquisitionMode::FIB_SEM:
                acquisition_mode_string = "FIB_SEM";
                break;
            case libCZI::DimensionChannelAcquisitionMode::ApertureCorrelation:
                acquisition_mode_string = "ApertureCorrelation";
                break;
            case libCZI::DimensionChannelAcquisitionMode::Other:
                acquisition_mode_string = "Other";
                break;
            }

            if (acquisition_mode_string != nullptr)
            {
                channel_object.AddMember("acquisition_mode", rapidjson::Value(acquisition_mode_string, allocator).Move(), allocator);
            }
        }
    }

    {
        libCZI::DimensionChannelIlluminationType illumination_type;
        if (channel_info->TryGetIlluminationType(&illumination_type))
        {
            const char* illumination_type_string = nullptr;
            switch (illumination_type)
            {
            case libCZI::DimensionChannelIlluminationType::Transmitted:
                illumination_type_string = "Transmitted";
                break;
            case libCZI::DimensionChannelIlluminationType::Epifluorescence:
                illumination_type_string = "Epifluorescence";
                break;
            case libCZI::DimensionChannelIlluminationType::Oblique:
                illumination_type_string = "Oblique";
                break;
            case libCZI::DimensionChannelIlluminationType::NonLinear:
                illumination_type_string = "NonLinear";
                break;
            case libCZI::DimensionChannelIlluminationType::Other:
                illumination_type_string = "Other";
                break;
            }

            if (illumination_type_string != nullptr)
            {
                channel_object.AddMember("illumination_type", rapidjson::Value(illumination_type_string, allocator).Move(), allocator);
            }
        }
    }

    {
        libCZI::DimensionChannelContrastMethod contrast_method;
        if (channel_info->TryGetContrastMethod(&contrast_method))
        {
            const char* contrast_method_string = nullptr;
            switch (contrast_method)
            {
            case libCZI::DimensionChannelContrastMethod::Brightfield:
                contrast_method_string = "Brightfield";
                break;
            case libCZI::DimensionChannelContrastMethod::Phase:
                contrast_method_string = "Phase";
                break;
            case libCZI::DimensionChannelContrastMethod::DIC:
                contrast_method_string = "DIC";
                break;
            case libCZI::DimensionChannelContrastMethod::HoffmanModulation:
                contrast_method_string = "HoffmanModulation";
                break;
            case libCZI::DimensionChannelContrastMethod::ObliqueIllumination:
                contrast_method_string = "ObliqueIllumination";
                break;
            case libCZI::DimensionChannelContrastMethod::PolarizedLight:
                contrast_method_string = "PolarizedLight";
                break;
            case libCZI::DimensionChannelContrastMethod::Darkfield:
                contrast_method_string = "Darkfield";
                break;
            case libCZI::DimensionChannelContrastMethod::Fluorescence:
                contrast_method_string = "Fluorescence";
                break;
            case libCZI::DimensionChannelContrastMethod::MultiPhotonFluorescence:
                contrast_method_string = "MultiPhotonFluorescence";
                break;
            case libCZI::DimensionChannelContrastMethod::Other:
                contrast_method_string = "Other";
                break;
            }

            if (contrast_method_string != nullptr)
            {
                channel_object.AddMember("contrast_method", rapidjson::Value(contrast_method_string, allocator).Move(), allocator);
            }
        }
    }

    {
        libCZI::SpectrumCharacteristics spectrum_characteristics;
        if (channel_info->TryGetIlluminationWavelength(&spectrum_characteristics))
        {
            channel_object.AddMember("illumination_wavelength", FormatSpectrumCharacteristicsAsJson(spectrum_characteristics, allocator), allocator);
        }
    }

    {
        libCZI::SpectrumCharacteristics spectrum_characteristics;
        if (channel_info->TryGetDetectionWavelength(&spectrum_characteristics))
        {
            channel_object.AddMember("detection_wavelength", FormatSpectrumCharacteristicsAsJson(spectrum_characteristics, allocator), allocator);
        }
    }

    {
        double excitation_wavelength;
        if (channel_info->TryGetExcitationWavelength(&excitation_wavelength))
        {
            channel_object.AddMember("excitation_wavelength", excitation_wavelength, allocator);
        }
    }

    {
        double emission_wavelength;
        if (channel_info->TryGetEmissionWavelength(&emission_wavelength))
        {
            channel_object.AddMember("emission_wavelength", emission_wavelength, allocator);
        }
    }

    {
        double effective_na;
        if (channel_info->TryGetEffectiveNA(&effective_na))
        {
            channel_object.AddMember("effective_na", effective_na, allocator);
        }
    }

    {
        wstring dye_id;
        if (channel_info->TryGetDyeId(&dye_id))
        {
            const string utf8_string = libCZI::Utils::ConvertToUtf8(dye_id);
            channel_object.AddMember("dye_id", rapidjson::Value(utf8_string.c_str(), allocator).Move(), allocator);
        }
    }

    {
        wstring dye_database_id;
        if (channel_info->TryGetDyeDatabaseId(&dye_database_id))
        {
            const string utf8_string = libCZI::Utils::ConvertToUtf8(dye_database_id);
            channel_object.AddMember("dye_database_id", rapidjson::Value(utf8_string.c_str(), allocator).Move(), allocator);
        }
    }

    {
        double pinhole_size;
        if (channel_info->TryGetPinholeSize(&pinhole_size))
        {
            channel_object.AddMember("pinhole_size", pinhole_size, allocator);
        }
    }

    {
        double pinhole_size_in_airy_units;
        if (channel_info->TryGetPinholeSizeAiry(&pinhole_size_in_airy_units))
        {
            channel_object.AddMember("pinhole_size_airy", pinhole_size_in_airy_units, allocator);
        }
    }

    {
        libCZI::DimensionChannelPinholeGeometry pinhole_geometry;
        if (channel_info->TryGetPinholeGeometry(&pinhole_geometry))
        {
            const char* pinhole_geometry_string = nullptr;
            switch (pinhole_geometry)
            {
            case libCZI::DimensionChannelPinholeGeometry::Circular:
                pinhole_geometry_string = "Circular";
                break;
            case libCZI::DimensionChannelPinholeGeometry::Rectangular:
                pinhole_geometry_string = "Rectangular";
                break;
            case libCZI::DimensionChannelPinholeGeometry::Other:
                pinhole_geometry_string = "Other";
                break;
            }

            if (pinhole_geometry_string != nullptr)
            {
                channel_object.AddMember("pinhole_geometry", rapidjson::Value(pinhole_geometry_string, allocator).Move(), allocator);
            }
        }
    }

    {
        wstring fluorophore;
        if (channel_info->TryGetFluor(&fluorophore))
        {
            const string utf8_string = libCZI::Utils::ConvertToUtf8(fluorophore);
            channel_object.AddMember("fluor", rapidjson::Value(utf8_string.c_str(), allocator).Move(), allocator);
        }
    }

    {
        double nd_filter;
        if (channel_info->TryGetNDFilter(&nd_filter))
        {
            channel_object.AddMember("nd_filter", nd_filter, allocator);
        }
    }

    {
        int pocket_cell_setting;
        if (channel_info->TryGetPocketCellSetting(&pocket_cell_setting))
        {
            channel_object.AddMember("pocket_cell_setting", pocket_cell_setting, allocator);
        }
    }

    {
        libCZI::Rgb8Color color;
        if (channel_info->TryGetColor(&color))
        {
            rapidjson::Value color_json_object(rapidjson::kObjectType);
            channel_object.AddMember("color", rapidjson::Value(ConvertColorToHexString(color).c_str(), allocator).Move(), allocator);
        }
    }

    {
        libCZI::RangeOrSingleValue<std::uint64_t> exposure_time;
        if (channel_info->TryGetExposureTime(&exposure_time))
        {
            ostringstream string_stream;
            if (exposure_time.singleValue)
            {
                string_stream << exposure_time.startOrSingleValue;
            }
            else
            {
                string_stream << exposure_time.startOrSingleValue << "-" << exposure_time.end;
            }

            channel_object.AddMember("exposure_time", rapidjson::Value(string_stream.str().c_str(), allocator).Move(), allocator);
        }
    }

    {
        double depth_of_focus;
        if (channel_info->TryGetDepthOfFocus(&depth_of_focus))
        {
            channel_object.AddMember("depth_of_focus", depth_of_focus, allocator);
        }
    }

    {
        double section_thickness;
        if (channel_info->TryGetSectionThickness(&section_thickness))
        {
            channel_object.AddMember("section_thickness", section_thickness, allocator);
        }
    }

    // TODO(JBL): add GetDetectorSettings, GetLightSourcesSettings, GetLightPath, GetLaserScanInfo, GetSPIMIlluminationSettings,
    //                 GetSPIMDetectionSettings, GetSIMSettings, GetPolarizingSettings, GetAiryscanSettings

    {
        wstring reflector;
        if (channel_info->TryGetReflector(&reflector))
        {
            const string utf8_string = libCZI::Utils::ConvertToUtf8(reflector);
            channel_object.AddMember("reflector", rapidjson::Value(utf8_string.c_str(), allocator).Move(), allocator);
        }
    }

    {
        wstring condensor_contrast;
        if (channel_info->TryGetReflector(&condensor_contrast))
        {
            const string utf8_string = libCZI::Utils::ConvertToUtf8(condensor_contrast);
            channel_object.AddMember("condensor_contrast", rapidjson::Value(utf8_string.c_str(), allocator).Move(), allocator);
        }
    }

    {
        double na_condensor;
        if (channel_info->TryGetNACondenser(&na_condensor))
        {
            channel_object.AddMember("na_condensor", na_condensor, allocator);
        }
    }

    // TODO(JBL): add GetRatio

    return channel_object;
}

/*static*/rapidjson::Value ParameterHelpers::FormatSpectrumCharacteristicsAsJson(const libCZI::SpectrumCharacteristics& spectrum_characteristics, rapidjson::Document::AllocatorType& allocator)
{
    rapidjson::Value spectrum_characteristics_json_object(rapidjson::kObjectType);
    if (spectrum_characteristics.type == libCZI::SpectrumCharacteristics::InformationType::SinglePeak)
    {
        spectrum_characteristics_json_object.AddMember("type", "SinglePeak", allocator);
        spectrum_characteristics_json_object.AddMember("single_peak", spectrum_characteristics.singlePeak, allocator);
    }
    else if (spectrum_characteristics.type == libCZI::SpectrumCharacteristics::InformationType::Ranges)
    {
        spectrum_characteristics_json_object.AddMember("type", "Ranges", allocator);
        ostringstream string_stream;
        bool is_first = true;
        for (const auto& range : spectrum_characteristics.ranges)
        {
            if (!is_first)
            {
                string_stream << ",";
            }

            if (range.singleValue)
            {
                string_stream << range.startOrSingleValue;
            }
            else
            {
                string_stream << range.startOrSingleValue << "-" << range.end;
            }

            is_first = false;
        }

        spectrum_characteristics_json_object.AddMember("ranges", rapidjson::Value(string_stream.str().c_str(), allocator).Move(), allocator);
    }

    return spectrum_characteristics_json_object;
}

//******************************************************************************

/*static*/void ParameterHelpers::FillOutCompositionChannelInfoFromDisplaySettings(const libCZI::IDisplaySettings* display_settings, int channel_index, bool sixteen_or_eight_bits_lut, CompositionChannelInfoInterop& composition_channel_info)
{
    memset(&composition_channel_info, 0, sizeof(CompositionChannelInfoInterop));

    const auto channel_display_settings = display_settings->GetChannelDisplaySettings(channel_index);

    if (!channel_display_settings->GetIsEnabled())
    {
        composition_channel_info.weight = 0.0f;
    }
    else
    {
        composition_channel_info.weight = channel_display_settings->GetWeight();

        channel_display_settings->GetBlackWhitePoint(&composition_channel_info.black_point, &composition_channel_info.white_point);

        libCZI::Rgb8Color tinting_color;
        if (channel_display_settings->TryGetTintingColorRgb8(&tinting_color))
        {
            composition_channel_info.enable_tinting = true;
            composition_channel_info.tinting_color_r = tinting_color.r;
            composition_channel_info.tinting_color_g = tinting_color.g;
            composition_channel_info.tinting_color_b = tinting_color.b;
        }
        else
        {
            composition_channel_info.enable_tinting = false;
        }

        // see. libCZIHelpers.h line 162
        switch (channel_display_settings->GetGradationCurveMode())
        {
        case libCZI::IDisplaySettings::GradationCurveMode::Linear:
            break;
        case libCZI::IDisplaySettings::GradationCurveMode::Gamma:
        {
            const int lutSize = sixteen_or_eight_bits_lut ? 256 * 256 : 256;
            float gamma;
            channel_display_settings->TryGetGamma(&gamma);  // TODO(JBL): handle error, must not get 'false' here
            const auto lut = libCZI::Utils::Create8BitLookUpTableFromGamma(lutSize, composition_channel_info.black_point, composition_channel_info.white_point, gamma);
            composition_channel_info.look_up_table_element_count = lutSize;
            auto p = ParameterHelpers::AllocateMemory(lutSize);
            memcpy(p, lut.data(), lutSize);
            composition_channel_info.ptr_look_up_table = static_cast<uint8_t*>(p);
        }

        break;
        case libCZI::IDisplaySettings::GradationCurveMode::Spline:
        {
            const int lutSize = sixteen_or_eight_bits_lut ? 256 * 256 : 256;
            std::vector<libCZI::IDisplaySettings::SplineData> splineData;
            channel_display_settings->TryGetSplineData(&splineData);// TODO(JBL): handle error, must not get 'false' here
            const auto lut = libCZI::Utils::Create8BitLookUpTableFromSplines(lutSize, composition_channel_info.black_point, composition_channel_info.white_point, splineData);
            composition_channel_info.look_up_table_element_count = lutSize;
            auto p = ParameterHelpers::AllocateMemory(lutSize);
            memcpy(p, lut.data(), lutSize);
            composition_channel_info.ptr_look_up_table = static_cast<uint8_t*>(p);
        }

        break;
        }
    }
}
