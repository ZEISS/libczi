// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <cmath>
#include <type_traits>
#include "consoleio.h"
#include "inc_libCZI.h"
#include "utils.h"

enum class Command
{
    Invalid,

    PrintInformation,

    ExtractSubBlock,

    SingleChannelTileAccessor,

    ChannelComposite,

    SingleChannelPyramidTileAccessor,

    SingleChannelScalingTileAccessor,

    ScalingChannelComposite,

    ExtractAttachment,

    CreateCZI,

    ReadWriteCZI,

    PlaneScan,
};

enum class InfoLevel : std::uint32_t
{
    None = 0,

    Statistics = 1,
    RawXML = 2,
    DisplaySettings = 4,
    DisplaySettingsJson = 8,
    AllSubBlocks = 16,
    AttachmentInfo = 32,
    AllAttachments = 64,
    PyramidStatistics = 128,
    GeneralInfo = 256,
    ScalingInfo = 512,

    All = Statistics | RawXML | DisplaySettings | DisplaySettingsJson | AllSubBlocks | AttachmentInfo | AllAttachments | PyramidStatistics | GeneralInfo | ScalingInfo
};

struct ChannelDisplaySettings
{

    float        weight;

    bool         enableTinting;
    libCZI::Compositors::TintingColor tinting;

    float        blackPoint;
    float        whitePoint;

    float        gamma;
    std::vector<std::tuple<double, double>> splinePoints;

    bool IsGammaValid() const
    {
        return !std::isnan(gamma);
    }

    bool IsSplinePointsValid() const
    {
        if (IsGammaValid())
        {
            // gamma takes precedence, if it is valid, we consider it to be "more important"
            return false;
        }

        return this->splinePoints.size() >= 2;
    }

    void Clear()
    {
        this->weight = 0;
        this->enableTinting = false;
        this->blackPoint = 0;
        this->whitePoint = 1;
        this->gamma = std::numeric_limits<float>::quiet_NaN();
        this->splinePoints.clear();
    }
};

struct ItemValue
{
    static const char* SelectionItem_Name;// = "name";
    static const char* SelectionItem_Index;// = "index";

    enum class Type
    {
        Invalid, String, Number, Boolean
    };

    ItemValue() : type(Type::Invalid) {}
    explicit ItemValue(const char* str) : type(Type::String), strValue(str) {}
    explicit ItemValue(const std::string& str) : type(Type::String), strValue(str) {}
    explicit ItemValue(double v) : type(Type::Number), doubleValue(v) {}
    explicit ItemValue(bool b) : type(Type::Boolean), boolValue(b) {}

    Type        type;
    std::string strValue;
    double      doubleValue;
    bool        boolValue;

    bool    IsValid() const { return this->type != Type::Invalid; }
    bool    IsNumber() const { return this->type == Type::Number; }
    bool    IsString() const { return this->type == Type::String; }
    bool    IsBoolean() const { return this->type == Type::Boolean; }

    bool TryGetNumber(double* p) const { if (this->IsNumber()) { if (p != nullptr) { *p = this->doubleValue; } return true; } return false; }
    bool TryGetString(std::string* p) const { if (this->IsString()) { if (p != nullptr) { *p = this->strValue; } return true; } return false; }
    bool TryGetBoolean(bool* p) const { if (this->IsBoolean()) { if (p != nullptr) { *p = this->boolValue; } return true; } return false; }
};

struct CreateTileInfo
{
    uint32_t rows, columns;
    float overlap;

    void Clear() { this->rows = this->columns = 0; this->overlap = 0; }
    bool IsValid()const { return this->rows > 0 && this->columns > 0; }
};

class CCmdLineOptions
{
private:
    std::shared_ptr<ILog> log;

    Command command;
    std::wstring cziFilename;
    std::string source_stream_class;
    std::map<int, libCZI::StreamsFactory::Property> property_bag_for_stream_class;
    libCZI::CDimCoordinate planeCoordinate;

    bool    rectModeAbsoluteOrRelative; // true->absolute, false->relative
    int     rectX, rectY, rectW, rectH;

    std::wstring outputPath;
    std::wstring outputFilename;

    std::map<int, ChannelDisplaySettings> multiChannelCompositeChannelInfos;
    bool useDisplaySettingsFromDocument;

    bool calcHashOfResult;
    bool drawTileBoundaries;
    std::uint32_t enabledOutputLevels;
    bool useWicJxrDecoder;
    libCZI::RgbFloatColor   backGroundColor;

    int pyramidMinificationFactor;
    int pyramidLayerNo;

    float zoom;
    InfoLevel infoLevel;

    libCZI::PixelType channelCompositePixelType;
    std::uint8_t channelCompositeAlphaValue;

    std::map<std::string, ItemValue> mapSelection;
    std::shared_ptr<libCZI::IIndexSet> sceneIndexSet;

    libCZI::CDimBounds createBounds;
    std::tuple<std::uint32_t, std::uint32_t> createSize;
    CreateTileInfo createTileInfo;

    std::wstring fontnameOrFile;
    int fontHeight;

    bool newCziFileGuidValid;
    libCZI::GUID newCziFileGuid;

    std::string bitmapGeneratorClassName;

    std::map<std::string, std::string> sbBlkMetadataKeyValue;
    bool sbBlkMetadataKeyValueValid;

    libCZI::CompressionMode compressionMode;
    std::shared_ptr<libCZI::ICompressParameters> compressionParameters;
    libCZI::PixelType pixelTypeForBitmapGenerator;

    std::uint64_t subBlockCacheSize;    ///< The size of the sub-block cache in bytes.
    std::tuple<std::uint32_t, std::uint32_t> tilesSizeForPlaneScan; ///< The size of the tiles in pixels for the plane scan operation.

    bool useVisibilityCheckOptimization;
public:
    /// Values that represent the result of the "Parse"-operation.
    enum class ParseResult
    {
        OK,     ///< An enum constant representing the result "arguments successfully parsed, operation can start".
        Exit,   ///< An enum constant representing the result "operation complete, the program should now be terminated, e.g. the synopsis was printed".
        Error   ///< An enum constant representing the result "the was an error parsing the command line arguments, program should terminate".
    };

    explicit CCmdLineOptions(std::shared_ptr<ILog> log);

    /// Parses the command line arguments. The arguments are expected to be given in UTF8-encoding.
    /// This method handles some operations like "printing the help text" internally, and in such 
    /// cases (where the is no additional operation to take place), the value 'ParseResult::Exit'
    /// is returned.
    ///
    /// \param          argc    The number of arguments.
    /// \param [in]     argv    An array containing the arguments.
    ///
    /// \returns    An enum indicating the result.
    ParseResult Parse(int argc, char** argv);
    void Clear();

    std::shared_ptr<ILog> GetLog() const { return this->log; }
    Command GetCommand() const { return this->command; }
    const std::wstring& GetCZIFilename() const { return this->cziFilename; }
    const std::string& GetInputStreamClassName() const { return this->source_stream_class; }
    const std::map<int, libCZI::StreamsFactory::Property>& GetInputStreamPropertyBag() const { return this->property_bag_for_stream_class; }
    const libCZI::CDimCoordinate& GetPlaneCoordinate() const { return this->planeCoordinate; }
    const std::map<int, ChannelDisplaySettings>& GetMultiChannelCompositeChannelInfos() const { return this->multiChannelCompositeChannelInfos; }
    bool GetUseDisplaySettingsFromDocument() const { return this->useDisplaySettingsFromDocument; }
    libCZI::IntRect GetRect() const { return libCZI::IntRect{ this->rectX,this->rectY,this->rectW,this->rectH }; }
    bool GetIsAbsoluteRectCoordinate() const { return this->rectModeAbsoluteOrRelative; }
    bool GetIsRelativeRectCoordinate() const { return !this->rectModeAbsoluteOrRelative; }
    int GetRectX() const { return this->rectX; }
    int GetRectY() const { return this->rectY; }
    int GetRectW() const { return this->rectW; }
    int GetRectH() const { return this->rectH; }
    bool GetCalcHashOfResult() const { return this->calcHashOfResult; }
    bool GetDrawTileBoundaries() const { return this->drawTileBoundaries; }
    std::wstring MakeOutputFilename(const wchar_t* suffix, const wchar_t* extension) const;
    std::wstring GetOutputFilename() const { return this->outputFilename; }
    bool GetUseWICJxrDecoder() const { return this->useWicJxrDecoder; }
    libCZI::RgbFloatColor GetBackGroundColor() const { return this->backGroundColor; }
    bool IsLogLevelEnabled(int level) const;
    int GetPyramidInfoMinificationFactor() const { return this->pyramidMinificationFactor; }
    int GetPyramidInfoLayerNo() const { return this->pyramidLayerNo; }
    float GetZoom() const { return this->zoom; }
    InfoLevel GetInfoLevel() const { return this->infoLevel; }
    bool IsInfoLevelEnabled(InfoLevel lvl) const { return (static_cast<std::underlying_type<InfoLevel>::type>(this->infoLevel) & static_cast<std::underlying_type<InfoLevel>::type>(lvl)) != 0; }
    ItemValue GetSelectionItemValue(const char* sz) const;
    std::shared_ptr<libCZI::IIndexSet> GetSceneIndexSet() const;
    libCZI::PixelType GetChannelCompositeOutputPixelType() const { return this->channelCompositePixelType; }
    std::uint8_t GetChannelCompositeOutputAlphaValue() const { return this->channelCompositeAlphaValue; }
    const libCZI::CDimBounds& GetCreateBounds() const { return this->createBounds; }
    const std::tuple<std::uint32_t, std::uint32_t>& GetCreateBitmapSize() const { return this->createSize; }
    const CreateTileInfo& GetCreateTileInfo() const { return this->createTileInfo; }
    std::wstring GetFontNameOrFile() const { return this->fontnameOrFile; }
    int GetFontHeight() const { return this->fontHeight; }
    bool GetIsFileGuidValid()const { return this->newCziFileGuidValid; }
    const libCZI::GUID& GetFileGuid()const { return this->newCziFileGuid; }
    const std::string& GetBitmapGeneratorClassName()const { return this->bitmapGeneratorClassName; }
    const std::map<std::string, std::string>& GetSubBlockKeyValueMetadata()const { return this->sbBlkMetadataKeyValue; }
    bool GetHasSubBlockKeyValueMetadata()const { return this->sbBlkMetadataKeyValueValid; }
    libCZI::CompressionMode GetCompressionMode() const { return this->compressionMode; }
    std::shared_ptr<libCZI::ICompressParameters> GetCompressionParameters() const { return this->compressionParameters; }
    libCZI::PixelType GetPixelGeneratorPixeltype() const { return this->pixelTypeForBitmapGenerator; }
    std::uint64_t GetSubBlockCacheSize() const { return this->subBlockCacheSize; }
    const std::tuple<std::uint32_t, std::uint32_t>& GetTileSizeForPlaneScan() const { return this->tilesSizeForPlaneScan; }
    bool GetUseVisibilityCheckOptimization() const { return this->useVisibilityCheckOptimization; }
private:
    friend struct RegionOfInterestValidator;
    friend struct DisplaySettingsValidator;
    friend struct JpgXrCodecValidator;
    friend struct VerbosityValidator;
    friend struct BackgroundColorValidator;
    friend struct PyramidInfoValidator;
    friend struct InfoLevelValidator;
    friend struct SelectionValidator;
    friend struct TileFilterValidator;
    friend struct ChannelCompositionFormatValidator;
    friend struct CreateBoundsValidator;
    friend struct CreateSubblockSizeValidator;
    friend struct CreateTileInfoValidator;
    friend struct GuidOfCziValidator;
    friend struct BitmapGeneratorValidator;
    friend struct CreateSubblockMetadataValidator;
    friend struct CompressionOptionsValidator;
    friend struct GeneratorPixelTypeValidator;
    friend struct CachesizeValidator;
    friend struct TileSizeForPlaneScanValidator;

    bool CheckArgumentConsistency() const;
    void SetOutputFilename(const std::wstring& s);

    void PrintHelpBuildInfo();
    void PrintHelpBitmapGenerator();
    void PrintHelpStreamsObjects();

    static bool TryParseInt32(const std::string& str, int* value);
    static bool TryParseRect(const std::string& str, bool* absolute_mode, int* x_position, int* y_position, int* width, int* height);
    static bool TryParseDisplaySettings(const std::string& s, std::map<int, ChannelDisplaySettings>* multiChannelCompositeChannelInfos);
    static bool TryParseVerbosityLevel(const std::string& s, std::uint32_t* levels);
    static bool TryParseJxrCodecUseWicCodec(const std::string& s, bool* use_wic_codec);
    static bool TryParseParseBackgroundColor(const std::string& s, libCZI::RgbFloatColor* color);
    static bool TryParsePyramidInfo(const std::string& s, int* pyramidMinificationFactor, int* pyramidLayerNo);
    static bool TryParseZoom(const std::string& s, float* zoom);
    static bool TryParseInfoLevel(const std::string& s, InfoLevel* info_level);
    static bool TryParseSelection(const std::string& s, std::map<std::string, ItemValue>* key_value);
    static bool TryParseTileFilter(const std::string& s, std::shared_ptr<libCZI::IIndexSet>* scene_index_set);
    static bool TryParseChannelCompositionFormat(const std::string& s, libCZI::PixelType* channel_composition_format, std::uint8_t* channel_composition_alpha_value);
    static bool TryParseChannelCompositionFormatWithAlphaValue(const std::string& s, libCZI::PixelType& channelCompositePixelType, std::uint8_t& channelCompositeAlphaValue);
    static bool TryParseCreateBounds(const std::string& s, libCZI::CDimBounds* create_bounds);
    static bool TryParseCreateSize(const std::string& s, std::tuple<std::uint32_t, std::uint32_t>* size);
    static bool TryParseCreateTileInfo(const std::string& s, CreateTileInfo* create_tile_info);
    static bool TryParseFontHeight(const std::string& s, int* font_height);
    static bool TryParseNewCziFileguid(const std::string& s, libCZI::GUID* guid);
    static bool TryParseBitmapGenerator(const std::string& s, std::string* generator_class_name);
    static bool TryParseSubBlockMetadataKeyValue(const std::string& s, std::map<std::string, std::string>* subblock_metadata_property_bag);
    static bool TryParseCompressionOptions(const std::string& s, libCZI::Utils::CompressionOption* compression_option);
    static bool TryParseGeneratorPixeltype(const std::string& s, libCZI::PixelType* pixel_type);
    static bool TryParseInputStreamCreationPropertyBag(const std::string& s, std::map<int, libCZI::StreamsFactory::Property>* property_bag);
    static bool TryParseSubBlockCacheSize(const std::string& text, std::uint64_t* size);

    static void ThrowIfFalse(bool b, const std::string& argument_switch, const std::string& argument);
};
