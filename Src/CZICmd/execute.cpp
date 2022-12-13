// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "stdafx.h"
#include <limits>
#include "execute.h"
#include "executeCreateCzi.h"
#include "inc_libCZI.h"
#include "SaveBitmap.h"
#include "utils.h"
#include "DisplaySettingsHelper.h"
#include "inc_rapidjson.h"
#include <iomanip>
#include <map>
#include <fstream>

using namespace libCZI;
using namespace std;
using namespace rapidjson;

class CExecuteBase
{
protected:
    static std::shared_ptr<ICZIReader> CreateAndOpenCziReader(const CCmdLineOptions& options)
    {
        return CreateAndOpenCziReader(options.GetCZIFilename().c_str());
    }

    static std::shared_ptr<ICZIReader> CreateAndOpenCziReader(const wchar_t* fileName)
    {
        auto stream = libCZI::CreateStreamFromFile(fileName);
        auto spReader = libCZI::CreateCZIReader();
        spReader->Open(stream);
        return spReader;
    }

    static IntRect GetRoiFromOptions(const CCmdLineOptions& options, const SubBlockStatistics& subBlockStatistics)
    {
        IntRect roi{ options.GetRectX(), options.GetRectY(), options.GetRectW(), options.GetRectH() };
        if (options.GetIsRelativeRectCoordinate())
        {
            roi.x += subBlockStatistics.boundingBox.x;
            roi.y += subBlockStatistics.boundingBox.y;
        }

        return roi;
    }

    static libCZI::RgbFloatColor GetBackgroundColorFromOptions(const CCmdLineOptions& options)
    {
        return options.GetBackGroundColor();
    }

    static void DoCalcHashOfResult(shared_ptr<libCZI::IBitmapData> bm, const CCmdLineOptions& options)
    {
        DoCalcHashOfResult(bm.get(), options);
    }

    static void HandleHashOfResult(std::function<bool(uint8_t*, size_t)> f, const CCmdLineOptions& options)
    {
        if (!options.GetCalcHashOfResult())
            return;

        uint8_t md5sumHash[16];
        if (!f(md5sumHash, sizeof(md5sumHash)))
            return;
        string hashHex = BytesToHexString(md5sumHash, sizeof(md5sumHash));
        std::stringstream ss;
        ss << "hash of result: " << hashHex;
        auto log = options.GetLog();
        log->WriteLineStdOut(ss.str().c_str());
    }

    static void DoCalcHashOfResult(libCZI::IBitmapData* bm, const CCmdLineOptions& options)
    {
        HandleHashOfResult(
            [&](uint8_t* ptrHash, size_t size)->bool
            {
                Utils::CalcMd5SumHash(bm, ptrHash, (int)size);
        return true;
            },
            options);
    }
};

class CExecutePrintInformation : CExecuteBase
{
public:
    static bool execute(const CCmdLineOptions& options)
    {
        auto spReader = CreateAndOpenCziReader(options);

        if (options.IsInfoLevelEnabled(InfoLevel::Statistics))
        {
            PrintStatistics(options, spReader.get());
        }

        auto mds = spReader->ReadMetadataSegment();
        auto md = mds->CreateMetaFromMetadataSegment();

        if (options.IsInfoLevelEnabled(InfoLevel::RawXML))
        {
            PrintRawXml(md.get(), options);
        }

        if (options.IsInfoLevelEnabled(InfoLevel::ScalingInfo))
        {
            PrintScalingInfo(md.get(), options);
        }

        if (options.IsInfoLevelEnabled(InfoLevel::GeneralInfo))
        {
            PrintGeneralInfo(md.get(), options);
        }


        if (options.IsInfoLevelEnabled(InfoLevel::DisplaySettings))
        {
            PrintDisplaySettingsMetadata(spReader.get(), md.get(), options);
        }

        if (options.IsInfoLevelEnabled(InfoLevel::DisplaySettingsJson))
        {
            PrintDisplaySettingsMetadataAsJson(spReader.get(), md.get(), options);
        }

        if (options.IsInfoLevelEnabled(InfoLevel::AllSubBlocks))
        {
            PrintAllSubBlocks(spReader.get(), options);
        }

        if (options.IsInfoLevelEnabled(InfoLevel::AttachmentInfo))
        {
            PrintAttachmentInfo(spReader.get(), options);
        }

        if (options.IsInfoLevelEnabled(InfoLevel::AllAttachments))
        {
            PrintAllAttachments(spReader.get(), options);
        }

        if (options.IsInfoLevelEnabled(InfoLevel::PyramidStatistics))
        {
            PrintPyramidStatistics(spReader.get(), options);
        }

        return true;
    }

private:
    static void PrintAttachmentInfo(ICZIReader* reader, const CCmdLineOptions& options)
    {
        options.GetLog()->WriteLineStdOut("Attachment Info");
        options.GetLog()->WriteLineStdOut("---------------");
        options.GetLog()->WriteLineStdOut("");

        std::map<std::string, int> mapAttchmntName;
        reader->EnumerateAttachments(
            [&](int index, const AttachmentInfo& info)->bool
            {
                ++mapAttchmntName[info.name];
        return true;
            });

        if (mapAttchmntName.empty())
        {
            options.GetLog()->WriteLineStdOut(" -> No attachments found.");
        }
        else
        {
            options.GetLog()->WriteLineStdOut("count | name");
            options.GetLog()->WriteLineStdOut("------+----------------------------");
            for (auto i : mapAttchmntName)
            {
                stringstream ss;
                ss << setw(5) << i.second << " | " << i.first;
                options.GetLog()->WriteLineStdOut(ss.str());
            }
        }
    }

    static void PrintScalingInfo(ICziMetadata* md, const CCmdLineOptions& options)
    {
        options.GetLog()->WriteLineStdOut("Scaling-Information");
        options.GetLog()->WriteLineStdOut("-------------------");
        options.GetLog()->WriteLineStdOut("");
        options.GetLog()->WriteLineStdOut(" (the numbers give the length of one pixel (in the respective direction) in the unit 'meter')");
        options.GetLog()->WriteLineStdOut("");

        auto docInfo = md->GetDocumentInfo();
        auto scalingInfo = docInfo->GetScalingInfo();
        stringstream ss;
        ss << "ScaleX=" << scalingInfo.scaleX << endl;
        ss << "ScaleY=" << scalingInfo.scaleY << endl;
        ss << "ScaleZ=" << scalingInfo.scaleZ << endl;
        options.GetLog()->WriteLineStdOut(ss.str());
    }

    static void PrintGeneralInfo(ICziMetadata* md, const CCmdLineOptions& options)
    {
        options.GetLog()->WriteLineStdOut("General Information");
        options.GetLog()->WriteLineStdOut("-------------------");
        options.GetLog()->WriteLineStdOut("");

        auto docInfo = md->GetDocumentInfo();
        auto generalInfo = docInfo->GetGeneralDocumentInfo();
        const wstring unspecifiedString(L"<unspecified>");
        wstringstream ss;
        ss << "Name=" << (generalInfo.name_valid ? generalInfo.name : unspecifiedString) << endl;
        ss << "Title=" << (generalInfo.title_valid ? generalInfo.title : unspecifiedString) << endl;
        ss << "UserName=" << (generalInfo.userName_valid ? generalInfo.userName : unspecifiedString) << endl;
        ss << "Description=" << (generalInfo.description_valid ? generalInfo.description : unspecifiedString) << endl;
        ss << "Comment=" << (generalInfo.comment_valid ? generalInfo.comment : unspecifiedString) << endl;
        ss << "Keywords=" << (generalInfo.keywords_valid ? generalInfo.keywords : unspecifiedString) << endl;
        ss << "Rating=";
        if (generalInfo.rating_valid)
        {
            ss << generalInfo.rating << endl;
        }
        else
        {
            ss << unspecifiedString << endl;
        }

        ss << "CreationDate=" << (generalInfo.creationDateTime_valid ? generalInfo.creationDateTime : unspecifiedString) << endl;
        options.GetLog()->WriteLineStdOut(ss.str());
    }

    static void PrintAllAttachments(ICZIReader* reader, const CCmdLineOptions& options)
    {
        options.GetLog()->WriteLineStdOut("Complete list of Attachments");
        options.GetLog()->WriteLineStdOut("----------------------------");
        options.GetLog()->WriteLineStdOut("");

        bool isFirst = true;
        reader->EnumerateAttachments(
            [&](int index, const AttachmentInfo& info)->bool
            {
                if (isFirst == true)
                {
                    isFirst = false;
                    options.GetLog()->WriteLineStdOut("index | filetype | GUID                                   | name");
                    options.GetLog()->WriteLineStdOut("------+----------+----------------------------------------+-------------");
                }

        stringstream ss;
        ss << setw(5) << index << " | " << setw(8) << std::left << info.contentFileType << " | {" << info.contentGuid << "} | " << info.name;
        options.GetLog()->WriteLineStdOut(ss.str());
        return true;
            });
    }

    static void PrintAllSubBlocks(ICZIReader* reader, const CCmdLineOptions& options)
    {
        options.GetLog()->WriteLineStdOut("Complete list of sub-blocks");
        options.GetLog()->WriteLineStdOut("---------------------------");
        options.GetLog()->WriteLineStdOut("");
        reader->EnumerateSubBlocks(
            [&](int index, const SubBlockInfo& info)->bool
            {
                stringstream ss;
        ss << "#" << index << ": " << Utils::DimCoordinateToString(&info.coordinate);
        if (info.IsMindexValid())
        {
            ss << " M=" << info.mIndex;
        }

        ss << " logical=" << info.logicalRect << " phys.=" << info.physicalSize;
        ss << " pixeltype=" << Utils::PixelTypeToInformalString(info.pixelType);
        auto compressionMode = info.GetCompressionMode();
        if (compressionMode != CompressionMode::Invalid)
        {
            ss << " compression=" << Utils::CompressionModeToInformalString(compressionMode);
        }
        else
        {
            ss << " compression=" << Utils::CompressionModeToInformalString(compressionMode) << "(" << info.compressionModeRaw << ")";
        }

        options.GetLog()->WriteLineStdOut(ss.str());
        return true;
            });
    }

    static void PrintRawXml(ICziMetadata* md, const CCmdLineOptions& options)
    {
        std::string xmlUtf8 = md->GetXml();

        //TODO: should we convert to UCS2/UTF16?
        options.GetLog()->WriteLineStdOut(xmlUtf8);
    }

    static void PrintDisplaySettingsMetadata(ICZIReader* reader, ICziMetadata* md, const CCmdLineOptions& options)
    {
        options.GetLog()->WriteLineStdOut("Display-Settings");
        options.GetLog()->WriteLineStdOut("----------------");
        options.GetLog()->WriteLineStdOut("");

        int startC = 0, endC = 0;

        int sizeC;
        if (reader->GetStatistics().dimBounds.TryGetInterval(DimensionIndex::C, &startC, &sizeC) == true)
        {
            endC = startC + sizeC;
        }

        auto docInfo = md->GetDocumentInfo();
        auto dsplSettings = docInfo->GetDisplaySettings();
        if (!dsplSettings)
        {
            options.GetLog()->WriteLineStdOut("-> No Display-Settings available");
            return;
        }

        dsplSettings->EnumChannels(
            [&](int chIdx)->bool
            {
                auto dsplChannelSettings = dsplSettings->GetChannelDisplaySettings(chIdx);
        PrintDisplaySettingsForChannel(chIdx, dsplChannelSettings.get(), options);
        return true;
            });
    }

    static void PrintDisplaySettingsMetadataAsJson(ICZIReader* reader, ICziMetadata* md, const CCmdLineOptions& options)
    {
        options.GetLog()->WriteLineStdOut("Display-Settings in CZIcmd-JSON-Format");
        options.GetLog()->WriteLineStdOut("--------------------------------------");
        options.GetLog()->WriteLineStdOut("");
        auto docInfo = md->GetDocumentInfo();
        auto dsplSettings = docInfo->GetDisplaySettings();
        if (!dsplSettings)
        {
            options.GetLog()->WriteLineStdOut("-> No Display-Settings available");
            return;
        }

        string dsplSettingsJson = CreateJsonForDisplaySettings(dsplSettings.get());

        Document document;
        document.Parse(dsplSettingsJson.c_str());
        StringBuffer sb;
        PrettyWriter<StringBuffer> pw(sb);
        document.Accept(pw);
        options.GetLog()->WriteLineStdOut("");
        options.GetLog()->WriteLineStdOut("Pretty-Print:");
        options.GetLog()->WriteLineStdOut(sb.GetString());

        options.GetLog()->WriteLineStdOut("");
        options.GetLog()->WriteLineStdOut("Compact:");
        options.GetLog()->WriteLineStdOut(dsplSettingsJson);
    }

    static void PrintDisplaySettingsForChannel(int ch, IChannelDisplaySetting* dsplChSettings, const CCmdLineOptions& options)
    {
        bool b, isEnabled;
        stringstream ss;
        ss << "Channel #" << ch << endl;
        ss << "==========" << endl;
        isEnabled = dsplChSettings->GetIsEnabled();
        ss << " Enabled: " << ((isEnabled) ? "yes" : "no") << endl;

        Rgb8Color col;
        b = dsplChSettings->TryGetTintingColorRgb8(&col);
        if (b == false)
        {
            ss << " Tinting: no" << endl;
        }
        else
        {
            ss << " Tinting: yes (R=" << (int)col.r << ", G=" << (int)col.g << ", B=" << (int)col.b << ")" << endl;
        }

        float blkpoint, whtpoint;
        dsplChSettings->GetBlackWhitePoint(&blkpoint, &whtpoint);
        ss << " Black-point: " << blkpoint << "  White-point: " << whtpoint << endl;

        ss << " Gradation-curve-mode: ";
        IDisplaySettings::GradationCurveMode gradationCurveMode = dsplChSettings->GetGradationCurveMode();
        switch (gradationCurveMode)
        {
        case IDisplaySettings::GradationCurveMode::Linear:
            ss << "linear";
            break;
        case IDisplaySettings::GradationCurveMode::Gamma:
        {
            float gamma;
            dsplChSettings->TryGetGamma(&gamma);
            ss << "gamma (" << gamma << ")";
        }
        break;
        case IDisplaySettings::GradationCurveMode::Spline:
            ss << "spline";
        }

        ss << endl;

        options.GetLog()->WriteLineStdOut(ss.str());
    }

    static string CreateJsonForDisplaySettings(IDisplaySettings* dsplSettings)
    {
        StringBuffer s;
        Writer<StringBuffer> writer(s);
        writer.StartObject();
        writer.String("channels");
        writer.StartArray();
        dsplSettings->EnumChannels(
            [&](int chIdx)->bool
            {
                auto dsplChannelSettings = dsplSettings->GetChannelDisplaySettings(chIdx);
        if (dsplChannelSettings->GetIsEnabled())
        {
            writer.StartObject();
            writer.String("ch");
            writer.Int(chIdx);
            float wght = dsplChannelSettings->GetWeight();
            if (abs(wght - 1) > std::numeric_limits<float>::epsilon())
            {
                writer.String("weight");
                writer.Double(wght);
            }

            float blkPt, whtPt;
            dsplChannelSettings->GetBlackWhitePoint(&blkPt, &whtPt);
            {
                writer.String("black-point");
                writer.Double(blkPt);
                writer.String("white-point");
                writer.Double(whtPt);
            }

            Rgb8Color tinting_color;
            if (dsplChannelSettings->TryGetTintingColorRgb8(&tinting_color))
            {
                writer.String("tinting");
                stringstream ss;
                ss << '#' << std::hex
                    << std::setfill('0') << std::setw(2) << (int)tinting_color.r
                    << std::setfill('0') << std::setw(2) << (int)tinting_color.g
                    << std::setfill('0') << std::setw(2) << (int)tinting_color.b;
                writer.String(ss.str());
            }

            switch (dsplChannelSettings->GetGradationCurveMode())
            {
            case IDisplaySettings::GradationCurveMode::Gamma:
            {
                float gamma;
                dsplChannelSettings->TryGetGamma(&gamma);
                writer.String("gamma");
                writer.Double(gamma);
            }
            break;
            case IDisplaySettings::GradationCurveMode::Spline:
            {
                std::vector<libCZI::IDisplaySettings::SplineControlPoint> ctrlPoints;
                dsplChannelSettings->TryGetSplineControlPoints(&ctrlPoints);
                writer.String("splinelut");
                writer.StartArray();
                for (const auto& ctrlPt : ctrlPoints)
                {
                    writer.Double(ctrlPt.x);
                    writer.Double(ctrlPt.y);
                }
                writer.EndArray();
            }
            break;
            default:
                break;
            }

            writer.EndObject();
        }

        return true;
            });

        writer.EndArray();
        writer.EndObject();

        string str = s.GetString();
        return str;
    }

    static void PrintStatistics(const CCmdLineOptions& options, ICZIReader* reader)
    {
        auto log = options.GetLog();

        auto sbStatistics = reader->GetStatistics();
        stringstream ss;
        ss << "SubBlock-Statistics" << endl;
        ss << "-------------------" << endl;
        ss << endl;
        ss << "SubBlock-Count: " << sbStatistics.subBlockCount << endl;
        ss << endl;
        ss << "Bounding-Box:" << endl;
        ss << " All:    ";
        WriteIntRect(ss, sbStatistics.boundingBox);
        ss << endl;
        ss << " Layer0: ";
        WriteIntRect(ss, sbStatistics.boundingBoxLayer0Only);
        ss << endl;

        ss << endl;;
        if (sbStatistics.IsMIndexValid())
        {
            ss << "M-Index: min=" << sbStatistics.minMindex << " max=" << sbStatistics.maxMindex << endl;
        }
        else
        {
            ss << "M-Index: not valid" << endl;
        }

        ss << endl;
        ss << "Bounds:" << endl;
        sbStatistics.dimBounds.EnumValidDimensions(
            [&](libCZI::DimensionIndex dim, int start, int size)->bool
            {
                ss << " " << Utils::DimensionToChar(dim) << " -> Start=" << start << " Size=" << size << endl;
        return true;
            });

        if (!sbStatistics.sceneBoundingBoxes.empty())
        {
            ss << endl;
            ss << "Bounding-Box for scenes:" << endl;
            for (const auto& sceneBb : sbStatistics.sceneBoundingBoxes)
            {
                ss << " Scene" << sceneBb.first << ":" << endl;
                ss << "  All:    ";
                WriteIntRect(ss, sceneBb.second.boundingBox);
                ss << endl;
                ss << "  Layer0: ";
                WriteIntRect(ss, sceneBb.second.boundingBoxLayer0);
                ss << endl;
            }
        }

        log->WriteLineStdOut(ss.str());
    }

    static void PrintPyramidStatistics(ICZIReader* reader, const CCmdLineOptions& options)
    {
        auto log = options.GetLog();

        auto pyrStatistics = reader->GetPyramidStatistics();
        stringstream ss;
        ss << "Pyramid-Subblock-Statistics" << endl;
        ss << "---------------------------" << endl;
        ss << endl;

        for (const auto& i : pyrStatistics.scenePyramidStatistics)
        {
            // scene-index==int::max means "scene-index not valid"
            if (i.first != (numeric_limits<int>::max)())
            {
                ss << "scene#" << i.first << ":" << endl;
            }

            for (const auto& j : i.second)
            {
                if (!j.layerInfo.IsNotIdentifiedAsPyramidLayer())
                {
                    int scaleDenom;
                    if (j.layerInfo.IsLayer0() == true)
                    {
                        scaleDenom = 1;
                    }
                    else
                    {
                        scaleDenom = j.layerInfo.minificationFactor;
                        for (int n = 0; n < j.layerInfo.pyramidLayerNo - 1; ++n)
                        {
                            scaleDenom *= j.layerInfo.minificationFactor;
                        }
                    }

                    ss << " number of subblocks with scale 1/" << scaleDenom << ": " << j.count << endl;
                }
                else
                {
                    ss << " number of subblocks not representable as pyramid-layers: " << j.count << endl;;
                }
            }

            ss << endl;
        }

        log->WriteLineStdOut(ss.str());
    }

    static void WriteIntRect(stringstream& ss, const IntRect& r)
    {
        if (r.IsValid())
        {
            ss << "X=" << r.x << " Y=" << r.y << " W=" << r.w << " H=" << r.h;
        }
        else
        {
            ss << "invalid";
        }
    }
};

class CExecuteSingleChannelTileAccessor : CExecuteBase
{
public:
    static bool execute(const CCmdLineOptions& options)
    {
        auto spReader = CreateAndOpenCziReader(options);

        auto accessor = spReader->CreateSingleChannelTileAccessor();

        libCZI::CDimCoordinate coordinate = options.GetPlaneCoordinate();
        libCZI::ISingleChannelTileAccessor::Options sctaOptions; sctaOptions.Clear();
        sctaOptions.sortByM = true;
        sctaOptions.drawTileBorder = options.GetDrawTileBoundaries();

        IntRect roi{ options.GetRectX() ,options.GetRectY(),options.GetRectW(),options.GetRectH() };
        if (options.GetIsRelativeRectCoordinate())
        {
            auto statistics = spReader->GetStatistics();
            roi.x += statistics.boundingBox.x;
            roi.y += statistics.boundingBox.y;
        }

        auto re = accessor->Get(roi, &coordinate, &sctaOptions);

        DoCalcHashOfResult(re, options);

        std::wstring outputfilename = options.MakeOutputFilename(L"", L"PNG");

        auto saver = CSaveBitmapFactory::CreateSaveBitmapObj(nullptr);
        saver->Save(outputfilename.c_str(), SaveDataFormat::PNG, re.get());

        return true;
    }
};

class CExecuteChannelComposite : CExecuteBase
{
public:
    static bool execute(const CCmdLineOptions& options)
    {
        auto spReader = CreateAndOpenCziReader(options);

        libCZI::CDisplaySettingsHelper dsplHlp;
        std::vector<int> activeChannels;
        static std::vector<shared_ptr<IBitmapData>> channelBitmaps;
        std::shared_ptr<libCZI::IDisplaySettings> dsplSettings;
        if (options.GetUseDisplaySettingsFromDocument())
        {
            auto mds = spReader->ReadMetadataSegment();
            auto md = mds->CreateMetaFromMetadataSegment();
            auto docInfo = md->GetDocumentInfo();
            dsplSettings = docInfo->GetDisplaySettings();
        }
        else
        {
            dsplSettings = std::make_shared<CDisplaySettingsWrapper>(options);
        }

        if (!dsplSettings)
        {
            options.GetLog()->WriteLineStdErr("No Display-Settings available.");
            return false;
        }

        activeChannels = libCZI::CDisplaySettingsHelper::GetActiveChannels(dsplSettings.get());
        channelBitmaps = GetBitmapsFromSpecifiedChannels(
            spReader.get(),
            options,
            [&](int idx, int& chNo)->bool
            {
                if (idx < (int)activeChannels.size())
                {
                    chNo = activeChannels.at(idx);
                    return true;
                }

        return false;
            });

        dsplHlp.Initialize(dsplSettings.get(), [&](int chIndx)->libCZI::PixelType
            {
                int idx = (int)std::distance(activeChannels.cbegin(), std::find(activeChannels.cbegin(), activeChannels.cend(), chIndx));
        return channelBitmaps[idx]->GetPixelType();
            });

        shared_ptr<IBitmapData> mcComposite;
        switch (options.GetChannelCompositeOutputPixelType())
        {
        case libCZI::PixelType::Bgr24:
            mcComposite = libCZI::Compositors::ComposeMultiChannel_Bgr24(
                (int)channelBitmaps.size(),
                std::begin(channelBitmaps),
                dsplHlp.GetChannelInfosArray());
            break;
        case libCZI::PixelType::Bgra32:
            mcComposite = libCZI::Compositors::ComposeMultiChannel_Bgra32(
                options.GetChannelCompositeOutputAlphaValue(),
                (int)channelBitmaps.size(),
                std::begin(channelBitmaps),
                dsplHlp.GetChannelInfosArray());
            break;
        default:
            break;
        }

        if (!mcComposite)
        {
            options.GetLog()->WriteLineStdErr("Unknown output pixeltype.");
            return false;
        }

        DoCalcHashOfResult(mcComposite, options);
        std::wstring outputfilename = options.MakeOutputFilename(L"", L"PNG");

        auto saver = CSaveBitmapFactory::CreateSaveBitmapObj(nullptr);
        saver->Save(outputfilename.c_str(), SaveDataFormat::PNG, mcComposite.get());

        return true;
    }
private:
    static std::vector<shared_ptr<IBitmapData>> GetBitmapsFromSpecifiedChannels(ICZIReader* reader, const CCmdLineOptions& options, std::function<bool(int index, int& channelNo)> getChannelNo)
    {
        std::vector<shared_ptr<IBitmapData>> chBitmaps;
        libCZI::CDimCoordinate coordinate = options.GetPlaneCoordinate();

        auto subBlockStatistics = reader->GetStatistics();

        libCZI::ISingleChannelTileAccessor::Options sctaOptions; sctaOptions.Clear();
        sctaOptions.sortByM = true;
        sctaOptions.drawTileBorder = options.GetDrawTileBoundaries();
        sctaOptions.backGroundColor = GetBackgroundColorFromOptions(options);
        IntRect roi{ options.GetRectX() ,options.GetRectY() ,options.GetRectW(),options.GetRectH() };
        if (options.GetIsRelativeRectCoordinate())
        {
            roi.x += subBlockStatistics.boundingBox.x;
            roi.y += subBlockStatistics.boundingBox.y;
        }

        auto accessor = reader->CreateSingleChannelTileAccessor();

        for (int i = 0;; ++i)
        {
            int chNo;
            if (getChannelNo(i, chNo) == false)
            {
                break;
            }

            if (subBlockStatistics.dimBounds.IsValid(DimensionIndex::C))
            {
                // That's a cornerstone case - or a loophole in the specification: if the document
                // does not contain C-dimension (=none of the sub-blocks has a valid C-dimension),
                // then we must not set the C-dimension here. I suppose we should define that a
                // valid C-dimension is mandatory...
                coordinate.Set(DimensionIndex::C, chNo);
            }

            chBitmaps.emplace_back(accessor->Get(roi, &coordinate, &sctaOptions));
        }

        return chBitmaps;
    }
};

class CExecuteSingleChannelPyramidTileAccessor : CExecuteBase
{
public:
    static bool execute(const CCmdLineOptions& options)
    {
        auto reader = CreateAndOpenCziReader(options);
        auto subBlockStatistics = reader->GetStatistics();
        auto accessor = reader->CreateSingleChannelPyramidLayerTileAccessor();

        auto roi = GetRoiFromOptions(options, subBlockStatistics);
        libCZI::CDimCoordinate coordinate = options.GetPlaneCoordinate();
        libCZI::ISingleChannelPyramidLayerTileAccessor::Options scptaOptions; scptaOptions.Clear();
        scptaOptions.backGroundColor = GetBackgroundColorFromOptions(options);
        scptaOptions.sceneFilter = options.GetSceneIndexSet();
        libCZI::ISingleChannelPyramidLayerTileAccessor::PyramidLayerInfo pyrLyrInfo;
        pyrLyrInfo.minificationFactor = options.GetPyramidInfoMinificationFactor();
        pyrLyrInfo.pyramidLayerNo = options.GetPyramidInfoLayerNo();

        auto re = accessor->Get(roi, &coordinate, pyrLyrInfo, &scptaOptions);

        DoCalcHashOfResult(re, options);
        std::wstring outputfilename = options.MakeOutputFilename(L"", L"PNG");

        auto saver = CSaveBitmapFactory::CreateSaveBitmapObj(nullptr);
        saver->Save(outputfilename.c_str(), SaveDataFormat::PNG, re.get());

        return true;
    }
};

class CExecuteSingleChannelScalingTileAccessor : public CExecuteBase
{
public:
    static bool execute(const CCmdLineOptions& options)
    {
        auto reader = CreateAndOpenCziReader(options);
        auto subBlockStatistics = reader->GetStatistics();
        auto accessor = reader->CreateSingleChannelScalingTileAccessor();

        auto roi = GetRoiFromOptions(options, subBlockStatistics);
        libCZI::CDimCoordinate coordinate = options.GetPlaneCoordinate();
        libCZI::ISingleChannelScalingTileAccessor::Options scstaOptions; scstaOptions.Clear();
        scstaOptions.backGroundColor = GetBackgroundColorFromOptions(options);
        scstaOptions.sceneFilter = options.GetSceneIndexSet();

        auto re = accessor->Get(roi, &coordinate, options.GetZoom(), &scstaOptions);

        DoCalcHashOfResult(re, options);
        std::wstring outputfilename = options.MakeOutputFilename(L"", L"PNG");

        auto saver = CSaveBitmapFactory::CreateSaveBitmapObj(nullptr);
        saver->Save(outputfilename.c_str(), SaveDataFormat::PNG, re.get());
        return true;
    }
};

class CExecuteScalingChannelComposite : CExecuteBase
{
public:
    static bool execute(const CCmdLineOptions& options)
    {
        const auto spReader = CreateAndOpenCziReader(options);
        libCZI::CDisplaySettingsHelper dsplHlp;
        static std::vector<shared_ptr<IBitmapData>> channelBitmaps;
        std::shared_ptr<libCZI::IDisplaySettings> dsplSettings;
        if (options.GetUseDisplaySettingsFromDocument())
        {
            const auto mds = spReader->ReadMetadataSegment();
            const auto md = mds->CreateMetaFromMetadataSegment();
            const auto docInfo = md->GetDocumentInfo();
            dsplSettings = docInfo->GetDisplaySettings();
        }
        else
        {
            dsplSettings = std::make_shared<CDisplaySettingsWrapper>(options);
        }

        if (!dsplSettings)
        {
            options.GetLog()->WriteLineStdErr("No Display-Settings available.");
            return false;
        }

        const std::vector<int> activeChannels = libCZI::CDisplaySettingsHelper::GetActiveChannels(dsplSettings.get());
        channelBitmaps = GetBitmapsFromSpecifiedChannels(
            spReader.get(),
            options,
            [&](int idx, int& chNo)->bool
            {
                if (idx < (int)activeChannels.size())
                {
                    chNo = activeChannels.at(idx);
                    return true;
                }

        return false;
            });

        dsplHlp.Initialize(dsplSettings.get(), [&](int chIndx)->libCZI::PixelType
            {
                int idx = (int)std::distance(activeChannels.cbegin(), std::find(activeChannels.cbegin(), activeChannels.cend(), chIndx));
        return channelBitmaps[idx]->GetPixelType();
            });

        shared_ptr<IBitmapData> mcComposite;
        switch (options.GetChannelCompositeOutputPixelType())
        {
        case libCZI::PixelType::Bgr24:
            mcComposite = libCZI::Compositors::ComposeMultiChannel_Bgr24(
                (int)channelBitmaps.size(),
                std::begin(channelBitmaps),
                dsplHlp.GetChannelInfosArray());
            break;
        case libCZI::PixelType::Bgra32:
            mcComposite = libCZI::Compositors::ComposeMultiChannel_Bgra32(
                options.GetChannelCompositeOutputAlphaValue(),
                (int)channelBitmaps.size(),
                std::begin(channelBitmaps),
                dsplHlp.GetChannelInfosArray());
            break;
        default:
            break;
        }

        if (!mcComposite)
        {
            options.GetLog()->WriteLineStdErr("Unknown output pixeltype.");
            return false;
        }

        DoCalcHashOfResult(mcComposite, options);
        std::wstring outputfilename = options.MakeOutputFilename(L"", L"PNG");

        auto saver = CSaveBitmapFactory::CreateSaveBitmapObj(nullptr);
        saver->Save(outputfilename.c_str(), SaveDataFormat::PNG, mcComposite.get());

        return true;
    }
private:
    static std::vector<shared_ptr<IBitmapData>> GetBitmapsFromSpecifiedChannels(ICZIReader* reader, const CCmdLineOptions& options, std::function<bool(int index, int& channelNo)> getChannelNo)
    {
        std::vector<shared_ptr<IBitmapData>> chBitmaps;
        libCZI::CDimCoordinate coordinate = options.GetPlaneCoordinate();

        auto subBlockStatistics = reader->GetStatistics();

        libCZI::ISingleChannelScalingTileAccessor::Options sctaOptions; sctaOptions.Clear();
        sctaOptions.backGroundColor = GetBackgroundColorFromOptions(options);
        sctaOptions.drawTileBorder = options.GetDrawTileBoundaries();
        sctaOptions.sceneFilter = options.GetSceneIndexSet();
        IntRect roi{ options.GetRectX() ,options.GetRectY() ,options.GetRectW(),options.GetRectH() };
        if (options.GetIsRelativeRectCoordinate())
        {
            roi.x += subBlockStatistics.boundingBox.x;
            roi.y += subBlockStatistics.boundingBox.y;
        }

        auto accessor = reader->CreateSingleChannelScalingTileAccessor();

        float zoom = options.GetZoom();

        for (int i = 0;; ++i)
        {
            int chNo;
            if (getChannelNo(i, chNo) == false)
            {
                break;
            }

            if (subBlockStatistics.dimBounds.IsValid(DimensionIndex::C))
            {
                // That's a cornerstone case - or a loophole in the specification: if the document
                // does not contain C-dimension (=none of the sub-blocks has a valid C-dimension),
                // then we must not set the C-dimension here. I suppose we should define that a
                // valid C-dimension is mandatory...
                coordinate.Set(DimensionIndex::C, chNo);
            }

            chBitmaps.emplace_back(accessor->Get(roi, &coordinate, zoom, &sctaOptions));
        }

        return chBitmaps;
    }
};

class CExecuteExtractAttachment : CExecuteBase
{
private:
    struct SelectionInfo
    {
        SelectionInfo() : nameValid(false), indexValid(false) {}
        bool		nameValid;
        std::string name;
        bool		indexValid;
        int			index;
    };
public:
    static bool execute(const CCmdLineOptions& options)
    {
        SelectionInfo selectionInfo = CreateSelectionInfo(options);

        auto spReader = CreateAndOpenCziReader(options);

        spReader->EnumerateAttachments(
            [&](int index, const AttachmentInfo& info)->bool
            {
                if (IsSelection(index, info, selectionInfo))
                {
                    auto filename = GenerateFilename(index, info, options);
                    auto attchmnt = spReader->ReadAttachment(index);
                    WriteFile(filename, attchmnt.get());
                    HandleHashOfResult(
                        [&](uint8_t* ptrHash, size_t sizeHash)->bool
                        {
                            const void* ptr; size_t size;
                    attchmnt->DangerousGetRawData(ptr, size);
                    Utils::CalcMd5SumHash(ptr, size, ptrHash, (int)sizeHash);
                    return true;
                        },
                        options);
                }

        return true;
            });

        return true;
    }
private:
    static SelectionInfo CreateSelectionInfo(const CCmdLineOptions& options)
    {
        SelectionInfo si;
        std::string name;
        ItemValue iv = options.GetSelectionItemValue(ItemValue::SelectionItem_Name);
        si.nameValid = iv.TryGetString(&si.name);
        iv = options.GetSelectionItemValue(ItemValue::SelectionItem_Index);
        double value;
        si.indexValid = iv.TryGetNumber(&value);
        if (si.indexValid)
        {
            si.index = (int)value;
        }

        return si;
    }

    static bool IsSelection(int index, const AttachmentInfo& info, const SelectionInfo& selectionInfo)
    {
        bool rv = true;
        if (selectionInfo.nameValid)
        {
            if (strcmp(selectionInfo.name.c_str(), info.name.c_str()) != 0)
            {
                rv = false;
            }
        }

        if (rv == true && selectionInfo.indexValid == true)
        {
            if (selectionInfo.index != index)
            {
                rv = false;
            }
        }

        return rv;
    }

    static std::wstring GenerateFilename(int index, const AttachmentInfo& info, const CCmdLineOptions& options)
    {
        std::wstring extension;
        if (strlen(info.contentFileType) == 0)
        {
            extension = L"XXX";
        }
        else
        {
            extension = convertUtf8ToUCS2(info.contentFileType);
        }

        std::wstring suffix(L"_");
        if (info.name.length() > 0)
        {
            suffix += convertUtf8ToUCS2(info.name);
            suffix += L'_';
        }

        suffix += std::to_wstring(index);

        std::wstring outputfilename = options.MakeOutputFilename(suffix.c_str(), extension.c_str());
        return outputfilename;
    }

    static void WriteFile(const wstring& filename, IAttachment* attchment)
    {
        size_t size;
        auto spData = attchment->GetRawData(&size);

        std::ofstream  output;
        output.exceptions(std::ifstream::badbit | std::ifstream::failbit);
#if defined(WIN32ENV)
        output.open(filename, ios::out | ios::binary);
#endif
#if defined(LINUXENV)
        output.open(convertToUtf8(filename), ios::out | ios::binary);
#endif
        output.write(static_cast<const char*>(spData.get()), size);
        output.close();
    }
};

class CExecuteExtractSubBlock : CExecuteBase
{
private:
    struct SelectionInfo
    {
        SelectionInfo() : indexValid(false) {}
        bool		indexValid;
        int			index;
    };
public:
    static bool execute(const CCmdLineOptions& options)
    {
        SelectionInfo selectionInfo = CreateSelectionInfo(options);

        auto spReader = CreateAndOpenCziReader(options);

        spReader->EnumerateSubBlocks(
            [&](int index, const SubBlockInfo& info)->bool
            {
                if (IsSelection(index, info, selectionInfo))
                {
                    auto subBlk = spReader->ReadSubBlock(index);
                    auto bm = subBlk->CreateBitmap();
                    WriteImage(index, bm.get(), options);
                    HandleHashOfResult(
                        [&](uint8_t* ptrHash, size_t sizeHash)->bool
                        {
                            Utils::CalcMd5SumHash(bm.get(), ptrHash, (int)sizeHash);
                    return true;
                        },
                        options);
                }

        return true;
            });

        return true;
    }
private:
    static SelectionInfo CreateSelectionInfo(const CCmdLineOptions& options)
    {
        SelectionInfo si;
        std::string name;
        ItemValue iv = options.GetSelectionItemValue(ItemValue::SelectionItem_Index);
        double value;
        si.indexValid = iv.TryGetNumber(&value);
        if (si.indexValid)
        {
            si.index = (int)value;
        }

        return si;
    }

    static bool IsSelection(int index, const SubBlockInfo& info, const SelectionInfo& selectionInfo)
    {
        bool rv = true;
        if (selectionInfo.indexValid == true)
        {
            if (selectionInfo.index != index)
            {
                rv = false;
            }
        }

        return rv;
    }

    static void WriteImage(int index, IBitmapData* bm, const CCmdLineOptions& options)
    {
        std::wstring suffix(L"#");
        suffix += std::to_wstring(index);
        std::wstring outputfilename = options.MakeOutputFilename(suffix.c_str(), L"PNG");

        auto saver = CSaveBitmapFactory::CreateSaveBitmapObj(nullptr);
        saver->Save(outputfilename.c_str(), SaveDataFormat::PNG, bm);
    }
};

bool execute(const CCmdLineOptions& options)
{
    bool success = true;
    try
    {
        switch (options.GetCommand())
        {
        case Command::PrintInformation:
            success = CExecutePrintInformation::execute(options);
            break;
        case Command::SingleChannelTileAccessor:
            success = CExecuteSingleChannelTileAccessor::execute(options);
            break;
        case Command::ChannelComposite:
            success = CExecuteChannelComposite::execute(options);
            break;
        case Command::SingleChannelPyramidTileAccessor:
            success = CExecuteSingleChannelPyramidTileAccessor::execute(options);
            break;
        case Command::SingleChannelScalingTileAccessor:
            success = CExecuteSingleChannelScalingTileAccessor::execute(options);
            break;
        case Command::ScalingChannelComposite:
            success = CExecuteScalingChannelComposite::execute(options);
            break;
        case Command::ExtractAttachment:
            success = CExecuteExtractAttachment::execute(options);
            break;
        case Command::ExtractSubBlock:
            success = CExecuteExtractSubBlock::execute(options);
            break;
        case Command::CreateCZI:
            success = executeCreateCzi(options);
            break;
        default:
            break;
        }
    }
    catch (std::exception& excp)
    {
        wstringstream ss;
        string what(excp.what() != nullptr ? excp.what() : "");
        ss << "FATAL ERROR: std::exception caught" << endl << " -> " << convertUtf8ToUCS2(what);
        options.GetLog()->WriteLineStdErr(ss.str());
        success = false;
    }

    return success;
}
