// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"
#include "MockMetadataSegment.h"
#include "utils.h"
#include <codecvt>
#include <locale>
#include <string>
#include "MemOutputStream.h"

using namespace libCZI;
using namespace std;

TEST(DisplaySettings, Test1)
{
    auto mockMdSegment = make_shared<MockMetadataSegment>();
    auto md = CreateMetaFromMetadataSegment(mockMdSegment.get());
    EXPECT_TRUE(md->IsXmlValid()) << "Expected valid XML.";
    auto docInfo = md->GetDocumentInfo();
    auto displaySettings = docInfo->GetDisplaySettings();

    DisplaySettingsPOD pod;
    IDisplaySettings::Clone(displaySettings.get(), pod);

    EXPECT_TRUE(pod.channelDisplaySettings.size() == 5) << "Expected to have a size of 5.";

    for (size_t i = 0; i < pod.channelDisplaySettings.size(); ++i)
    {
        EXPECT_TRUE(pod.channelDisplaySettings[i].isEnabled == true) << "Expected the channel to be enabled";
        EXPECT_TRUE(pod.channelDisplaySettings[i].tintingMode == IDisplaySettings::TintingMode::Color) << "Expected the tinting mode to be 'Color'";
        EXPECT_TRUE(pod.channelDisplaySettings[i].gradationCurveMode == IDisplaySettings::GradationCurveMode::Linear) << "Expected the gradation-curve-mode to be 'Linear'";
    }
}

TEST(DisplaySettings, Test2)
{
    auto mockMdSegment = make_shared<MockMetadataSegment>();
    auto md = CreateMetaFromMetadataSegment(mockMdSegment.get());
    EXPECT_TRUE(md->IsXmlValid()) << "Expected valid XML.";
    auto docInfo = md->GetDocumentInfo();
    auto displaySettings = docInfo->GetDisplaySettings();

    DisplaySettingsPOD pod;
    IDisplaySettings::Clone(displaySettings.get(), pod);

    auto displaySettings2 = DisplaySettingsPOD::CreateIDisplaySettingSp(pod);

    auto chDs1 = displaySettings->GetChannelDisplaySettings(0);
    auto chDs2 = displaySettings2->GetChannelDisplaySettings(0);

    EXPECT_TRUE(chDs1->GetIsEnabled() == chDs2->GetIsEnabled()) << "Expected to have the same value.";
    EXPECT_TRUE(chDs1->GetWeight() == chDs2->GetWeight()) << "Expected to have the same value.";
    float bp1, bp2, wp1, wp2;
    chDs1->GetBlackWhitePoint(&bp1, &wp1);
    chDs2->GetBlackWhitePoint(&bp2, &wp2);
    EXPECT_TRUE(bp1 == bp2 && wp1 == wp2) << "Expected to have the same value.";
}

TEST(DisplaySettings, Test3)
{
    auto mockMdSegment = make_shared<MockMetadataSegment>(MockMetadataSegment::Type::Data2);
    auto md = CreateMetaFromMetadataSegment(mockMdSegment.get());
    ASSERT_TRUE(md->IsXmlValid()) << "Expected valid XML.";
    auto docInfo = md->GetDocumentInfo();
    auto displaySettings = docInfo->GetDisplaySettings();

    DisplaySettingsPOD pod;
    IDisplaySettings::Clone(displaySettings.get(), pod);

    auto displaySettings2 = DisplaySettingsPOD::CreateIDisplaySettingSp(pod);

    auto chDs1 = displaySettings->GetChannelDisplaySettings(1);
    auto chDs2 = displaySettings2->GetChannelDisplaySettings(1);

    EXPECT_TRUE(chDs1->GetIsEnabled() == chDs2->GetIsEnabled()) << "Expected to have the same value.";
    EXPECT_TRUE(chDs1->GetWeight() == chDs2->GetWeight()) << "Expected to have the same value.";
    float bp1, bp2, wp1, wp2;
    chDs1->GetBlackWhitePoint(&bp1, &wp1);
    chDs2->GetBlackWhitePoint(&bp2, &wp2);
    EXPECT_TRUE(bp1 == bp2 && wp1 == wp2) << "Expected to have the same value.";

    EXPECT_TRUE(chDs1->GetGradationCurveMode() == IDisplaySettings::GradationCurveMode::Spline &&
        chDs2->GetGradationCurveMode() == IDisplaySettings::GradationCurveMode::Spline) << "Expected to have the same value (=Spline).";

    vector< IDisplaySettings::SplineControlPoint> splineCtrlPts1, splineCtrlPts2;
    ASSERT_TRUE(chDs1->TryGetSplineControlPoints(&splineCtrlPts1) && chDs1->TryGetSplineControlPoints(&splineCtrlPts2)) << "Expected to find spline-control-points";
    EXPECT_THAT(splineCtrlPts1, ::testing::ContainerEq(splineCtrlPts2)) << "The data should have been equal";
}

TEST(DisplaySettings, WriteDisplaySettingsToDocumentAndReadFromThereAndCompare)
{
    // what happens here:
    // - we are creating a simple 2-channel-CZI-document, add two subblocks 
    // - and, we construct "display-settings" for the document, write them into the CZI-document
    // - then we open the CZI-document
    // - and read the display-settings from it
    // - and, finally, compare them to what we put into
    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);
    //auto outStream = CreateOutputStreamForFile(L"D:\\libczi_displaysettings.czi", true);

    auto spWriterInfo = make_shared<CCziWriterInfo >(
        GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } },
        CDimBounds{ { { DimensionIndex::C,0,2 } } });	// set a bounds for  C

    writer->Create(outStream, spWriterInfo);

    // now add two subblocks (does not really matter, though)
    auto bitmap = CreateTestBitmap(PixelType::Gray8, 64, 64);

    ScopedBitmapLockerSP lockBm{ bitmap };
    AddSubBlockInfoStridedBitmap addSbBlkInfo;
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate = CDimCoordinate::Parse("C0");
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = 0;
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = bitmap->GetWidth();
    addSbBlkInfo.logicalHeight = bitmap->GetHeight();
    addSbBlkInfo.physicalWidth = bitmap->GetWidth();
    addSbBlkInfo.physicalHeight = bitmap->GetHeight();
    addSbBlkInfo.PixelType = bitmap->GetPixelType();
    addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
    addSbBlkInfo.strideBitmap = lockBm.stride;
    writer->SyncAddSubBlock(addSbBlkInfo);

    addSbBlkInfo.coordinate = CDimCoordinate::Parse("C1");
    writer->SyncAddSubBlock(addSbBlkInfo);

    // the writer-object can give us a "partially filled out metadata-object"
    auto metadata_to_be_written = writer->GetPreparedMetadata(PrepareMetadataInfo{});

    // ...to which we add here some display-settings
    DisplaySettingsPOD display_settings;
    ChannelDisplaySettingsPOD channel_display_settings;
    channel_display_settings.Clear();
    channel_display_settings.isEnabled = true;
    channel_display_settings.tintingMode = IDisplaySettings::TintingMode::Color;
    channel_display_settings.tintingColor = Rgb8Color{ 0xff,0,0 };
    channel_display_settings.blackPoint = 0.3f;
    channel_display_settings.whitePoint = 0.8f;
    display_settings.channelDisplaySettings[0] = channel_display_settings;  // set the channel-display-settings for channel 0
    channel_display_settings.tintingColor = Rgb8Color{ 0,0xff,0 };
    channel_display_settings.blackPoint = 0.1f;
    channel_display_settings.whitePoint = 0.4f;
    display_settings.channelDisplaySettings[1] = channel_display_settings;  // set the channel-display-settings for channel 1

    // and now, write those display-settings into the metadata-builder-object
    MetadataUtils::WriteDisplaySettings(metadata_to_be_written.get(), DisplaySettingsPOD::CreateIDisplaySettingSp(display_settings).get(), 2);

    // then, get the XML-string containing the metadata, and put this into the CZI-file
    string xml = metadata_to_be_written->GetXml(true);
    WriteMetadataInfo writerMdInfo = { 0 };
    writerMdInfo.szMetadata = xml.c_str();
    writerMdInfo.szMetadataSize = xml.size();
    writer->SyncWriteMetadata(writerMdInfo);

    writer->Close();
    writer.reset();

    size_t cziData_Size;
    auto cziData = outStream->GetCopy(&cziData_Size);
    outStream.reset();	// not needed anymore

    // now, we open the CZI-document (note: this is "in-memory")
    auto inputStream = CreateStreamFromMemory(cziData, cziData_Size);
    auto spReader = libCZI::CreateCZIReader();
    spReader->Open(inputStream);

    // read the metadata-segment, get the document-info-object, and from it the display-settings
    auto metadata = spReader->ReadMetadataSegment()->CreateMetaFromMetadataSegment();
    auto display_settings_from_document = metadata->GetDocumentInfo()->GetDisplaySettings();

    // and here, compare those display-settings we got from the document to the information we put in before
    auto channel_display_settings_from_document = display_settings_from_document->GetChannelDisplaySettings(0);
    ASSERT_TRUE(channel_display_settings_from_document);
    EXPECT_TRUE(channel_display_settings_from_document->GetIsEnabled());
    Rgb8Color tinting_color_from_document;
    EXPECT_TRUE(channel_display_settings_from_document->TryGetTintingColorRgb8(&tinting_color_from_document));
    EXPECT_TRUE(tinting_color_from_document.r == 0xff && tinting_color_from_document.g == 0 && tinting_color_from_document.b == 0);
    float black_point_from_document, white_point_from_document;
    channel_display_settings_from_document->GetBlackWhitePoint(&black_point_from_document, &white_point_from_document);
    EXPECT_NEAR(black_point_from_document, 0.3f, 1e-8f);
    EXPECT_NEAR(white_point_from_document, 0.8f, 1e-8f);

    channel_display_settings_from_document = display_settings_from_document->GetChannelDisplaySettings(1);
    ASSERT_TRUE(channel_display_settings_from_document);
    EXPECT_TRUE(channel_display_settings_from_document->GetIsEnabled());
    EXPECT_TRUE(channel_display_settings_from_document->TryGetTintingColorRgb8(&tinting_color_from_document));
    EXPECT_TRUE(tinting_color_from_document.r == 0 && tinting_color_from_document.g == 0xff && tinting_color_from_document.b == 0);
    channel_display_settings_from_document->GetBlackWhitePoint(&black_point_from_document, &white_point_from_document);
    EXPECT_NEAR(black_point_from_document, 0.1f, 1e-8f);
    EXPECT_NEAR(white_point_from_document, 0.4f, 1e-8f);
}

TEST(DisplaySettings, WriteDisplaySettingsWithGradationCurveGammaAndSplineToDocumentAndReadFromThereAndCompare)
{
    // what happens here:
    // - we are creating a simple 2-channel-CZI-document, add two subblocks 
    // - and, we construct "display-settings" for the document, write them into the CZI-document
    // - then we open the CZI-document
    // - and read the display-settings from it
    // - and, finally, compare them to what we put into
    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);
    //auto outStream = CreateOutputStreamForFile(L"D:\\libczi_displaysettings.czi", true);

    auto spWriterInfo = make_shared<CCziWriterInfo >(
        GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } },
        CDimBounds{ { { DimensionIndex::C,0,2 } } });	// set a bounds for  C

    writer->Create(outStream, spWriterInfo);

    // now add two subblocks (does not really matter, though)
    auto bitmap = CreateTestBitmap(PixelType::Gray8, 64, 64);

    ScopedBitmapLockerSP lockBm{ bitmap };
    AddSubBlockInfoStridedBitmap addSbBlkInfo;
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate = CDimCoordinate::Parse("C0");
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = 0;
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = bitmap->GetWidth();
    addSbBlkInfo.logicalHeight = bitmap->GetHeight();
    addSbBlkInfo.physicalWidth = bitmap->GetWidth();
    addSbBlkInfo.physicalHeight = bitmap->GetHeight();
    addSbBlkInfo.PixelType = bitmap->GetPixelType();
    addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
    addSbBlkInfo.strideBitmap = lockBm.stride;
    writer->SyncAddSubBlock(addSbBlkInfo);

    addSbBlkInfo.coordinate = CDimCoordinate::Parse("C1");
    writer->SyncAddSubBlock(addSbBlkInfo);

    // the writer-object can give us a "partially filled out metadata-object"
    auto metadata_to_be_written = writer->GetPreparedMetadata(PrepareMetadataInfo{});

    // ...to which we add here some display-settings
    DisplaySettingsPOD display_settings;
    ChannelDisplaySettingsPOD channel_display_settings;
    channel_display_settings.Clear();
    channel_display_settings.isEnabled = true;
    channel_display_settings.tintingMode = IDisplaySettings::TintingMode::Color;
    channel_display_settings.tintingColor = Rgb8Color{ 0xff,0,0 };
    channel_display_settings.blackPoint = 0.3f;
    channel_display_settings.whitePoint = 0.8f;
    channel_display_settings.gradationCurveMode = IDisplaySettings::GradationCurveMode::Gamma;
    channel_display_settings.gamma = 0.83f;
    display_settings.channelDisplaySettings[0] = channel_display_settings;  // set the channel-display-settings for channel 0
    channel_display_settings.tintingColor = Rgb8Color{ 0,0xff,0 };
    channel_display_settings.blackPoint = 0.1f;
    channel_display_settings.whitePoint = 0.4f;
    channel_display_settings.gradationCurveMode = IDisplaySettings::GradationCurveMode::Spline;
    channel_display_settings.splineCtrlPoints.push_back(IDisplaySettings::SplineControlPoint{ 0.155251141552511,0.428571428571429 });
    channel_display_settings.splineCtrlPoints.push_back(IDisplaySettings::SplineControlPoint{ 0.468036529680365,0.171428571428571 });
    channel_display_settings.splineCtrlPoints.push_back(IDisplaySettings::SplineControlPoint{ 0.58675799086758,0.657142857142857 });
    channel_display_settings.splineCtrlPoints.push_back(IDisplaySettings::SplineControlPoint{ 0.840182648401826,0.2 });
    display_settings.channelDisplaySettings[1] = channel_display_settings;  // set the channel-display-settings for channel 1

    // and now, write those display-settings into the metadata-builder-object
    MetadataUtils::WriteDisplaySettings(metadata_to_be_written.get(), DisplaySettingsPOD::CreateIDisplaySettingSp(display_settings).get());

    // then, get the XML-string containing the metadata, and put this into the CZI-file
    string xml = metadata_to_be_written->GetXml(true);
    WriteMetadataInfo writerMdInfo = { 0 };
    writerMdInfo.szMetadata = xml.c_str();
    writerMdInfo.szMetadataSize = xml.size();
    writer->SyncWriteMetadata(writerMdInfo);

    writer->Close();
    writer.reset();

    size_t cziData_Size;
    auto cziData = outStream->GetCopy(&cziData_Size);
    outStream.reset();	// not needed anymore

    // now, we open the CZI-document (note: this is "in-memory")
    auto inputStream = CreateStreamFromMemory(cziData, cziData_Size);
    auto spReader = libCZI::CreateCZIReader();
    spReader->Open(inputStream);

    // read the metadata-segment, get the document-info-object, and from it the display-settings
    auto metadata = spReader->ReadMetadataSegment()->CreateMetaFromMetadataSegment();
    auto display_settings_from_document = metadata->GetDocumentInfo()->GetDisplaySettings();

    // and here, compare those display-settings we got from the document to the information we put in before
    auto channel_display_settings_from_document = display_settings_from_document->GetChannelDisplaySettings(0);
    ASSERT_TRUE(channel_display_settings_from_document);
    EXPECT_TRUE(channel_display_settings_from_document->GetIsEnabled());
    Rgb8Color tinting_color_from_document;
    EXPECT_TRUE(channel_display_settings_from_document->TryGetTintingColorRgb8(&tinting_color_from_document));
    EXPECT_TRUE(tinting_color_from_document.r == 0xff && tinting_color_from_document.g == 0 && tinting_color_from_document.b == 0);
    float black_point_from_document, white_point_from_document;
    channel_display_settings_from_document->GetBlackWhitePoint(&black_point_from_document, &white_point_from_document);
    EXPECT_NEAR(black_point_from_document, 0.3f, 1e-8f);
    EXPECT_NEAR(white_point_from_document, 0.8f, 1e-8f);
    EXPECT_TRUE(channel_display_settings_from_document->GetGradationCurveMode() == IDisplaySettings::GradationCurveMode::Gamma);
    float gamma__from_document;
    EXPECT_TRUE(channel_display_settings_from_document->TryGetGamma(&gamma__from_document));
    EXPECT_NEAR(gamma__from_document, 0.83f, 1e-8f);

    channel_display_settings_from_document = display_settings_from_document->GetChannelDisplaySettings(1);
    ASSERT_TRUE(channel_display_settings_from_document);
    EXPECT_TRUE(channel_display_settings_from_document->GetIsEnabled());
    EXPECT_TRUE(channel_display_settings_from_document->TryGetTintingColorRgb8(&tinting_color_from_document));
    EXPECT_TRUE(tinting_color_from_document.r == 0 && tinting_color_from_document.g == 0xff && tinting_color_from_document.b == 0);
    channel_display_settings_from_document->GetBlackWhitePoint(&black_point_from_document, &white_point_from_document);
    EXPECT_NEAR(black_point_from_document, 0.1f, 1e-8f);
    EXPECT_NEAR(white_point_from_document, 0.4f, 1e-8f);
    EXPECT_TRUE(channel_display_settings_from_document->GetGradationCurveMode() == IDisplaySettings::GradationCurveMode::Spline);
    vector<libCZI::IDisplaySettings::SplineControlPoint> spline_control_points_from_document;
    EXPECT_TRUE(channel_display_settings_from_document->TryGetSplineControlPoints(&spline_control_points_from_document));
    ASSERT_EQ(spline_control_points_from_document.size(), 4);
    EXPECT_NEAR(spline_control_points_from_document[0].x, 0.155251141552511, 1e-7);
    EXPECT_NEAR(spline_control_points_from_document[0].y, 0.428571428571429, 1e-7);
    EXPECT_NEAR(spline_control_points_from_document[1].x, 0.468036529680365, 1e-7);
    EXPECT_NEAR(spline_control_points_from_document[1].y, 0.171428571428571, 1e-7);
    EXPECT_NEAR(spline_control_points_from_document[2].x, 0.58675799086758, 1e-7);
    EXPECT_NEAR(spline_control_points_from_document[2].y, 0.657142857142857, 1e-7);
    EXPECT_NEAR(spline_control_points_from_document[3].x, 0.840182648401826, 1e-7);
    EXPECT_NEAR(spline_control_points_from_document[3].y, 0.2, 1e-7);
}

TEST(DisplaySettings, WriteDisplaySettingsAndCheckIdAndNameAttributeAutomaticallyGenerated)
{
    // what happens here:
    // - we are creating a simple 2-channel-CZI-document, add two subblocks 
    // - and, we construct "display-settings" for the document, write them into the CZI-document
    // - we add the display-settings without explicitly setting the "Id" or "Name" attribute, so
    //    we expect that the default values are used
    // - we then open the resulting CZI-document and check if the "Id" and "Name" attributes are set as expected
    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);

    auto spWriterInfo = make_shared<CCziWriterInfo >(
        GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } },
        CDimBounds{ { { DimensionIndex::C,0,2 } } });	// set a bounds for  C

    writer->Create(outStream, spWriterInfo);

    // now add two subblocks (does not really matter, though)
    auto bitmap = CreateTestBitmap(PixelType::Gray8, 64, 64);

    ScopedBitmapLockerSP lockBm{ bitmap };
    AddSubBlockInfoStridedBitmap addSbBlkInfo;
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate = CDimCoordinate::Parse("C0");
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = 0;
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = bitmap->GetWidth();
    addSbBlkInfo.logicalHeight = bitmap->GetHeight();
    addSbBlkInfo.physicalWidth = bitmap->GetWidth();
    addSbBlkInfo.physicalHeight = bitmap->GetHeight();
    addSbBlkInfo.PixelType = bitmap->GetPixelType();
    addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
    addSbBlkInfo.strideBitmap = lockBm.stride;
    writer->SyncAddSubBlock(addSbBlkInfo);

    addSbBlkInfo.coordinate = CDimCoordinate::Parse("C1");
    writer->SyncAddSubBlock(addSbBlkInfo);

    // the writer-object can give us a "partially filled out metadata-object"
    auto metadata_to_be_written = writer->GetPreparedMetadata(PrepareMetadataInfo{});

    // ...to which we add here some display-settings
    DisplaySettingsPOD display_settings;
    ChannelDisplaySettingsPOD channel_display_settings;
    channel_display_settings.Clear();
    channel_display_settings.isEnabled = true;
    channel_display_settings.tintingMode = IDisplaySettings::TintingMode::Color;
    channel_display_settings.tintingColor = Rgb8Color{ 0xff,0,0 };
    channel_display_settings.blackPoint = 0.3f;
    channel_display_settings.whitePoint = 0.8f;
    display_settings.channelDisplaySettings[0] = channel_display_settings;  // set the channel-display-settings for channel 0
    channel_display_settings.tintingColor = Rgb8Color{ 0,0xff,0 };
    channel_display_settings.blackPoint = 0.1f;
    channel_display_settings.whitePoint = 0.4f;
    display_settings.channelDisplaySettings[1] = channel_display_settings;  // set the channel-display-settings for channel 1

    // and now, write those display-settings into the metadata-builder-object
    MetadataUtils::WriteDisplaySettings(metadata_to_be_written.get(), DisplaySettingsPOD::CreateIDisplaySettingSp(display_settings).get(), 2);

    // then, get the XML-string containing the metadata, and put this into the CZI-file
    string xml = metadata_to_be_written->GetXml(true);
    WriteMetadataInfo writerMdInfo;
    writerMdInfo.Clear();
    writerMdInfo.szMetadata = xml.c_str();
    writerMdInfo.szMetadataSize = xml.size();
    writer->SyncWriteMetadata(writerMdInfo);

    writer->Close();
    writer.reset();

    size_t cziData_Size;
    auto cziData = outStream->GetCopy(&cziData_Size);
    outStream.reset();	// not needed anymore

    // now, we open the CZI-document (note: this is "in-memory")
    auto inputStream = CreateStreamFromMemory(cziData, cziData_Size);
    auto spReader = libCZI::CreateCZIReader();
    spReader->Open(inputStream);

    // read the metadata-segment, get the document-info-object, and from it the display-settings
    auto metadata = spReader->ReadMetadataSegment()->CreateMetaFromMetadataSegment();
    ASSERT_TRUE(metadata);

    auto channel0node = metadata->GetChildNodeReadonly("ImageDocument/Metadata/DisplaySetting/Channels/Channel[0]");
    ASSERT_TRUE(channel0node);
    wstring channelIdActual;
    EXPECT_TRUE(channel0node->TryGetAttribute(L"Id", &channelIdActual));
    EXPECT_STREQ(channelIdActual.c_str(), L"Channel:0");
    wstring channelNameActual;
    EXPECT_FALSE(channel0node->TryGetAttribute(L"Name", &channelNameActual));

    auto channel1node = metadata->GetChildNodeReadonly("ImageDocument/Metadata/DisplaySetting/Channels/Channel[1]");
    ASSERT_TRUE(channel1node);
    EXPECT_TRUE(channel1node->TryGetAttribute(L"Id", &channelIdActual));
    EXPECT_STREQ(channelIdActual.c_str(), L"Channel:1");
    EXPECT_FALSE(channel1node->TryGetAttribute(L"Name", &channelNameActual));
}

TEST(DisplaySettings, WriteDisplaySettingsAndCheckIdAndNameAttributeExplictlyGenerated)
{
    // what happens here:
    // - we are creating a simple 2-channel-CZI-document, add two subblocks 
    // - and, we construct "display-settings" for the document, write them into the CZI-document
    // - we add the display-settings with explicitly setting the "Id" or "Name" attribute, and we
    //    expect that those values are then used for the display-settings
    // - we then open the resulting CZI-document and check if the "Id" and "Name" attributes are set as expected
    auto writer = CreateCZIWriter();
    auto outStream = make_shared<CMemOutputStream>(0);

    auto spWriterInfo = make_shared<CCziWriterInfo >(
        GUID{ 0x1234567,0x89ab,0xcdef,{ 1,2,3,4,5,6,7,8 } },
        CDimBounds{ { { DimensionIndex::C,0,2 } } });	// set a bounds for  C

    writer->Create(outStream, spWriterInfo);

    // now add two subblocks (does not really matter, though)
    auto bitmap = CreateTestBitmap(PixelType::Gray8, 64, 64);

    ScopedBitmapLockerSP lockBm{ bitmap };
    AddSubBlockInfoStridedBitmap addSbBlkInfo;
    addSbBlkInfo.Clear();
    addSbBlkInfo.coordinate = CDimCoordinate::Parse("C0");
    addSbBlkInfo.mIndexValid = true;
    addSbBlkInfo.mIndex = 0;
    addSbBlkInfo.x = 0;
    addSbBlkInfo.y = 0;
    addSbBlkInfo.logicalWidth = bitmap->GetWidth();
    addSbBlkInfo.logicalHeight = bitmap->GetHeight();
    addSbBlkInfo.physicalWidth = bitmap->GetWidth();
    addSbBlkInfo.physicalHeight = bitmap->GetHeight();
    addSbBlkInfo.PixelType = bitmap->GetPixelType();
    addSbBlkInfo.ptrBitmap = lockBm.ptrDataRoi;
    addSbBlkInfo.strideBitmap = lockBm.stride;
    writer->SyncAddSubBlock(addSbBlkInfo);

    addSbBlkInfo.coordinate = CDimCoordinate::Parse("C1");
    writer->SyncAddSubBlock(addSbBlkInfo);

    // the writer-object can give us a "partially filled out metadata-object"
    PrepareMetadataInfo prepareMetadataInfo;
    prepareMetadataInfo.funcGenerateIdAndNameForChannel = [](size_t channelIndex) ->std::tuple < std::string, std::tuple<bool, std::string>>
        {
            if (channelIndex != 0 && channelIndex != 1)
            {
                throw std::runtime_error("Invalid channel index");
            }

            return std::make_tuple("Ch:" + std::to_string(channelIndex), std::make_tuple(true, "ChannelName:" + std::to_string(channelIndex)));
        };

    auto metadata_to_be_written = writer->GetPreparedMetadata(prepareMetadataInfo);

    // ...to which we add here some display-settings
    DisplaySettingsPOD display_settings;
    ChannelDisplaySettingsPOD channel_display_settings;
    channel_display_settings.Clear();
    channel_display_settings.isEnabled = true;
    channel_display_settings.tintingMode = IDisplaySettings::TintingMode::Color;
    channel_display_settings.tintingColor = Rgb8Color{ 0xff,0,0 };
    channel_display_settings.blackPoint = 0.3f;
    channel_display_settings.whitePoint = 0.8f;
    display_settings.channelDisplaySettings[0] = channel_display_settings;  // set the channel-display-settings for channel 0
    channel_display_settings.tintingColor = Rgb8Color{ 0,0xff,0 };
    channel_display_settings.blackPoint = 0.1f;
    channel_display_settings.whitePoint = 0.4f;
    display_settings.channelDisplaySettings[1] = channel_display_settings;  // set the channel-display-settings for channel 1

    // and now, write those display-settings into the metadata-builder-object
    MetadataUtils::WriteDisplaySettings(metadata_to_be_written.get(), DisplaySettingsPOD::CreateIDisplaySettingSp(display_settings).get(), 2);

    // then, get the XML-string containing the metadata, and put this into the CZI-file
    string xml = metadata_to_be_written->GetXml(true);
    WriteMetadataInfo writerMdInfo;
    writerMdInfo.Clear();
    writerMdInfo.szMetadata = xml.c_str();
    writerMdInfo.szMetadataSize = xml.size();
    writer->SyncWriteMetadata(writerMdInfo);

    writer->Close();
    writer.reset();

    size_t cziData_Size;
    auto cziData = outStream->GetCopy(&cziData_Size);
    outStream.reset();	// not needed anymore

    // now, we open the CZI-document (note: this is "in-memory")
    auto inputStream = CreateStreamFromMemory(cziData, cziData_Size);
    auto spReader = libCZI::CreateCZIReader();
    spReader->Open(inputStream);

    // read the metadata-segment, get the document-info-object, and from it the display-settings
    auto metadata = spReader->ReadMetadataSegment()->CreateMetaFromMetadataSegment();
    ASSERT_TRUE(metadata);

    auto channel0node = metadata->GetChildNodeReadonly("ImageDocument/Metadata/DisplaySetting/Channels/Channel[0]");
    ASSERT_TRUE(channel0node);
    wstring channelIdActual;
    EXPECT_TRUE(channel0node->TryGetAttribute(L"Id", &channelIdActual));
    EXPECT_STREQ(channelIdActual.c_str(), L"Ch:0");
    wstring channelNameActual;
    EXPECT_TRUE(channel0node->TryGetAttribute(L"Name", &channelNameActual));
    EXPECT_STREQ(channelNameActual.c_str(), L"ChannelName:0");

    auto channel1node = metadata->GetChildNodeReadonly("ImageDocument/Metadata/DisplaySetting/Channels/Channel[1]");
    ASSERT_TRUE(channel1node);
    EXPECT_TRUE(channel1node->TryGetAttribute(L"Id", &channelIdActual));
    EXPECT_STREQ(channelIdActual.c_str(), L"Ch:1");
    EXPECT_TRUE(channel1node->TryGetAttribute(L"Name", &channelNameActual));
    EXPECT_STREQ(channelNameActual.c_str(), L"ChannelName:1");
}
