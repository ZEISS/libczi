// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include <libCZIApi.h>
#include <../src/parameterhelpers.h>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include "rapidjson/schema.h"

#include <CziDimensionInfo.h>
#include <CziMetadataDocumentInfo2.h>

#include <random>
#include <map>
#include <string>

using namespace std;

namespace
{
    std::string GenerateRandomString(size_t length)
    {
        constexpr char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\"'?#*+-$%&/(){[]}";
        constexpr size_t charset_size = sizeof(charset) - 1;
        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<> dist(0, charset_size - 1);

        std::string random_string;
        for (size_t i = 0; i < length; ++i)
        {
            random_string += charset[dist(generator)];
        }

        return random_string;
    }

    std::map<int, libCZI::StreamsFactory::Property> GeneratePropertyBagForStreamsFactoryWithRandomContent()
    {
        map<int, libCZI::StreamsFactory::Property> property_bag;

        const libCZI::StreamsFactory::StreamPropertyBagPropertyInfo* property_bag_info = libCZI::StreamsFactory::GetStreamPropertyBagPropertyInfo(nullptr);
        if (property_bag_info != nullptr && property_bag_info->property_name != nullptr)
        {
            // Random number generators
            random_device rd;
            mt19937 generator(rd());
            uniform_int_distribution<int> int_dist(-1000, 1000);
            uniform_real_distribution<float> float_dist(-1000.0f, 1000.0f);
            uniform_real_distribution<double> double_dist(-1000.0, 1000.0);
            uniform_int_distribution<int> bool_dist(0, 1);

            for (; property_bag_info->property_name != nullptr; ++property_bag_info)
            {
                switch (property_bag_info->property_type)
                {
                case libCZI::StreamsFactory::Property::Type::Int32:
                    property_bag[property_bag_info->property_id] = libCZI::StreamsFactory::Property{ int_dist(generator) };
                    break;
                case libCZI::StreamsFactory::Property::Type::Float:
                    property_bag[property_bag_info->property_id] = libCZI::StreamsFactory::Property{ float_dist(generator) };
                    break;
                case libCZI::StreamsFactory::Property::Type::Double:
                    property_bag[property_bag_info->property_id] = libCZI::StreamsFactory::Property{ double_dist(generator) };
                    break;
                case libCZI::StreamsFactory::Property::Type::Boolean:
                    property_bag[property_bag_info->property_id] = libCZI::StreamsFactory::Property{ static_cast<bool>(bool_dist(generator)) };
                    break;
                case libCZI::StreamsFactory::Property::Type::String:
                    property_bag[property_bag_info->property_id] = libCZI::StreamsFactory::Property{ GenerateRandomString(10) };
                    break;
                default:
                    break;
                }
            }
        }

        return property_bag;
    }

    string PropertyIdToPropertyName(int property_id)
    {
        const libCZI::StreamsFactory::StreamPropertyBagPropertyInfo* property_bag_info = libCZI::StreamsFactory::GetStreamPropertyBagPropertyInfo(nullptr);
        for (; property_bag_info->property_name != nullptr; ++property_bag_info)
        {
            if (property_bag_info->property_id == property_id)
            {
                return property_bag_info->property_name;
            }
        }

        return "";
    }

    std::string CreationPropertyBagToJson(const map<int, libCZI::StreamsFactory::Property>& property_bag)
    {
        rapidjson::Document document;
        document.SetObject();
        rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

        for (const auto& prop : property_bag)
        {
            const std::string propertyName = PropertyIdToPropertyName(prop.first);
            rapidjson::Value key(propertyName.c_str(), allocator);

            switch (prop.second.GetType())
            {
            case libCZI::StreamsFactory::Property::Type::Int32:
            {
                int value = prop.second.GetAsInt32OrThrow();
                document.AddMember(key, value, allocator);
                break;
            }
            case libCZI::StreamsFactory::Property::Type::Float:
            {
                float value = prop.second.GetAsFloatOrThrow();
                document.AddMember(key, value, allocator);
                break;
            }
            case libCZI::StreamsFactory::Property::Type::Double:
            {
                double value = prop.second.GetAsDoubleOrThrow();
                document.AddMember(key, value, allocator);
                break;
            }
            case libCZI::StreamsFactory::Property::Type::Boolean:
            {
                bool value = prop.second.GetAsBoolOrThrow();
                document.AddMember(key, value, allocator);
                break;
            }
            case libCZI::StreamsFactory::Property::Type::String:
            {
                const std::string& value = prop.second.GetAsStringOrThrow();
                rapidjson::Value jsonValue(value.c_str(), allocator);
                document.AddMember(key, jsonValue, allocator);
                break;
            }
            default:
                break;
            }
        }

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        document.Accept(writer);

        return buffer.GetString();
    }

    bool CompareForEquality(const libCZI::StreamsFactory::Property& property_a, const libCZI::StreamsFactory::Property& property_b)
    {
        if (property_a.GetType() != property_b.GetType())
        {
            return false;
        }

        switch (property_a.GetType())
        {
        case libCZI::StreamsFactory::Property::Type::Int32:
            return property_a.GetAsInt32OrThrow() == property_b.GetAsInt32OrThrow();
        case libCZI::StreamsFactory::Property::Type::Float:
            return fabs(property_a.GetAsFloatOrThrow() - property_b.GetAsFloatOrThrow()) < 1e-9f;
        case libCZI::StreamsFactory::Property::Type::Double:
            return fabs(property_a.GetAsDoubleOrThrow() - property_b.GetAsDoubleOrThrow()) < 1e-9f;
        case libCZI::StreamsFactory::Property::Type::Boolean:
            return property_a.GetAsBoolOrThrow() == property_b.GetAsBoolOrThrow();
        case libCZI::StreamsFactory::Property::Type::String:
            return property_a.GetAsStringOrThrow() == property_b.GetAsStringOrThrow();
        default:
            return false;
        }
    }

    bool CompareForEquality(const map<int, libCZI::StreamsFactory::Property>& property_bag_1, const map<int, libCZI::StreamsFactory::Property>& property_bag_2)
    {
        if (property_bag_1.size() != property_bag_2.size())
        {
            return false;
        }

        for (const auto& prop : property_bag_1)
        {
            const auto it = property_bag_2.find(prop.first);
            if (it == property_bag_2.end())
            {
                return false;
            }

            if (!CompareForEquality(prop.second, it->second))
            {
                return false;
            }
        }

        return true;
    }
}

TEST(CZIAPI_ParameterHelpers, Check_TryParseInputStreamCreationPropertyBag_with_invalid_input)
{
    // here we use some bogus input, and expect a failure
    std::map<int, libCZI::StreamsFactory::Property> property_bag;
    const bool b = ParameterHelpers::TryParseInputStreamCreationPropertyBag("this is not a valid property bag", &property_bag);
    EXPECT_FALSE(b);
}

TEST(CZIAPI_ParameterHelpers, Check_TryParseInputStreamCreationPropertyBag_with_all_possible_properties)
{
    auto property_bag = GeneratePropertyBagForStreamsFactoryWithRandomContent();
    const string property_bag_string = CreationPropertyBagToJson(property_bag);

    map<int, libCZI::StreamsFactory::Property> property_bag_parsed;
    bool b = ParameterHelpers::TryParseInputStreamCreationPropertyBag(property_bag_string, &property_bag_parsed);
    EXPECT_TRUE(b);

    b = CompareForEquality(property_bag, property_bag_parsed);
    EXPECT_TRUE(b);
}

TEST(CZIAPI_ParameterHelpers, Check_ConvertLibCZIPyramidStatisticsToJson)
{
    libCZI::PyramidStatistics pyramid_statistics;
    pyramid_statistics.scenePyramidStatistics[0] =
    {
        libCZI::PyramidStatistics::PyramidLayerStatistics{ libCZI::PyramidStatistics::PyramidLayerInfo{2,0}, 10 },
        libCZI::PyramidStatistics::PyramidLayerStatistics{ libCZI::PyramidStatistics::PyramidLayerInfo{2,1}, 4 }
    };
    pyramid_statistics.scenePyramidStatistics[1] =
    {
            libCZI::PyramidStatistics::PyramidLayerStatistics{ libCZI::PyramidStatistics::PyramidLayerInfo{2,0}, 20 },
            libCZI::PyramidStatistics::PyramidLayerStatistics{ libCZI::PyramidStatistics::PyramidLayerInfo{2,1}, 6 },
            libCZI::PyramidStatistics::PyramidLayerStatistics{ libCZI::PyramidStatistics::PyramidLayerInfo{2,2}, 1 },
    };

    auto json_document = ParameterHelpers::ConvertLibCZIPyramidStatisticsToJson(pyramid_statistics);
    EXPECT_TRUE(json_document.IsObject());

    // Check the structure of the JSON document
    EXPECT_TRUE(json_document.HasMember("scenePyramidStatistics"));
    const auto& scenePyramidStatistics = json_document["scenePyramidStatistics"];
    EXPECT_TRUE(scenePyramidStatistics.IsObject());

    // Check the first scene's pyramid statistics
    EXPECT_TRUE(scenePyramidStatistics.HasMember("0"));
    const auto& scene0 = scenePyramidStatistics["0"];
    EXPECT_TRUE(scene0.IsArray());
    EXPECT_EQ(scene0.Size(), 2);

    EXPECT_TRUE(scene0[0].IsObject());
    EXPECT_TRUE(scene0[0].HasMember("layerInfo"));
    EXPECT_TRUE(scene0[0]["layerInfo"].IsObject());
    EXPECT_TRUE(scene0[0]["layerInfo"].HasMember("minificationFactor"));
    EXPECT_EQ(scene0[0]["layerInfo"]["minificationFactor"].GetInt(), 2);
    EXPECT_TRUE(scene0[0]["layerInfo"].HasMember("pyramidLayerNo"));
    EXPECT_EQ(scene0[0]["layerInfo"]["pyramidLayerNo"].GetInt(), 0);
    EXPECT_TRUE(scene0[0].HasMember("count"));
    EXPECT_EQ(scene0[0]["count"].GetInt(), 10);

    EXPECT_TRUE(scene0[1].IsObject());
    EXPECT_TRUE(scene0[1].HasMember("layerInfo"));
    EXPECT_TRUE(scene0[1]["layerInfo"].IsObject());
    EXPECT_TRUE(scene0[1]["layerInfo"].HasMember("minificationFactor"));
    EXPECT_EQ(scene0[1]["layerInfo"]["minificationFactor"].GetInt(), 2);
    EXPECT_TRUE(scene0[1]["layerInfo"].HasMember("pyramidLayerNo"));
    EXPECT_EQ(scene0[1]["layerInfo"]["pyramidLayerNo"].GetInt(), 1);
    EXPECT_TRUE(scene0[1].HasMember("count"));
    EXPECT_EQ(scene0[1]["count"].GetInt(), 4);

    // Check the second scene's pyramid statistics
    EXPECT_TRUE(scenePyramidStatistics.HasMember("1"));
    const auto& scene1 = scenePyramidStatistics["1"];
    EXPECT_TRUE(scene1.IsArray());
    EXPECT_EQ(scene1.Size(), 3);

    EXPECT_TRUE(scene1[0].IsObject());
    EXPECT_TRUE(scene1[0].HasMember("layerInfo"));
    EXPECT_TRUE(scene1[0]["layerInfo"].IsObject());
    EXPECT_TRUE(scene1[0]["layerInfo"].HasMember("minificationFactor"));
    EXPECT_EQ(scene1[0]["layerInfo"]["minificationFactor"].GetInt(), 2);
    EXPECT_TRUE(scene1[0]["layerInfo"].HasMember("pyramidLayerNo"));
    EXPECT_EQ(scene1[0]["layerInfo"]["pyramidLayerNo"].GetInt(), 0);
    EXPECT_TRUE(scene1[0].HasMember("count"));
    EXPECT_EQ(scene1[0]["count"].GetInt(), 20);

    EXPECT_TRUE(scene1[1].IsObject());
    EXPECT_TRUE(scene1[1].HasMember("layerInfo"));
    EXPECT_TRUE(scene1[1]["layerInfo"].IsObject());
    EXPECT_TRUE(scene1[1]["layerInfo"].HasMember("minificationFactor"));
    EXPECT_EQ(scene1[1]["layerInfo"]["minificationFactor"].GetInt(), 2);
    EXPECT_TRUE(scene1[1]["layerInfo"].HasMember("pyramidLayerNo"));
    EXPECT_EQ(scene1[1]["layerInfo"]["pyramidLayerNo"].GetInt(), 1);
    EXPECT_TRUE(scene1[1].HasMember("count"));
    EXPECT_EQ(scene1[1]["count"].GetInt(), 6);

    EXPECT_TRUE(scene1[2].IsObject());
    EXPECT_TRUE(scene1[2].HasMember("layerInfo"));
    EXPECT_TRUE(scene1[2]["layerInfo"].IsObject());
    EXPECT_TRUE(scene1[2]["layerInfo"].HasMember("minificationFactor"));
    EXPECT_EQ(scene1[2]["layerInfo"]["minificationFactor"].GetInt(), 2);
    EXPECT_TRUE(scene1[2]["layerInfo"].HasMember("pyramidLayerNo"));
    EXPECT_EQ(scene1[2]["layerInfo"]["pyramidLayerNo"].GetInt(), 2);
    EXPECT_TRUE(scene1[2].HasMember("count"));
    EXPECT_EQ(scene1[2]["count"].GetInt(), 1);
}

TEST(CZIAPI_ParameterHelpers, Check_ConvertLibCZIPyramidStatisticsToJsonString)
{
    libCZI::PyramidStatistics pyramid_statistics;
    pyramid_statistics.scenePyramidStatistics[0] =
    {
        libCZI::PyramidStatistics::PyramidLayerStatistics{ libCZI::PyramidStatistics::PyramidLayerInfo{2,0}, 10 },
        libCZI::PyramidStatistics::PyramidLayerStatistics{ libCZI::PyramidStatistics::PyramidLayerInfo{2,1}, 4 }
    };
    pyramid_statistics.scenePyramidStatistics[1] =
    {
        libCZI::PyramidStatistics::PyramidLayerStatistics{ libCZI::PyramidStatistics::PyramidLayerInfo{2,0}, 20 },
        libCZI::PyramidStatistics::PyramidLayerStatistics{ libCZI::PyramidStatistics::PyramidLayerInfo{2,1}, 6 },
        libCZI::PyramidStatistics::PyramidLayerStatistics{ libCZI::PyramidStatistics::PyramidLayerInfo{2,2}, 1 },
    };

    auto json_string = ParameterHelpers::ConvertLibCZIPyramidStatisticsToJsonString(pyramid_statistics);

    static const char* json_schema = R"(
    {
        "type": "object",
        "properties": {
            "scenePyramidStatistics": {
                "type": "object",
                "patternProperties": {
                    "^[0-9]+$": {
                        "type": "array",
                        "items": {
                            "type": "object",
                            "properties": {
                                "layerInfo": {
                                    "type": "object",
                                    "properties": {
                                        "minificationFactor": { "type": "number" },
                                        "pyramidLayerNo": { "type": "number" }
                                    },
                                    "required": ["minificationFactor", "pyramidLayerNo"]
                                },
                                "count": { "type": "number" }
                            },
                            "required": ["layerInfo", "count"]
                        }
                    }
                }
            }
        },
        "required": ["scenePyramidStatistics"]
    }
    )";

    rapidjson::Document schema_document;
    schema_document.Parse(json_schema);
    ASSERT_FALSE(schema_document.HasParseError()) << "Schema JSON parse error: " << schema_document.GetParseError();

    rapidjson::SchemaDocument schema(schema_document);
    rapidjson::SchemaValidator validator(schema);

    rapidjson::Document json_document;
    json_document.Parse(json_string.c_str());
    ASSERT_FALSE(json_document.HasParseError()) << "JSON parse error: " << json_document.GetParseError();

    ASSERT_TRUE(json_document.Accept(validator)) << "JSON does not conform to schema";
    ASSERT_TRUE(validator.IsValid()) << "JSON is invalid according to schema";

    EXPECT_TRUE(json_document.IsObject());

    // Check the structure of the JSON document
    EXPECT_TRUE(json_document.HasMember("scenePyramidStatistics"));
    const auto& scenePyramidStatistics = json_document["scenePyramidStatistics"];
    EXPECT_TRUE(scenePyramidStatistics.IsObject());

    // Check the first scene's pyramid statistics
    EXPECT_TRUE(scenePyramidStatistics.HasMember("0"));
    const auto& scene0 = scenePyramidStatistics["0"];
    EXPECT_TRUE(scene0.IsArray());
    EXPECT_EQ(scene0.Size(), 2);

    EXPECT_TRUE(scene0[0].IsObject());
    EXPECT_TRUE(scene0[0].HasMember("layerInfo"));
    EXPECT_TRUE(scene0[0]["layerInfo"].IsObject());
    EXPECT_TRUE(scene0[0]["layerInfo"].HasMember("minificationFactor"));
    EXPECT_EQ(scene0[0]["layerInfo"]["minificationFactor"].GetInt(), 2);
    EXPECT_TRUE(scene0[0]["layerInfo"].HasMember("pyramidLayerNo"));
    EXPECT_EQ(scene0[0]["layerInfo"]["pyramidLayerNo"].GetInt(), 0);
    EXPECT_TRUE(scene0[0].HasMember("count"));
    EXPECT_EQ(scene0[0]["count"].GetInt(), 10);

    EXPECT_TRUE(scene0[1].IsObject());
    EXPECT_TRUE(scene0[1].HasMember("layerInfo"));
    EXPECT_TRUE(scene0[1]["layerInfo"].IsObject());
    EXPECT_TRUE(scene0[1]["layerInfo"].HasMember("minificationFactor"));
    EXPECT_EQ(scene0[1]["layerInfo"]["minificationFactor"].GetInt(), 2);
    EXPECT_TRUE(scene0[1]["layerInfo"].HasMember("pyramidLayerNo"));
    EXPECT_EQ(scene0[1]["layerInfo"]["pyramidLayerNo"].GetInt(), 1);
    EXPECT_TRUE(scene0[1].HasMember("count"));
    EXPECT_EQ(scene0[1]["count"].GetInt(), 4);

    // Check the second scene's pyramid statistics
    EXPECT_TRUE(scenePyramidStatistics.HasMember("1"));
    const auto& scene1 = scenePyramidStatistics["1"];
    EXPECT_TRUE(scene1.IsArray());
    EXPECT_EQ(scene1.Size(), 3);

    EXPECT_TRUE(scene1[0].IsObject());
    EXPECT_TRUE(scene1[0].HasMember("layerInfo"));
    EXPECT_TRUE(scene1[0]["layerInfo"].IsObject());
    EXPECT_TRUE(scene1[0]["layerInfo"].HasMember("minificationFactor"));
    EXPECT_EQ(scene1[0]["layerInfo"]["minificationFactor"].GetInt(), 2);
    EXPECT_TRUE(scene1[0]["layerInfo"].HasMember("pyramidLayerNo"));
    EXPECT_EQ(scene1[0]["layerInfo"]["pyramidLayerNo"].GetInt(), 0);
    EXPECT_TRUE(scene1[0].HasMember("count"));
    EXPECT_EQ(scene1[0]["count"].GetInt(), 20);

    EXPECT_TRUE(scene1[1].IsObject());
    EXPECT_TRUE(scene1[1].HasMember("layerInfo"));
    EXPECT_TRUE(scene1[1]["layerInfo"].IsObject());
    EXPECT_TRUE(scene1[1]["layerInfo"].HasMember("minificationFactor"));
    EXPECT_EQ(scene1[1]["layerInfo"]["minificationFactor"].GetInt(), 2);
    EXPECT_TRUE(scene1[1]["layerInfo"].HasMember("pyramidLayerNo"));
    EXPECT_EQ(scene1[1]["layerInfo"]["pyramidLayerNo"].GetInt(), 1);
    EXPECT_TRUE(scene1[1].HasMember("count"));
    EXPECT_EQ(scene1[1]["count"].GetInt(), 6);

    EXPECT_TRUE(scene1[2].IsObject());
    EXPECT_TRUE(scene1[2].HasMember("layerInfo"));
    EXPECT_TRUE(scene1[2]["layerInfo"].IsObject());
    EXPECT_TRUE(scene1[2]["layerInfo"].HasMember("minificationFactor"));
    EXPECT_EQ(scene1[2]["layerInfo"]["minificationFactor"].GetInt(), 2);
    EXPECT_TRUE(scene1[2]["layerInfo"].HasMember("pyramidLayerNo"));
    EXPECT_EQ(scene1[2]["layerInfo"]["pyramidLayerNo"].GetInt(), 2);
    EXPECT_TRUE(scene1[2].HasMember("count"));
    EXPECT_EQ(scene1[2]["count"].GetInt(), 1);
}

TEST(CZIAPI_ParameterHelpers, Check_TryParseCZIWriterOptions_with_invalid_input)
{
    libCZI::CZIWriterOptions czi_writer_options;
    bool b = ParameterHelpers::TryParseCZIWriterOptions("this is not a valid json string", czi_writer_options);
    EXPECT_FALSE(b);
    b = ParameterHelpers::TryParseCZIWriterOptions("", czi_writer_options);
    EXPECT_FALSE(b);
    b = ParameterHelpers::TryParseCZIWriterOptions(nullptr, czi_writer_options);
    EXPECT_FALSE(b);
    b = ParameterHelpers::TryParseCZIWriterOptions("{\"allow_duplicate_subblocks\":tru}", czi_writer_options);
    EXPECT_FALSE(b);
    b = ParameterHelpers::TryParseCZIWriterOptions(R"({"allow_duplicate_subblocks":"u"})", czi_writer_options);
    EXPECT_FALSE(b);
    b = ParameterHelpers::TryParseCZIWriterOptions("{\"allow_duplicate_subblocks\":-42}", czi_writer_options);
    EXPECT_FALSE(b);
    b = ParameterHelpers::TryParseCZIWriterOptions("{\"allow_duplicate_subblocks\":0}", czi_writer_options);
    EXPECT_FALSE(b);
}

TEST(CZIAPI_ParameterHelpers, Check_TryParseCZIWriterOptions_and_check_result)
{
    libCZI::CZIWriterOptions czi_writer_options;
    czi_writer_options.allow_duplicate_subblocks = false;
    bool b = ParameterHelpers::TryParseCZIWriterOptions("{\"allow_duplicate_subblocks\":true}", czi_writer_options);
    EXPECT_TRUE(b);
    EXPECT_TRUE(czi_writer_options.allow_duplicate_subblocks);

    b = ParameterHelpers::TryParseCZIWriterOptions("{\"allow_duplicate_subblocks\":false}", czi_writer_options);
    EXPECT_TRUE(b);
    EXPECT_FALSE(czi_writer_options.allow_duplicate_subblocks);

    // this should work, and the additional field "abc" should be ignored
    b = ParameterHelpers::TryParseCZIWriterOptions(R"({"allow_duplicate_subblocks":false, "abc":"xyz"})", czi_writer_options);
    EXPECT_TRUE(b);
    EXPECT_FALSE(czi_writer_options.allow_duplicate_subblocks);
}

TEST(CZIAPI_ParameterHelpers, Check_TryParseCZIWriterInfo_with_invalid_input)
{
    std::shared_ptr<libCZI::ICziWriterInfo> czi_writer_info;
    bool b = ParameterHelpers::TryParseCZIWriterInfo("this is not a valid json string", czi_writer_info);
    EXPECT_FALSE(b);
    EXPECT_FALSE(czi_writer_info);
    b = ParameterHelpers::TryParseCZIWriterInfo("", czi_writer_info);
    EXPECT_FALSE(b);
    EXPECT_FALSE(czi_writer_info);
    b = ParameterHelpers::TryParseCZIWriterInfo(nullptr, czi_writer_info);
    EXPECT_FALSE(b);
    EXPECT_FALSE(czi_writer_info);
}

TEST(CZIAPI_ParameterHelpers, Check_TryParseCZIWriterInfo_and_check_result)
{
    std::shared_ptr<libCZI::ICziWriterInfo> czi_writer_info;
    bool b = ParameterHelpers::TryParseCZIWriterInfo(R"({"file_guid":"{6A138A22-46A0-4749-9C33-ED0363D29B28}"})", czi_writer_info);
    EXPECT_TRUE(b);
    EXPECT_TRUE(czi_writer_info);
    EXPECT_TRUE(czi_writer_info->GetFileGuid().Data1 == 0x6A138A22 &&
                    czi_writer_info->GetFileGuid().Data2 == 0x46A0 &&
                    czi_writer_info->GetFileGuid().Data3 == 0x4749 &&
                    czi_writer_info->GetFileGuid().Data4[0] == 0x9C && czi_writer_info->GetFileGuid().Data4[1] == 0x33 &&
                    czi_writer_info->GetFileGuid().Data4[2] == 0xED && czi_writer_info->GetFileGuid().Data4[3] == 0x03 &&
                    czi_writer_info->GetFileGuid().Data4[4] == 0x63 && czi_writer_info->GetFileGuid().Data4[5] == 0xD2 &&
                    czi_writer_info->GetFileGuid().Data4[6] == 0x9B && czi_writer_info->GetFileGuid().Data4[7] == 0x28);
    czi_writer_info.reset();

    b = ParameterHelpers::TryParseCZIWriterInfo(R"({"file_guid":"{6A138A22-46A0-4749-9C33-ED0363D29B28}","reserved_size_attachments_directory":101,"reserved_size_subblock_directory":102,"reserved_size_metadata_segment":103})", czi_writer_info);
    EXPECT_TRUE(b);
    EXPECT_TRUE(czi_writer_info);
    size_t size_attachments_directory;
    EXPECT_TRUE(czi_writer_info->TryGetReservedSizeForAttachmentDirectory(&size_attachments_directory));
    EXPECT_EQ(size_attachments_directory, 101);
    size_t size_subblocks_directory;
    EXPECT_TRUE(czi_writer_info->TryGetReservedSizeForSubBlockDirectory(&size_subblocks_directory));
    EXPECT_EQ(size_subblocks_directory, 102);
    size_t size_metadata_segment;
    EXPECT_TRUE(czi_writer_info->TryGetReservedSizeForMetadataSegment(&size_metadata_segment));
    EXPECT_EQ(size_metadata_segment, 103);

    b = ParameterHelpers::TryParseCZIWriterInfo(
        R"({"file_guid":"{6A138A22-46A0-4749-9C33-ED0363D29B28}",)"
        R"("reserved_size_attachments_directory":101,)"
        R"("reserved_size_subblock_directory":102,)"
        R"("reserved_size_metadata_segment":103,)"
        R"("minimum_m_index":0,)"
        R"("maximum_m_index":55})",
        czi_writer_info);
    EXPECT_TRUE(b);
    EXPECT_TRUE(czi_writer_info);
    size_attachments_directory = 0;
    EXPECT_TRUE(czi_writer_info->TryGetReservedSizeForAttachmentDirectory(&size_attachments_directory));
    EXPECT_EQ(size_attachments_directory, 101);
    size_subblocks_directory = 0;
    EXPECT_TRUE(czi_writer_info->TryGetReservedSizeForSubBlockDirectory(&size_subblocks_directory));
    EXPECT_EQ(size_subblocks_directory, 102);
    size_metadata_segment = 0;
    EXPECT_TRUE(czi_writer_info->TryGetReservedSizeForMetadataSegment(&size_metadata_segment));
    EXPECT_EQ(size_metadata_segment, 103);
    int min_m_index, max_m_index;
    EXPECT_TRUE(czi_writer_info->TryGetMIndexMinMax(&min_m_index, &max_m_index));
    EXPECT_EQ(0, min_m_index);
    EXPECT_EQ(55, max_m_index);
}

TEST(CZIAPI_ParameterHelpers, Check_FormatGeneralDocumentInfoAsJson)
{
    libCZI::GeneralDocumentInfo general_document_info;
    general_document_info.Clear();
    general_document_info.SetName(L"Test-Name");
    general_document_info.SetTitle(L"Test-Description");
    general_document_info.SetUserName(L"Test-UserName");
    general_document_info.SetDescription(L"Test-Description");
    general_document_info.SetComment(L"Test-Comment");
    general_document_info.SetKeywords(L"Test-Keywords");
    general_document_info.SetRating(3);
    general_document_info.SetCreationDate(L"2025-01-01T12:00:00Z");

    const std::string json_string = ParameterHelpers::FormatGeneralDocumentInfoAsJsonString(general_document_info);

    static const char* json_schema = R"(
{
"$schema": "http://json-schema.org/draft-07/schema#",
"title": "ExampleObject",
"type": "object",
"properties": {
    "name": {
        "type": "string"
    },
    "title": {
        "type": "string"
    },
    "user_name": {
        "type": "string"
    },
    "description": {
        "type": "string"
    },
    "comment": {
        "type": "string"
    },
    "keywords": {
        "type": "string"
    },
    "creation_date_time": {
        "type": "string",
        "format": "date-time"
    },
    "rating": {
        "type": "integer"
    }
    },
    "required": [
        "name",
        "title",
        "user_name",
        "description",
        "comment",
        "keywords",
        "creation_date_time",
        "rating"
    ],
    "additionalProperties": false
}
)";

    rapidjson::Document schema_document;
    schema_document.Parse(json_schema);
    ASSERT_FALSE(schema_document.HasParseError()) << "Schema JSON parse error: " << schema_document.GetParseError();

    rapidjson::SchemaDocument schema(schema_document);
    rapidjson::SchemaValidator validator(schema);

    rapidjson::Document json_document;
    json_document.Parse(json_string.c_str());
    ASSERT_FALSE(json_document.HasParseError()) << "JSON parse error: " << json_document.GetParseError();

    ASSERT_TRUE(json_document.Accept(validator)) << "JSON does not conform to schema";
    ASSERT_TRUE(validator.IsValid()) << "JSON is invalid according to schema";
}

namespace
{
    rapidjson::Document CreateSchemaDocumentForZDimensionInfo()
    {
        static const char* json_schema = R"(
{
    "$schema": "http://json-schema.org/draft-07/schema#",
    "title": "ZDimensionInfo",
    "type": "object",
    "properties": {
        "reference_position": {
            "type": "number"
    },
    "xyz_handedness": {
        "type": "string",
        "enum": ["left_handed", "right_handed", "undefined"]
    },
    "z_axis_direction": {
        "type": "string",
        "enum": ["from_specimen_to_objective", "from_objective_to_specimen", "undefined"]
    },
    "z_drive_mode": {
        "type": "string",
        "enum": ["continuous", "step"]
    },
    "z_drive_speed": {
        "type": "number"
    },
    "interval_definition": {
        "type": "array",
        "items": {
            "type": "number"
        },
        "minItems": 2,
        "maxItems": 2
    },
    "position_list": {
        "type": "array",
        "items": {
            "type": "number"
        },
        "minItems": 1
    }
},
    "required": [
        "reference_position",
        "xyz_handedness",
        "z_axis_direction",
        "z_drive_mode",
        "z_drive_speed"
    ],
    "allOf": [
    {
        "not": {
            "required": ["interval_definition", "position_list"]
        }
    }
    ],
    "additionalProperties": false
}
)";

        rapidjson::Document schema_document;
        schema_document.Parse(json_schema);
        return schema_document;
    }

    rapidjson::Document CreateSchemaDocumentForTDimensionInfo()
    {
        static const char* json_schema = R"(
{
    "$schema": "http://json-schema.org/draft-07/schema#",
    "title": "TDimensionInfo",
    "type": "object",
    "properties": {
        "start_time": {
            "type": "string",
            "format": "date-time"
        },
        "interval_definition": {
            "type": "array",
        "items": {
            "type": "number"
        },
        "minItems": 2,
        "maxItems": 2
    },
    "offset_list": {
        "type": "array",
        "items": {
            "type": "number"
        },
        "minItems": 1
    }
},
    "allOf": [
    {
        "not": {
            "required": ["interval_definition", "offset_list"]
        }
    }
    ],
"additionalProperties": false
}
)";

        rapidjson::Document schema_document;
        schema_document.Parse(json_schema);
        return schema_document;
    }

    rapidjson::Document CreateSchemaDocumentForCDimensionInfo()
    {
        static const char* json_schema = R"(
{
    "$schema": "http://json-schema.org/draft-07/schema#",
    "type": "object",
    "patternProperties": {
    "^[0-9]+$": {
    "type": "object",
    "properties": {
        "attribute_id": {
            "type": "string"
        },
        "attribute_name": {
            "type": "string"
        },
        "channel_type": {
            "type": "string",
            "enum": ["Heightmap","PalHR","PalWidefield","SimHR","SimWidefield","SimDWF","AiryScanSum","AiryScanRawSr","AiryScanRaw","AiryScanSr","AiryScanVp","AiryScanMb","AiryScanRingSheppardSum","OnlineUnmixing","Unspecified"]
        },
        "channel_unit": {
            "type": "string"
        },
        "pixel_type": {
            "type": "string",
            "enum": ["Gray8","Gray16","Gray32Float","Bgr24","Bgr48","Bgr96Float","Bgra32","Gray64ComplexFloat","Bgr192ComplexFloat","Gray32","Gray64Float"]
        },
        "component_bit_count": {
            "type": "integer"
        },
        "acquisition_mode": {
            "type": "string",
            "enum": ["WideField","LaserScanningConfocalMicroscopy","SpinningDiskConfocal","SlitScanConfocal","MultiPhotonMicroscopy","StructuredIllumination","SingleMoleculeImaging","TotalInternalReflection","FluorescenceLifetime","SpectralImaging","FluorescenceCorrelationSpectroscopy","NearFieldScanningOpticalMicroscopy","SecondHarmonicGenerationImaging","PALM","STORM","STED","TIRF","FSM","LCM","SPIM","SEM","FIB","FIB_SEM","ApertureCorrelation","Other"]
        },
        "illumination_type": {
            "type": "string",
            "enum": ["Transmitted","Epifluorescence","Oblique","NonLinear","Other"]
        },
        "contrast_method": {
            "type": "string",
            "enum": ["Brightfield","Phase","DIC","HoffmanModulation","ObliqueIllumination","PolarizedLight","Darkfield","Fluorescence","MultiPhotonFluorescence","Other"]
        },
        "illumination_wavelength": {
            "type": "object",
            "properties": {
            "type": {
                "type": "string",
                "enum": ["Ranges"]
            },
            "ranges": {
                "type": "string"
            }
            },
            "required": ["type", "ranges"]
        },
        "detection_wavelength": {
            "type": "object",
            "properties": {
            "type": {
                "type": "string",
                "enum": ["Ranges"]
            },
            "ranges": {
                "type": "string"
            }
            },
            "required": ["type", "ranges"]
        },
        "excitation_wavelength": {
            "type": "number"
        },
        "emission_wavelength": {
            "type": "number"
        },
        "effective_na": {
            "type": "number"
        },
        "dye_id": {
            "type": "string"
        },
        "dye_database_id": {
            "type": "string"
        },
        "pinhole_size": {
            "type": "number"
        },
        "pinhole_size_airy": {
            "type": "number"
        },
        "pinhole_geometry": {
            "type": "string",
            "enum": ["Circular","Rectangular","Other"]
        },
        "fluor": {
            "type": "string"
        },
        "nd_filter": {
            "type": "number"
        },
        "pocket_cell_setting": {
            "type": "integer"
        },
        "color": {
            "type": "string"
        },
        "exposure_time": {
            "type": "string"
        },
        "depth_of_focus": {
            "type": "number"
        },
        "section_thickness": {
            "type": "number"
        },
        "reflector": {
            "type": "string"
        },
        "condensor_contrast": {
            "type": "string"
        },
        "na_condensor": {
            "type": "number"
        }
    },
    "required": [
        "attribute_id",
        "attribute_name",
        "channel_type",
        "channel_unit",
        "pixel_type",
        "component_bit_count",
        "acquisition_mode",
        "illumination_type",
        "contrast_method",
        "illumination_wavelength",
        "detection_wavelength",
        "excitation_wavelength",
        "emission_wavelength",
        "effective_na",
        "dye_id",
        "dye_database_id",
        "pinhole_size",
        "pinhole_size_airy",
        "pinhole_geometry",
        "fluor",
        "nd_filter",
        "pocket_cell_setting",
        "color",
        "exposure_time",
        "depth_of_focus",
        "section_thickness",
        "reflector",
        "condensor_contrast",
        "na_condensor"
        ]
    }
    },
    "additionalProperties": false
}
)";

        rapidjson::Document schema_document;
        schema_document.Parse(json_schema);
        return schema_document;
    }
}

TEST(CZIAPI_ParameterHelpers, Check_FormatDimensionZInfoAsJson_Scenario1)
{
    CCziDimensionZInfo dimension_z_info;
    dimension_z_info.SetZDriveSpeed(1.23);
    dimension_z_info.SetZDriveMode(libCZI::IDimensionZInfo::ZDriveMode::Continuous);
    dimension_z_info.SetZAxisDirection(libCZI::IDimensionZInfo::ZaxisDirection::FromSpecimenToObjective);
    dimension_z_info.SetXyzHandedness(libCZI::IDimensionZInfo::XyzHandedness::LeftHanded);
    dimension_z_info.SetStartPosition(3.45);
    dimension_z_info.SetIntervalDefinition(0, 1.1);

    const std::string json_string = ParameterHelpers::FormatZDimensionInfoAsJsonString(&dimension_z_info);

    rapidjson::SchemaDocument schema(CreateSchemaDocumentForZDimensionInfo());
    rapidjson::SchemaValidator validator(schema);

    rapidjson::Document json_document;
    json_document.Parse(json_string.c_str());
    ASSERT_FALSE(json_document.HasParseError()) << "JSON parse error: " << json_document.GetParseError();

    ASSERT_TRUE(json_document.Accept(validator)) << "JSON does not conform to schema";
    ASSERT_TRUE(validator.IsValid()) << "JSON is invalid according to schema";
}

TEST(CZIAPI_ParameterHelpers, Check_FormatDimensionZInfoAsJson_Scenario2)
{
    CCziDimensionZInfo dimension_z_info;
    dimension_z_info.SetZDriveSpeed(0.23);
    dimension_z_info.SetZDriveMode(libCZI::IDimensionZInfo::ZDriveMode::Step);
    dimension_z_info.SetZAxisDirection(libCZI::IDimensionZInfo::ZaxisDirection::FromObjectiveToSpecimen);
    dimension_z_info.SetXyzHandedness(libCZI::IDimensionZInfo::XyzHandedness::RightHanded);
    dimension_z_info.SetStartPosition(2.45);
    dimension_z_info.SetListDefinition({ 1.1, 2.2, 3.3, 5.2, 4.1, .4, 8.01 });

    const std::string json_string = ParameterHelpers::FormatZDimensionInfoAsJsonString(&dimension_z_info);

    rapidjson::SchemaDocument schema(CreateSchemaDocumentForZDimensionInfo());
    rapidjson::SchemaValidator validator(schema);

    rapidjson::Document json_document;
    json_document.Parse(json_string.c_str());
    ASSERT_FALSE(json_document.HasParseError()) << "JSON parse error: " << json_document.GetParseError();

    ASSERT_TRUE(json_document.Accept(validator)) << "JSON does not conform to schema";
    ASSERT_TRUE(validator.IsValid()) << "JSON is invalid according to schema";
}

TEST(CZIAPI_ParameterHelpers, Check_FormatDimensionTInfoAsJson_Scenario1)
{
    CCziDimensionTInfo dimension_t_info;
    libCZI::XmlDateTime start_time;
    ASSERT_TRUE(start_time.TryParse("2025-03-02T13:20:01Z", &start_time));
    dimension_t_info.SetStartTime(start_time);
    dimension_t_info.SetIntervalDefinition(1.12, 1.03);

    const std::string json_string = ParameterHelpers::FormatTDimensionInfoAsJsonString(&dimension_t_info);

    rapidjson::SchemaDocument schema(CreateSchemaDocumentForTDimensionInfo());
    rapidjson::SchemaValidator validator(schema);

    rapidjson::Document json_document;
    json_document.Parse(json_string.c_str());
    ASSERT_FALSE(json_document.HasParseError()) << "JSON parse error: " << json_document.GetParseError();

    ASSERT_TRUE(json_document.Accept(validator)) << "JSON does not conform to schema";
    ASSERT_TRUE(validator.IsValid()) << "JSON is invalid according to schema";
}

TEST(CZIAPI_ParameterHelpers, Check_FormatDimensionTInfoAsJson_Scenario2)
{
    CCziDimensionTInfo dimension_t_info;
    libCZI::XmlDateTime start_time;
    ASSERT_TRUE(start_time.TryParse("2025-03-02T13:20:01Z", &start_time));
    dimension_t_info.SetStartTime(start_time);
    dimension_t_info.SetListDefinition({ 1.12, 1.03, 0.4, 3.4, 8.12 });

    const std::string json_string = ParameterHelpers::FormatTDimensionInfoAsJsonString(&dimension_t_info);

    rapidjson::SchemaDocument schema(CreateSchemaDocumentForTDimensionInfo());
    rapidjson::SchemaValidator validator(schema);

    rapidjson::Document json_document;
    json_document.Parse(json_string.c_str());
    ASSERT_FALSE(json_document.HasParseError()) << "JSON parse error: " << json_document.GetParseError();

    ASSERT_TRUE(json_document.Accept(validator)) << "JSON does not conform to schema";
    ASSERT_TRUE(validator.IsValid()) << "JSON is invalid according to schema";
}

TEST(CZIAPI_ParameterHelpers, Check_FormatDimensionCInfoAsJson_Scenario1)
{
    libCZI::pugi::xml_document doc;
    auto dimension_channel_info_1 = make_shared<CDimensionChannelInfo>();
    wstring attribute_id(L"ID");
    wstring attribute_name(L"NAME");
    dimension_channel_info_1->SetAttributeId(&attribute_id);
    dimension_channel_info_1->SetAttributeName(&attribute_name);
    dimension_channel_info_1->SetChannelType(libCZI::DimensionChannelChannelType::PalWidefield);
    dimension_channel_info_1->SetChannelUnit(L"channel-unit");
    dimension_channel_info_1->SetPixelType(libCZI::PixelType::Gray8);
    dimension_channel_info_1->SetComponentBitCount(8);
    dimension_channel_info_1->SetAcquisitionMode(libCZI::DimensionChannelAcquisitionMode::WideField);
    dimension_channel_info_1->SetIlluminationType(libCZI::DimensionChannelIlluminationType::Epifluorescence);
    dimension_channel_info_1->SetContrastMethod(libCZI::DimensionChannelContrastMethod::Brightfield);
    libCZI::SpectrumCharacteristics spectrum_characteristics_illumination;
    spectrum_characteristics_illumination.type = libCZI::SpectrumCharacteristics::InformationType::Ranges;
    spectrum_characteristics_illumination.ranges.push_back(libCZI::RangeOrSingleValue<double>{ false, 1.0, 2.0 });
    spectrum_characteristics_illumination.ranges.push_back(libCZI::RangeOrSingleValue<double>{ false, 4.2, 6.2 });
    spectrum_characteristics_illumination.ranges.push_back(libCZI::RangeOrSingleValue<double>{ true, 8, 0});
    dimension_channel_info_1->SetIlluminationWavelength(spectrum_characteristics_illumination);
    libCZI::SpectrumCharacteristics spectrum_characteristics_detection;
    spectrum_characteristics_detection.type = libCZI::SpectrumCharacteristics::InformationType::Ranges;
    spectrum_characteristics_detection.ranges.push_back(libCZI::RangeOrSingleValue<double>{ false, 5.0, 7.0 });
    spectrum_characteristics_detection.ranges.push_back(libCZI::RangeOrSingleValue<double>{ false, 9.2, 9.8 });
    spectrum_characteristics_detection.ranges.push_back(libCZI::RangeOrSingleValue<double>{ true, 11.1, 0});
    dimension_channel_info_1->SetDetectionWavelength(spectrum_characteristics_detection);
    dimension_channel_info_1->SetExcitationWavelength(11.23);
    dimension_channel_info_1->SetEmissionWavelength(14.3);

    dimension_channel_info_1->SetEffectiveNA(78.3);
    dimension_channel_info_1->SetDyeId(L"Dye-Id");
    dimension_channel_info_1->SetDyeDatabaseId(L"Dye-Database-Id");
    dimension_channel_info_1->SetPinholeSize(7.4);
    dimension_channel_info_1->SetPinholeSizeAiry(41.2);
    dimension_channel_info_1->SetPinholeGeometry(libCZI::DimensionChannelPinholeGeometry::Rectangular);
    dimension_channel_info_1->SetFluor(L"fluor");
    dimension_channel_info_1->SetNDFilter(2.41);
    dimension_channel_info_1->SetPockelCellSetting(2);
    dimension_channel_info_1->SetColor(libCZI::Rgb8Color{ 1,2,3 });

    dimension_channel_info_1->SetExposureTime(libCZI::RangeOrSingleValue<std::uint64_t>{false, 10, 14});
    dimension_channel_info_1->SetDepthOfFocus(7.4);
    dimension_channel_info_1->SetSectionThickness(3.1);
    dimension_channel_info_1->SetReflector(L"reflector");
    dimension_channel_info_1->SetCondenserContrast(L"condensorContrast");
    dimension_channel_info_1->SetNACondenser(4.21);

    auto dimension_channel_info_2 = make_shared<CDimensionChannelInfo>();
    attribute_id = L"ID2";
    attribute_name = L"NAME2";
    dimension_channel_info_2->SetAttributeId(&attribute_id);
    dimension_channel_info_2->SetAttributeName(&attribute_name);
    dimension_channel_info_2->SetChannelType(libCZI::DimensionChannelChannelType::SimDWF);
    dimension_channel_info_2->SetChannelUnit(L"channel-unit");
    dimension_channel_info_2->SetPixelType(libCZI::PixelType::Gray16);
    dimension_channel_info_2->SetComponentBitCount(8);
    dimension_channel_info_2->SetAcquisitionMode(libCZI::DimensionChannelAcquisitionMode::LaserScanningConfocalMicroscopy);
    dimension_channel_info_2->SetIlluminationType(libCZI::DimensionChannelIlluminationType::Oblique);
    dimension_channel_info_2->SetContrastMethod(libCZI::DimensionChannelContrastMethod::HoffmanModulation);
    spectrum_characteristics_illumination.type = libCZI::SpectrumCharacteristics::InformationType::Ranges;
    spectrum_characteristics_illumination.ranges.push_back(libCZI::RangeOrSingleValue<double>{ true, 1.0, 0 });
    spectrum_characteristics_illumination.ranges.push_back(libCZI::RangeOrSingleValue<double>{ false, 2.2, 6.2 });
    spectrum_characteristics_illumination.ranges.push_back(libCZI::RangeOrSingleValue<double>{ true, 8, 0});
    dimension_channel_info_2->SetIlluminationWavelength(spectrum_characteristics_illumination);
    spectrum_characteristics_detection.type = libCZI::SpectrumCharacteristics::InformationType::Ranges;
    spectrum_characteristics_detection.ranges.push_back(libCZI::RangeOrSingleValue<double>{ false, 5.0, 7.0 });
    spectrum_characteristics_detection.ranges.push_back(libCZI::RangeOrSingleValue<double>{ false, 9.2, 9.8 });
    spectrum_characteristics_detection.ranges.push_back(libCZI::RangeOrSingleValue<double>{ true, 11.1, 0});
    dimension_channel_info_2->SetDetectionWavelength(spectrum_characteristics_detection);
    dimension_channel_info_2->SetExcitationWavelength(11.23);
    dimension_channel_info_2->SetEmissionWavelength(14.3);

    dimension_channel_info_2->SetEffectiveNA(78.3);
    dimension_channel_info_2->SetDyeId(L"Dye-Id");
    dimension_channel_info_2->SetDyeDatabaseId(L"Dye-Database-Id");
    dimension_channel_info_2->SetPinholeSize(7.4);
    dimension_channel_info_2->SetPinholeSizeAiry(41.2);
    dimension_channel_info_2->SetPinholeGeometry(libCZI::DimensionChannelPinholeGeometry::Circular);
    dimension_channel_info_2->SetFluor(L"fluor");
    dimension_channel_info_2->SetNDFilter(2.41);
    dimension_channel_info_2->SetPockelCellSetting(2);
    dimension_channel_info_2->SetColor(libCZI::Rgb8Color{ 4,5,7 });

    dimension_channel_info_2->SetExposureTime(libCZI::RangeOrSingleValue<std::uint64_t>{true, 11, 0});
    dimension_channel_info_2->SetDepthOfFocus(7.4);
    dimension_channel_info_2->SetSectionThickness(3.1);
    dimension_channel_info_2->SetReflector(L"reflector");
    dimension_channel_info_2->SetCondenserContrast(L"condensorContrast");
    dimension_channel_info_2->SetNACondenser(4.21);

    CDimensionsChannelsInfo dimension_c_info;
    dimension_c_info.AddChannel(dimension_channel_info_1);
    dimension_c_info.AddChannel(dimension_channel_info_2);

    const std::string json_string = ParameterHelpers::FormatCDimensionInfoAsJsonString(&dimension_c_info);

    rapidjson::SchemaDocument schema(CreateSchemaDocumentForCDimensionInfo());
    rapidjson::SchemaValidator validator(schema);

    rapidjson::Document json_document;
    json_document.Parse(json_string.c_str());
    ASSERT_FALSE(json_document.HasParseError()) << "JSON parse error: " << json_document.GetParseError();

    const bool json_validates_against_schema = json_document.Accept(validator);
    if (!json_validates_against_schema)
    {
        // The story is: older versions of RapidJSON had a partial or incorrect implementation of the JSON schema validator,
        // in our case the "additionalProperties" keyword was not implemented correctly. To make things worse, the version macros
        // are not set correctly in the RapidJSON library, so we cannot check the version at compile time. The version remained statically
        // set to 1.1.0 since at least 2016.
        // To work around this problem - we check whether the error we get here is "of the kind we expect", and if so, we ignore it.
        rapidjson::StringBuffer buffer;
        validator.GetInvalidSchemaPointer().StringifyUriFragment(buffer);
        const string invalid_schema = buffer.GetString();
        const string invalid_keyword = validator.GetInvalidSchemaKeyword();
        buffer.Clear();
        validator.GetInvalidDocumentPointer().StringifyUriFragment(buffer);
        const string invalid_document = buffer.GetString();
        if (invalid_schema=="#" && invalid_keyword=="additionalProperties" && invalid_document=="#/0/attribute_id")
        {
            // this is the error we expect, so we ignore it
            GTEST_LOG_(INFO) << "Ignoring validation error, this is a known issue with old RapidJSON-versions.";
        }
        else
        {
            FAIL() <<  "JSON does not conform to schema - invalid schema: \"" << invalid_schema
                    << "\" invalid keyword: \"" << invalid_keyword
                    << "\" invalid document: \"" << buffer.GetString()<<"\"";
        }
    }
    else
    {
        ASSERT_TRUE(validator.IsValid()) << "JSON is invalid according to schema";
    }
}
