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

	ReadWriteCZI
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

	float		 weight;

	bool		 enableTinting;
	libCZI::Compositors::TintingColor tinting;

	float		 blackPoint;
	float		 whitePoint;

	float		 gamma;
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

	Type		type;
	std::string strValue;
	double      doubleValue;
	bool        boolValue;

	bool	IsValid() const { return this->type != Type::Invalid; }
	bool	IsNumber() const { return this->type == Type::Number; }
	bool	IsString() const { return this->type == Type::String; }
	bool	IsBoolean() const { return this->type == Type::Boolean; }

	bool TryGetNumber(double* p) const { if (this->IsNumber()) { if (p != nullptr) { *p = this->doubleValue; } return true; } return false; }
	bool TryGetString(std::string* p) const { if (this->IsString()) { if (p != nullptr) { *p = this->strValue; } return true; } return false; }
	bool TryGetBoolean(std::string* p) const { if (this->IsBoolean()) { if (p != nullptr) { *p = this->boolValue; } return true; } return false; }
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

	libCZI::CDimCoordinate planeCoordinate;

	bool	rectModeAbsoluteOrRelative;	// true->absolute, false->relative
	int		rectX, rectY, rectW, rectH;

	std::wstring outputPath;
	std::wstring outputFilename;

	std::map<int, ChannelDisplaySettings> multiChannelCompositeChannelInfos;
	bool useDisplaySettingsFromDocument;

	bool calcHashOfResult;

	bool drawTileBoundaries;

	std::uint32_t enabledOutputLevels;

	bool useWicJxrDecoder;

	libCZI::RgbFloatColor	backGroundColor;

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
	GUID newCziFileGuid;

	std::string bitmapGeneratorClassName;

	std::map<std::string, std::string> sbBlkMetadataKeyValue;
	bool sbBlkMetadataKeyValueValid;

	libCZI::CompressionMode	compressionMode;
	std::shared_ptr<libCZI::ICompressParameters> compressionParameters;
	libCZI::PixelType pixelTypeForBitmapGenerator;
public:
	explicit CCmdLineOptions(std::shared_ptr<ILog> log);

	void Clear();

#if defined(WIN32ENV)
	bool Parse(int argc, wchar_t** argv);
#endif
#if defined(LINUXENV)
	bool Parse(int argc, char** argv);
#endif

	std::shared_ptr<ILog> GetLog() const { return this->log; }
	Command GetCommand() const { return this->command; }
	const std::wstring& GetCZIFilename() const { return this->cziFilename; }
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
	bool IsInfoLevelEnabled(InfoLevel lvl) const { return (((std::underlying_type<InfoLevel>::type)this->infoLevel)&((std::underlying_type<InfoLevel>::type)lvl)) != 0; }

	ItemValue GetSelectionItemValue(const char* sz) const;

	std::shared_ptr<libCZI::IIndexSet> GetSceneIndexSet() const;

	libCZI::PixelType GetChannelCompositeOutputPixelType() const { return this->channelCompositePixelType; }
	std::uint8_t GetChannelCompositeOutputAlphaValue() const { return this->channelCompositeAlphaValue; }
	const libCZI::CDimBounds& GetCreateBounds() const { return this->createBounds; }
	const std::tuple<std::uint32_t, std::uint32_t>& GetCreateBitmapSize() const { return this->createSize; }
	const CreateTileInfo& GetCreateTileInfo() const { return this->createTileInfo; }

	std::wstring GetFontNameOrFile()const { return this->fontnameOrFile; }
	int GetFontHeight()const { return this->fontHeight; }

	bool GetIsFileGuidValid()const { return this->newCziFileGuidValid; }
	const GUID& GetFileGuid()const { return this->newCziFileGuid; }

	const std::string& GetBitmapGeneratorClassName()const { return this->bitmapGeneratorClassName; }

	const std::map<std::string, std::string>& GetSubBlockKeyValueMetadata()const { return this->sbBlkMetadataKeyValue; }
	bool GetHasSubBlockKeyValueMetadata()const { return this->sbBlkMetadataKeyValueValid; }

	libCZI::CompressionMode GetCompressionMode() const { return this->compressionMode; }
	std::shared_ptr<libCZI::ICompressParameters> GetCompressionParameters() const { return this->compressionParameters; }
	libCZI::PixelType GetPixelGeneratorPixeltype() const { return this->pixelTypeForBitmapGenerator; }
private:
	void PrintUsage(int switchesCnt, const std::function<std::tuple<int, std::wstring>(int idx)>& getSwitch);
	bool CheckArgumentConsistency() const;

	static Command ParseCommand(const wchar_t* s);
	static Command ParseCommand(const std::string& s) { auto sucs2 = convertUtf8ToUCS2(s); return ParseCommand(sucs2.c_str()); }

	libCZI::CDimCoordinate ParseDimCoordinate(const std::string& s);
	void ParseRect(const std::string& s) { auto sucs2 = convertUtf8ToUCS2(s); this->ParseRect(sucs2); }
	void SetOutputFilename(const std::string& s) { auto sucs2 = convertUtf8ToUCS2(s); this->SetOutputFilename(sucs2); }
	libCZI::CDimCoordinate ParseDimCoordinate(const std::wstring& str);
	void ParseRect(const std::wstring& s);
	void SetOutputFilename(const std::wstring& s);

	void ParseDisplaySettings(const std::wstring& s);
	void ParseDisplaySettings(const std::string& s);

	std::uint32_t ParseVerbosityLevel(const std::string& s) { auto sucs2 = convertUtf8ToUCS2(s); return ParseVerbosityLevel(sucs2.c_str()); }
	std::uint32_t ParseVerbosityLevel(const wchar_t* s);

	bool ParseJxrCodec(const std::string& s) { auto sucs2 = convertUtf8ToUCS2(s); return ParseJxrCodec(sucs2.c_str()); }
	bool ParseJxrCodec(const wchar_t* s);

	void PrintSynopsis(int switchesCnt, std::function<std::tuple<int, std::wstring>(int idx)> getSwitch, std::function<std::tuple<std::wstring, std::wstring>(int shortOption)> getExplanation);

	void PrintHelpBuildInfo();
	void PrintHelp(int switchesCnt, const std::function<std::tuple<int, std::wstring>(int idx)>& getSwitch);
	void PrintHelp(const wchar_t* sz, int switchesCnt, const std::function<std::tuple<int, std::wstring>(int idx)>& getSwitch);
	void PrintHelp(const char* s, int switchesCnt, const std::function<std::tuple<int, std::wstring>(int idx)>& getSwitch)
	{
		if (s != nullptr) { auto ucs2 = convertUtf8ToUCS2(s); this->PrintHelp(ucs2.c_str(), switchesCnt, getSwitch); }
		else { this->PrintHelp((const wchar_t*)nullptr, switchesCnt, getSwitch); }
	}
	void PrintHelpBitmapGenerator();

	static libCZI::RgbFloatColor ParseBackgroundColor(const wchar_t* s);
	static libCZI::RgbFloatColor ParseBackgroundColor(const std::string& s) { auto sucs2 = convertUtf8ToUCS2(s); return ParseBackgroundColor(sucs2.c_str()); }

	void ParsePyramidInfo(const wchar_t* sz);
	void ParsePyramidInfo(const std::string& s) { auto sucs2 = convertUtf8ToUCS2(s); this->ParsePyramidInfo(sucs2.c_str()); }

	void ParseZoom(const wchar_t* sz);
	void ParseZoom(const std::string& s) { auto sucs2 = convertUtf8ToUCS2(s); this->ParseZoom(sucs2.c_str()); }

	void ParseInfoLevel(const wchar_t* s);
	void ParseInfoLevel(const std::string& s) { auto sucs2 = convertUtf8ToUCS2(s); this->ParseInfoLevel(sucs2.c_str()); }

	void ParseSelection(const std::wstring& s);
	void ParseSelection(const std::string& s);

	void ParseTileFilter(const wchar_t* s);
	void ParseTileFilter(const std::string& s) { auto sucs2 = convertUtf8ToUCS2(s); this->ParseTileFilter(sucs2.c_str()); }

	void ParseChannelCompositionFormat(const wchar_t* s);
	void ParseChannelCompositionFormat(const std::string& s) { auto sucs2 = convertUtf8ToUCS2(s); this->ParseChannelCompositionFormat(sucs2.c_str()); }

	static bool TryParseChannelCompositionFormatWithAlphaValue(const std::wstring& s, libCZI::PixelType& channelCompositePixelType, std::uint8_t& channelCompositeAlphaValue);

	void ParseCreateBounds(const std::string& s) { auto sucs2 = convertUtf8ToUCS2(s); this->ParseCreateBounds(sucs2); }
	void ParseCreateBounds(const std::wstring& s);

	void ParseCreateSize(const std::string& s) { auto sucs2 = convertUtf8ToUCS2(s); this->ParseCreateSize(sucs2); }
	void ParseCreateSize(const std::wstring& s);

	void ParseCreateTileInfo(const std::string& s) { auto sucs2 = convertUtf8ToUCS2(s); this->ParseCreateTileInfo(sucs2); }
	void ParseCreateTileInfo(const std::wstring& s);

	void ParseFont(const std::wstring& s);
	void ParseFont(const std::string& s) { auto sucs2 = convertUtf8ToUCS2(s); this->ParseFont(sucs2); }

	void ParseFontHeight(const std::wstring& s);
	void ParseFontHeight(const std::string& s) { auto sucs2 = convertUtf8ToUCS2(s); this->ParseFontHeight(sucs2); }

	void ParseNewCziFileguid(const std::wstring& s);
	void ParseNewCziFileguid(const std::string& s) { auto sucs2 = convertUtf8ToUCS2(s); this->ParseNewCziFileguid(sucs2); }

	void ParseBitmapGenerator(const std::wstring& s);
	void ParseBitmapGenerator(const std::string& s) { auto sucs2 = convertUtf8ToUCS2(s); this->ParseBitmapGenerator(sucs2); }

	void ParseSubBlockMetadataKeyValue(const std::wstring& s) { auto str = convertToUtf8(s); this->ParseSubBlockMetadataKeyValue(str); }
	void ParseSubBlockMetadataKeyValue(const std::string& s);

	void ParseCompressionOptions(const std::wstring& s) { auto str = convertToUtf8(s); this->ParseCompressionOptions(str); }
	void ParseCompressionOptions(const std::string& s);

	void ParseGeneratorPixeltype(const std::wstring& s) { auto str = convertToUtf8(s); this->ParseGeneratorPixeltype(str); }
	void ParseGeneratorPixeltype(const std::string& s);
};
