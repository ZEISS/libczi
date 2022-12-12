// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "stdafx.h"
#include "cmdlineoptions.h"
#include "getOpt.h"

#include <clocale>
#include <locale>
#include <regex>
#include <iostream>
#include <utility>
#include <cstring>
#if defined(LINUXENV)
#include <libgen.h>
#endif
#include "inc_rapidjson.h"
#include "IBitmapGen.h"
#include <iomanip>

#include <CLI/App.hpp>
#include <CLI/Formatter.hpp>
#include <CLI/Config.hpp>

using namespace std;

/*static*/const char* ItemValue::SelectionItem_Name = "name";
/*static*/const char* ItemValue::SelectionItem_Index = "index";

CCmdLineOptions::CCmdLineOptions(std::shared_ptr<ILog> log)
    : log(std::move(log))
{
    this->Clear();
}

/// CLI11-validator for the option "--plane-coordinate".
struct PlaneCoordinateValidator : public CLI::Validator
{
    PlaneCoordinateValidator()
    {
        this->name_ = "PlaneCoordinate";
        this->func_ = [](const std::string& str) -> string
        {
            try
            {
                auto plane_coordinate = libCZI::CDimCoordinate::Parse(str.c_str());
                return {}; // returning "" means "validation was successful" (c.f. https://cliutils.github.io/CLI11/book/chapters/validators.html)
            }
            catch (libCZI::LibCZIStringParseException&)
            {
                ostringstream string_stream;
                string_stream << "Invalid coordinate given \"" << str << "\"";
                throw CLI::ValidationError(string_stream.str());
            }
        };
    }
};

/// CLI11-validator for the option "--rect".
struct RegionOfInterestValidator : public CLI::Validator
{
    RegionOfInterestValidator()
    {
        this->name_ = "RegionOfInterest";
        this->func_ = [](const std::string& str) -> string
        {
            const bool parsed_ok = CCmdLineOptions::TryParseRect(str, nullptr, nullptr, nullptr, nullptr, nullptr);
            if (!parsed_ok)
            {
                ostringstream string_stream;
                string_stream << "Invalid ROI given \"" << str << "\"";
                throw CLI::ValidationError(string_stream.str());
            }

            return {};
        };
    }
};

/// CLI11-validator for the option "--display-settings".
struct DisplaySettingsValidator : public CLI::Validator
{
    DisplaySettingsValidator()
    {
        this->name_ = "DisplaySettings";
        this->func_ = [](const std::string& str) -> string
        {
            const bool parsed_ok = CCmdLineOptions::TryParseDisplaySettings(str, nullptr);
            if (!parsed_ok)
            {
                ostringstream string_stream;
                string_stream << "Invalid DisplaySettings (JSON) given \"" << str << "\"";
                throw CLI::ValidationError(string_stream.str());
            }

            return {};
        };
    }
};

/// CLI11-validator for the option "--jpgxrcodec".
struct JpgXrCodecValidator : public CLI::Validator
{
    JpgXrCodecValidator()
    {
        this->name_ = "JpgXrCodecValidator";
        this->func_ = [](const std::string& str) -> string
        {
            const bool parsed_ok = CCmdLineOptions::TryParseJxrCodecUseWicCodec(str, nullptr);
            if (!parsed_ok)
            {
                ostringstream string_stream;
                string_stream << "Invalid JPGXR-decoder-name given \"" << str << "\"";
                throw CLI::ValidationError(string_stream.str());
            }

            return {};
        };
    }
};

/// CLI11-validator for the option "--verbosity".
struct VerbosityValidator : public CLI::Validator
{
    VerbosityValidator()
    {
        this->name_ = "VerbosityValidator";
        this->func_ = [](const std::string& str) -> string
        {
            const bool parsed_ok = CCmdLineOptions::TryParseVerbosityLevel(str, nullptr);
            if (!parsed_ok)
            {
                ostringstream string_stream;
                string_stream << "Invalid verbosity given \"" << str << "\"";
                throw CLI::ValidationError(string_stream.str());
            }

            return {};
        };
    }
};

/// CLI11-validator for the option "--background".
struct BackgroundColorValidator : public CLI::Validator
{
    BackgroundColorValidator()
    {
        this->name_ = "BackgroundColorValidator";
        this->func_ = [](const std::string& str) -> string
        {
            const bool parsed_ok = CCmdLineOptions::TryParseParseBackgroundColor(str, nullptr);
            if (!parsed_ok)
            {
                ostringstream string_stream;
                string_stream << "Invalid background-color given \"" << str << "\"";
                throw CLI::ValidationError(string_stream.str());
            }

            return {};
        };
    }
};

/// CLI11-validator for the option "--pyramidinfo".
struct PyramidInfoValidator : public CLI::Validator
{
    PyramidInfoValidator()
    {
        this->name_ = "PyramidInfoValidator";
        this->func_ = [](const std::string& str) -> string
        {
            const bool parsed_ok = CCmdLineOptions::TryParsePyramidInfo(str, nullptr, nullptr);
            if (!parsed_ok)
            {
                ostringstream string_stream;
                string_stream << "Invalid pyramid-info given \"" << str << "\"";
                throw CLI::ValidationError(string_stream.str());
            }

            return {};
        };
    }
};

/// CLI11-validator for the option "--info-level".
struct InfoLevelValidator : public CLI::Validator
{
    InfoLevelValidator()
    {
        this->name_ = "InfoLevelValidator";
        this->func_ = [](const std::string& str) -> string
        {
            const bool parsed_ok = CCmdLineOptions::TryParseInfoLevel(str, nullptr);
            if (!parsed_ok)
            {
                ostringstream string_stream;
                string_stream << "Invalid info-level given \"" << str << "\"";
                throw CLI::ValidationError(string_stream.str());
            }

            return {};
        };
    }
};

/// CLI11-validator for the option "--selection".
struct SelectionValidator : public CLI::Validator
{
    SelectionValidator()
    {
        this->name_ = "SelectionValidator";
        this->func_ = [](const std::string& str) -> string
        {
            const bool parsed_ok = CCmdLineOptions::TryParseSelection(str, nullptr);
            if (!parsed_ok)
            {
                ostringstream string_stream;
                string_stream << "Invalid selection given \"" << str << "\"";
                throw CLI::ValidationError(string_stream.str());
            }

            return {};
        };
    }
};

/// CLI11-validator for the option "--tile-filter".
struct TileFilterValidator : public CLI::Validator
{
    TileFilterValidator()
    {
        this->name_ = "TileFilterValidator";
        this->func_ = [](const std::string& str) -> string
        {
            const bool parsed_ok = CCmdLineOptions::TryParseTileFilter(str, nullptr);
            if (!parsed_ok)
            {
                ostringstream string_stream;
                string_stream << "Invalid tile-filter given \"" << str << "\"";
                throw CLI::ValidationError(string_stream.str());
            }

            return {};
        };
    }
};

/// CLI11-validator for the option "--channelcompositionformat".
struct ChannelCompositionFormatValidator : public CLI::Validator
{
    ChannelCompositionFormatValidator()
    {
        this->name_ = "ChannelCompositionFormatValidator";
        this->func_ = [](const std::string& str) -> string
        {
            const bool parsed_ok = CCmdLineOptions::TryParseChannelCompositionFormat(str, nullptr, nullptr);
            if (!parsed_ok)
            {
                ostringstream string_stream;
                string_stream << "Invalid channel-composition-format given \"" << str << "\"";
                throw CLI::ValidationError(string_stream.str());
            }

            return {};
        };
    }
};

/// CLI11-validator for the option "--createbounds".
struct CreateBoundsValidator : public CLI::Validator
{
    CreateBoundsValidator()
    {
        this->name_ = "CreateBoundsValidator";
        this->func_ = [](const std::string& str) -> string
        {
            const bool parsed_ok = CCmdLineOptions::TryParseCreateBounds(str, nullptr);
            if (!parsed_ok)
            {
                ostringstream string_stream;
                string_stream << "Invalid create-bounds given \"" << str << "\"";
                throw CLI::ValidationError(string_stream.str());
            }

            return {};
        };
    }
};

/// CLI11-validator for the option "--createsubblocksize".
struct CreateSubblockSizeValidator : public CLI::Validator
{
    CreateSubblockSizeValidator()
    {
        this->name_ = "CreateSubblockSizeValidator";
        this->func_ = [](const std::string& str) -> string
        {
            const bool parsed_ok = CCmdLineOptions::TryParseCreateSize(str, nullptr);
            if (!parsed_ok)
            {
                ostringstream string_stream;
                string_stream << "Invalid create-subblock-size given \"" << str << "\"";
                throw CLI::ValidationError(string_stream.str());
            }

            return {};
        };
    }
};

/// CLI11-validator for the option "--createtileinfo".
struct CreateTileInfoValidator : public CLI::Validator
{
    CreateTileInfoValidator()
    {
        this->name_ = "CreateTileInfoValidator";
        this->func_ = [](const std::string& str) -> string
        {
            const bool parsed_ok = CCmdLineOptions::TryParseCreateTileInfo(str, nullptr);
            if (!parsed_ok)
            {
                ostringstream string_stream;
                string_stream << "Invalid create-tileinfo given \"" << str << "\"";
                throw CLI::ValidationError(string_stream.str());
            }

            return {};
        };
    }
};

/// CLI11-validator for the option "--guidofczi".
struct GuidOfCziValidator : public CLI::Validator
{
    GuidOfCziValidator()
    {
        this->name_ = "GuidOfCziValidator";
        this->func_ = [](const std::string& str) -> string
        {
            const bool parsed_ok = CCmdLineOptions::TryParseNewCziFileguid(str, nullptr);
            if (!parsed_ok)
            {
                ostringstream string_stream;
                string_stream << "Invalid GUID-of-CZI given \"" << str << "\"";
                throw CLI::ValidationError(string_stream.str());
            }

            return {};
        };
    }
};

/// CLI11-validator for the option "--bitmapgenerator".
struct BitmapGeneratorValidator : public CLI::Validator
{
    BitmapGeneratorValidator()
    {
        this->name_ = "BitmapGeneratorValidator";
        this->func_ = [](const std::string& str) -> string
        {
            const bool parsed_ok = CCmdLineOptions::TryParseBitmapGenerator(str, nullptr);
            if (!parsed_ok)
            {
                ostringstream string_stream;
                string_stream << "Invalid bitmapgenerator-classname given \"" << str << "\"";
                throw CLI::ValidationError(string_stream.str());
            }

            return {};
        };
    }
};

/// CLI11-validator for the option "--createczisbblkmetadata".
struct CreateSubblockMetadataValidator : public CLI::Validator
{
    CreateSubblockMetadataValidator()
    {
        this->name_ = "CreateSubblockMetadataValidator";
        this->func_ = [](const std::string& str) -> string
        {
            const bool parsed_ok = CCmdLineOptions::TryParseSubBlockMetadataKeyValue(str, nullptr);
            if (!parsed_ok)
            {
                ostringstream string_stream;
                string_stream << "Invalid create-subblock-metadata (JSON) given \"" << str << "\"";
                throw CLI::ValidationError(string_stream.str());
            }

            return {};
        };
    }
};

/// CLI11-validator for the option "--compressionopts".
struct CompressionOptionsValidator : public CLI::Validator
{
    CompressionOptionsValidator()
    {
        this->name_ = "CompressionOptionsValidator";
        this->func_ = [](const std::string& str) -> string
        {
            const bool parsed_ok = CCmdLineOptions::TryParseCompressionOptions(str, nullptr);
            if (!parsed_ok)
            {
                ostringstream string_stream;
                string_stream << "Invalid compression-options given \"" << str << "\"";
                throw CLI::ValidationError(string_stream.str());
            }

            return {};
        };
    }
};

/// CLI11-validator for the option "--generatorpixeltype".
struct GeneratorPixelTypeValidator : public CLI::Validator
{
    GeneratorPixelTypeValidator()
    {
        this->name_ = "GeneratorPixelTypeValidator";
        this->func_ = [](const std::string& str) -> string
        {
            const bool parsed_ok = CCmdLineOptions::TryParseGeneratorPixeltype(str, nullptr);
            if (!parsed_ok)
            {
                ostringstream string_stream;
                string_stream << "Invalid generator-pixel-type given \"" << str << "\"";
                throw CLI::ValidationError(string_stream.str());
            }

            return {};
        };
    }
};

class CustomFormatter : public CLI::Formatter
{
public:
    CustomFormatter() : Formatter() {}
    //std::string make_option_opts(const CLI::Option*) const override { return " OPTION"; }

    //std::string make_option_opts(const CLI::Option* o) const override
    //{
    //    auto rv = this->CLI::Formatter::make_option_opts(o);
    //    return rv;
    //}

    std::string make_option_desc(const CLI::Option* o) const override
    {
        // we wrap the text so that it fits in the "second column"
        const auto lines = wrap(o->get_description().c_str(), 80 - this->get_column_width());

        string options_description;
        options_description.reserve(accumulate(lines.cbegin(), lines.cend(), static_cast<size_t>(0), [](size_t sum, const std::string& str) { return 1 + sum + str.size(); }));
        for (const auto& l : lines)
        {
            options_description.append(l).append("\n");
        }

        return options_description;
    }
};


bool CCmdLineOptions::Parse2(int argc, char** argv)
{
    CLI::App cli_app{ "CZIcmd" };

    // specify the string-to-enum-mapping for "command"
    std::map<string, Command> map_string_to_command
    {
        { "PrintInformation",					Command::PrintInformation },
        { "ExtractSubBlock",					Command::ExtractSubBlock },
        { "SingleChannelTileAccessor",			Command::SingleChannelTileAccessor },
        { "ChannelComposite",					Command::ChannelComposite },
        { "SingleChannelPyramidTileAccessor",	Command::SingleChannelPyramidTileAccessor },
        { "SingleChannelScalingTileAccessor",   Command::SingleChannelScalingTileAccessor },
        { "ScalingChannelComposite",			Command::ScalingChannelComposite },
        { "ExtractAttachment",                  Command::ExtractAttachment},
        { "CreateCZI",							Command::CreateCZI },
    };

    const static PlaneCoordinateValidator plane_coordinate_validator;
    const static RegionOfInterestValidator region_of_interest_validator;
    const static DisplaySettingsValidator display_settings_validator;
    const static JpgXrCodecValidator jpgxr_codec_validator;
    const static VerbosityValidator verbosity_validator;
    const static BackgroundColorValidator background_color_validator;
    const static PyramidInfoValidator pyramidinfo_validator;
    const static InfoLevelValidator infolevel_validator;
    const static SelectionValidator selection_validator;
    const static TileFilterValidator tilefilter_validator;
    const static ChannelCompositionFormatValidator channelcompositionformat_validator;
    const static CreateBoundsValidator createbounds_validator;
    const static CreateSubblockSizeValidator createsubblocksize_validator;
    const static CreateTileInfoValidator createtileinfo_validator;
    const static GuidOfCziValidator guidofczi_validator;
    const static BitmapGeneratorValidator bitmapgenerator_validator;
    const static CreateSubblockMetadataValidator createsubblockmetadata_validator;
    const static CompressionOptionsValidator compressionoptions_validator;
    const static GeneratorPixelTypeValidator generatorpixeltype_validator;

    Command argument_command;
    string argument_source_filename;
    string argument_output_filename;
    string argument_plane_coordinate;
    string argument_rect;
    string argument_display_settings;
    bool argument_calc_hash;
    bool argument_drawtileboundaries;
    string argument_jpgxrcodec;
    string argument_verbosity;
    string argument_backgroundcolor;
    string argument_pyramidinfo;
    string argument_zoom;
    string argument_info_level;
    string argument_selection;
    string argument_tile_filter;
    string argument_channelcompositionformat;
    string arguments_createbounds;
    string arguments_createsubblocksize;
    string argument_createtileinfo;
    string argument_truetypefontname;
    string argument_fontheight;
    string argument_guidofczi;
    string argument_bitmapgenerator;
    string argument_createczisubblockmetadata;
    string argument_compressionoptions;
    string argument_generatorpixeltype;

    cli_app.add_option("-c,--command", argument_command,
        R"(COMMAND can be one of 'PrintInformation', 'ExtractSubBlock', 'SingleChannelTileAccessor', 'ChannelComposite',
    	   'SingleChannelPyramidTileAccessor', 'SingleChannelScalingTileAccessor', 'ScalingChannelComposite', 'ExtractAttachment' and 'CreateCZI'.
    	   \N'PrintInformation' will print information about the CZI-file to the console. The argument 'info-level' can be used
    	   to specify which information is to be printed.
    	   \N'ExtractSubBlock' will write the bitmap contained in the specified sub-block to the OUTPUTFILE.
    	   \N'ChannelComposite' will create a
    	   channel-composite of the specified region and plane and apply display-settings to it. The resulting bitmap will be written
    	   to the specified OUTPUTFILE.
    	   \N'SingleChannelTileAccessor' will create a tile-composite (only from sub-blocks on pyramid-layer 0) of the specified region and plane.
    	   The resulting bitmap will be written to the specified OUTPUTFILE.
    	   \N'SingleChannelPyramidTileAccessor' adds to the previous command the ability to explictely address a specific pyramid-layer (which must
    	   exist in the CZI-document).
    	   \N'SingleChannelScalingTileAccessor' gets the specified region with an arbitrary zoom factor. It uses the pyramid-layers in the CZI-document
    	   and scales the bitmap if neccessary. The resulting bitmap will be written to the specified OUTPUTFILE.
    	   \N'ScalingChannelComposite' operates like the previous command, but in addition gets all channels and creates a multi-channel-composite from them
    	   using display-settings.
    	   \N'ExtractAttachment' allows to extract (and save to a file) the contents of attachments.)
    	   \N'CreateCZI' is used to demonstrate the CZI-creation capabilities of libCZI.)")
        ->default_val(Command::Invalid)
        ->option_text("COMMAND")
        ->required()
        ->transform(CLI::CheckedTransformer(map_string_to_command, CLI::ignore_case));
    cli_app.add_option("-s,--source", argument_source_filename,
        "specifies the source CZI-file.")
        ->option_text("SOURCEFILE")
        ->check(CLI::ExistingFile);
    cli_app.add_option("-o,--output", argument_output_filename,
        "specifies the output-filename. A suffix will be appended to the name given here depending on the type of the file.")
        ->option_text("OUTPUTFILE");
    cli_app.add_option("-p,--plane-coordinate", argument_plane_coordinate,
        R"(Uniquely select a 2D-plane from the document. It is given in the form [DimChar][number], where 'DimChar' specifies a dimension and 
           can be any of 'Z', 'C', 'T', 'R', 'I', 'H', 'V' or 'B'. 'number' is an integer. \nExamples: C1T3, C0T-2, C1T44Z15H1.)")
        ->option_text("PLANE-COORDINATE")
        ->check(plane_coordinate_validator);
    cli_app.add_option("-r,--rect", argument_rect,
        R"(Select a paraxial rectangular region as the region-of-interest. The coordinates may be given either absolute or relative. If using relative
            coordinates, they are relative to what is determined as the upper-left point in the document.\nRelative coordinates are specified with
            the syntax 'rel([x],[y],[width],[height])', absolute coordinates are specified 'abs([x],[y],[width],[height])'.
            \nExamples: rel(0, 0, 1024, 1024), rel(-100, -100, 500, 500), abs(-230, 100, 800, 800).)")
        ->option_text("ROI")
        ->check(region_of_interest_validator);
    cli_app.add_option("-d,--display-settings", argument_display_settings,
        "Specifies the display-settings used for creating a channel-composite. The data is given in JSON-notation.")
        ->option_text("DISPLAYSETTINGS")
        ->check(display_settings_validator);
    cli_app.add_flag("--calc-hash", argument_calc_hash, "Calculate a hash of the output-picture. The MD5Sum-algorithm is used for this.");
    cli_app.add_flag("-t,--drawtileboundaries", argument_drawtileboundaries, "Draw a one-pixel black line around each tile.");
    cli_app.add_option("-j,--jpgxrcodec", argument_jpgxrcodec,
        "Choose which decoder implementation is used. Specifying \"WIC\" will request the Windows-provided decoder - which "
        "is only available on Windows.By default the internal JPG-XR-decoder is used.")
        ->option_text("DECODERNAME")
        ->check(jpgxr_codec_validator);
    cli_app.add_option("-v,--verbosity", argument_verbosity,
        "Set the verbosity of this program. The argument is a comma- or semicolon-separated list of the "
        "following strings : 'All', 'Errors', 'Warnings', 'Infos', 'Errors1', 'Warnings1', 'Infos1', "
        "'Errors2', 'Warnings2', 'Infos2'.")
        ->option_text("VERBOSITYLEVEL")
        ->check(verbosity_validator);
    cli_app.add_option("-b,--background", argument_backgroundcolor,
        "Specify the background color. BACKGROUND is either a single float or three floats, separated by a comma or semicolon. In case of "
        "a single float, it gives a grayscale value, in case of three floats it gives a RGB - value.The floats are given normalized to a range "
        "from 0 to 1.")
        ->option_text("BACKGROUND")
        ->check(background_color_validator);
    cli_app.add_option("-y,--pyramidinfo", argument_pyramidinfo,
        "For the command 'SingleChannelPyramidTileAccessor' the argument PYRAMIDINFO specifies the pyramid layer. It consists of two "
        "integers(separated by a comma, semicolon or pipe-symbol), where the first specifies the minification-factor (between pyramid-layers) and "
        "the second the pyramid-layer (starting with 0 for the layer with the highest resolution).")
        ->option_text("PYRAMIDINFO")
        ->check(pyramidinfo_validator);
    cli_app.add_option("-z,--zoom", argument_zoom,
        "The zoom-factor (which is used for the commands 'SingleChannelScalingTileAccessor' and 'ScalingChannelComposite'). "
        "It is a float between 0 and 1.")
        ->option_text("ZOOM")
        ->check(CLI::Range(0.f, 1.f));
    cli_app.add_option("-i,--info-level", argument_info_level,
        "When using the command 'PrintInformation' the INFO-LEVEL can be used to specify which information is printed. Possible "
        "values are \"Statistics\", \"RawXML\", \"DisplaySettings\", \"DisplaySettingsJson\", \"AllSubBlocks\", \"Attachments\", \"AllAttachments\", "
        "\"PyramidStatistics\", \"GeneralInfo\", \"ScalingInfo\" and \"All\". "
        "The values are given as a list separated by comma or semicolon.")
        ->option_text("INFO-LEVEL")
        ->check(infolevel_validator);
    cli_app.add_option("-e,--selection", argument_selection,
        "For the command 'ExtractAttachment' this allows to specify a subset which is to be extracted (and saved to a file). "
        "It is possible to specify the name and the index - only attachments for which the name/index is equal to those values "
        "specified are processed. The arguments are given in JSON-notation, e.g. {\"name\":\"Thumbnail\"} or {\"index\":3.0}.")
        ->option_text("SELECTION")
        ->check(selection_validator);
    cli_app.add_option("-f,--tile-filter", argument_tile_filter,
        "Specify to filter subblocks according to the scene-index. A comma separated list of either an interval or a single "
        "integer may be given here, e.g. \"2,3\" or \"2-4,6\" or \"0-3,5-8\".")
        ->option_text("FILTER")
        ->check(tilefilter_validator);
    cli_app.add_option("-m,--channelcompositionformat", argument_channelcompositionformat,
        "In case of a channel-composition, specifies the pixeltype of the output. Possible values are \"bgr24\" (the default) and \"bgra32\". "
        "If specifying \"bgra32\" it is possible to give the value of the alpha-pixels in the form \"bgra32(128)\" - for an alpha-value of 128.")
        ->option_text("CHANNELCOMPOSITIONFORMAT")
        ->check(channelcompositionformat_validator);
    cli_app.add_option("--createbounds", arguments_createbounds,
        "Only used for 'CreateCZI': specify the range of coordinates used to create a CZI. Format is e.g. 'T0:3Z0:3C0:2'.")
        ->option_text("BOUNDS")
        ->check(createbounds_validator);
    cli_app.add_option("--createsubblocksize", arguments_createsubblocksize,
        "Only used for 'CreateCZI': specify the size of the subblocks created in pixels. Format is e.g. '1600x1200'.")
        ->option_text("SIZE")
        ->check(createsubblocksize_validator);
    cli_app.add_option("--createtileinfo", argument_createtileinfo,
        "Only used for 'CreateCZI': specify the number of tiles on each plane. Format is e.g. '3x3;10%' for a 3 by 3 tiles arrangement with 10% overlap.")
        ->option_text("TILEINFO")
        ->check(createtileinfo_validator);
    cli_app.add_option("--font", argument_truetypefontname,
        "Only used for 'CreateCZI': (on Linux) specify the filename of a TrueType-font (.ttf) to be used for generating text in the subblocks; (on Windows) name of the font.")
#if defined(LINUXENV)
        ->check(CLI::ExistingFile)
#endif
        ->option_text("NAME/FILENAME");
    cli_app.add_option("--fontheight", argument_fontheight,
        "Only used for 'CreateCZI': specifies the height of the font in pixels (default: 36).")
        ->option_text("HEIGHT")
        ->check(CLI::Range(0.f, 10000.f));
    cli_app.add_option("-g,--guidofczi", argument_guidofczi,
        "Only used for 'CreateCZI': specify the GUID of the file (which is useful for bit-exact reproducible results); the GUID must be "
        "given in the form  \"cfc4a2fe-f968-4ef8-b685-e73d1b77271a\" or \"{cfc4a2fe-f968-4ef8-b685-e73d1b77271a}\"")
        ->option_text("CZI-File-GUID")
        ->check(guidofczi_validator);
    cli_app.add_option("--bitmapgenerator", argument_bitmapgenerator,
        "Only used for 'CreateCZI': specifies the bitmap-generator to use. Possibly values are \"gdi\", \"freetype\", \"null\" or \"default\". "
        "Run with argument '--help=bitmapgen' to get a list of available bitmap-generators.")
        ->option_text("BITMAPGENERATORCLASSNAME")
        ->check(bitmapgenerator_validator);
    cli_app.add_option("--createczisbblkmetadata", argument_createczisubblockmetadata,
        "Only used for 'CreateCZI': a key-value list in JSON-notation which will be written as subblock-metadata. For example: "
        "{\"StageXPosition\":-8906.346,\"StageYPosition\":-648.51}")
        ->option_text("KEY_VALUE_SUBBLOCKMETADATA")
        ->check(createsubblockmetadata_validator);
    cli_app.add_option("--compressionopts", argument_compressionoptions,
        "Only used for 'CreateCZI': a string in a defined format which states the compression-method and (compression-method specific) "
        "parameters.The format is \"compression_method: key=value; ...\". It starts with the name of the compression-method, followed by a colon, "
        "then followed by a list of key-value pairs which are separated by a semicolon.Examples: \"zstd0:ExplicitLevel=3\", \"zstd1:ExplicitLevel=2;PreProcess=HiLoByteUnpack\".")
        ->option_text("COMPRESSIONDESCRIPTION")
        ->check(compressionoptions_validator);
    cli_app.add_option("--generatorpixeltype", argument_generatorpixeltype,
        "Only used for 'CreateCZI': a string defining the pixeltype used by the bitmap - generator.Possible valules are 'Gray8', 'Gray16', "
        "'Bgr24' or 'Bgr48'.Default is 'Bgr24'.")
        ->option_text("PIXELTYPE")
        ->check(generatorpixeltype_validator);

    auto formatter = make_shared<CustomFormatter>();
    cli_app.formatter(formatter);

    try
    {
        cli_app.parse(argc, argv);
    }
    catch (const CLI::ParseError& e)
    {
        cli_app.exit(e);
        return false;
    }

    return false;
}

#if defined(WIN32ENV)
bool CCmdLineOptions::Parse(int argc, wchar_t** argv)
#endif
#if defined(LINUXENV)
bool CCmdLineOptions::Parse(int argc, char** argv)
#endif
{
#if defined(WIN32ENV)
#define OPTTEXTSPEC(x) L##x
    static const struct optionW long_options[] =
#endif
#if defined(LINUXENV)
#define OPTTEXTSPEC(x) x
        static const struct option long_options[] =
#endif
    {
        { OPTTEXTSPEC("help"),						 optional_argument,	0, OPTTEXTSPEC('?') },
        { OPTTEXTSPEC("command"),					 required_argument, 0, OPTTEXTSPEC('c') },
        { OPTTEXTSPEC("source"),					 required_argument, 0, OPTTEXTSPEC('s') },
        { OPTTEXTSPEC("output"),					 required_argument, 0, OPTTEXTSPEC('o') },
        { OPTTEXTSPEC("plane-coordinate"),			 required_argument, 0, OPTTEXTSPEC('p') },
        { OPTTEXTSPEC("rect"),						 required_argument, 0, OPTTEXTSPEC('r') },
        { OPTTEXTSPEC("display-settings"),			 required_argument, 0, OPTTEXTSPEC('d') },
        { OPTTEXTSPEC("calc-hash"),					 no_argument,		0, OPTTEXTSPEC('h') },
        { OPTTEXTSPEC("drawtileboundaries"),		 no_argument,		0, OPTTEXTSPEC('t') },
        { OPTTEXTSPEC("jpgxrcodec"),				 required_argument, 0, OPTTEXTSPEC('j') },
        { OPTTEXTSPEC("verbosity"),					 required_argument, 0, OPTTEXTSPEC('v') },
        { OPTTEXTSPEC("background"),				 required_argument, 0, OPTTEXTSPEC('b') },
        { OPTTEXTSPEC("pyramidinfo"),				 required_argument, 0, OPTTEXTSPEC('y') },
        { OPTTEXTSPEC("zoom"),						 required_argument, 0, OPTTEXTSPEC('z') },
        { OPTTEXTSPEC("info-level"),				 required_argument, 0, OPTTEXTSPEC('i') },
        { OPTTEXTSPEC("selection"),					 required_argument, 0, OPTTEXTSPEC('e') },
        { OPTTEXTSPEC("tile-filter"),				 required_argument, 0, OPTTEXTSPEC('f') },
        { OPTTEXTSPEC("channelcompositionformat"),	 required_argument, 0, OPTTEXTSPEC('m') },
        { OPTTEXTSPEC("createbounds"),				 required_argument, 0, 256},
        { OPTTEXTSPEC("createsubblocksize"),		 required_argument, 0, 257 },
        { OPTTEXTSPEC("createtileinfo"),			 required_argument, 0, 258 },
        { OPTTEXTSPEC("font"),						 required_argument, 0, 259 },
        { OPTTEXTSPEC("fontheight"),				 required_argument, 0, 260 },
        { OPTTEXTSPEC("guidofczi"),					 required_argument, 0, OPTTEXTSPEC('g') },
        { OPTTEXTSPEC("bitmapgenerator"),			 required_argument, 0, 261 },
        { OPTTEXTSPEC("createczisbblkmetadata"),     required_argument, 0, 262 },
        { OPTTEXTSPEC("compressionopts"),			 required_argument, 0, 263 },
        { OPTTEXTSPEC("generatorpixeltype"),		 required_argument, 0, 264 },
        { 0, 0, 0, 0 }
    };

#undef OPTTEXTSPEC

    for (;;)
    {
        int option_index;
#if defined(WIN32ENV)
        int c = getoptW_long(argc, argv, L"?v:j:s:c:p:r:o:d:htb:y:z:i:e:f:m:g:", long_options, &option_index);
#endif
#if defined(LINUXENV)
        int c = getopt_long(argc, argv, "?v:j:s:c:p:r:o:d:htb:y:z:i:e:f:m:g:", long_options, &option_index);
#endif
        if (c == -1)
        {
            break;
        }

        switch (c)
        {
        case 'c':
            this->command = CCmdLineOptions::ParseCommand(optarg);
            break;
        case 's':
#if defined(WIN32ENV)
            this->cziFilename = optarg;
#endif
#if defined(LINUXENV)
            this->cziFilename = convertUtf8ToUCS2(optarg);
#endif
            break;
        case '?':
            this->PrintHelp(
                optarg,
                sizeof(long_options) / sizeof(long_options[0]) - 1,
                [&](int idx) ->	tuple<int, wstring>
                {
#if defined(WIN32ENV)
                    return make_tuple(long_options[idx].val, wstring(long_options[idx].name));
#endif
#if defined(LINUXENV)
            return make_tuple(long_options[idx].val, convertUtf8ToUCS2(long_options[idx].name));
#endif
                });
            return true;
        case 'p':
            this->planeCoordinate = this->ParseDimCoordinate(optarg);
            break;
        case 'r':
            this->ParseRect(optarg);
            break;
        case 'o':
            this->SetOutputFilename(optarg);
            break;
        case 'd':
            this->ParseDisplaySettings(optarg);
            this->useDisplaySettingsFromDocument = false;
            break;
        case 'h':
            this->calcHashOfResult = true;
            break;
        case 't':
            this->drawTileBoundaries = true;
            break;
        case 'v':
            this->enabledOutputLevels = this->ParseVerbosityLevel(optarg);
            break;
        case 'j':
            this->useWicJxrDecoder = this->ParseJxrCodec(optarg);
            break;
        case 'b':
            this->backGroundColor = ParseBackgroundColor(optarg);
            break;
        case 'y':
            this->ParsePyramidInfo(optarg);
            break;
        case 'z':
            this->ParseZoom(optarg);
            break;
        case 'i':
            this->ParseInfoLevel(optarg);
            break;
        case 'e':
            this->ParseSelection(optarg);
            break;
        case 'f':
            this->ParseTileFilter(optarg);
            break;
        case 'm':
            this->ParseChannelCompositionFormat(optarg);
            break;
        case 256:
            this->ParseCreateBounds(optarg);
            break;
        case 257:
            this->ParseCreateSize(optarg);
            break;
        case 258:
            this->ParseCreateTileInfo(optarg);
            break;
        case 259:
            this->ParseFont(optarg);
            break;
        case 260:
            this->ParseFontHeight(optarg);
            break;
        case 'g':
            this->ParseNewCziFileguid(optarg);
            break;
        case 261:
            this->ParseBitmapGenerator(optarg);
            break;
        case 262:
            this->ParseSubBlockMetadataKeyValue(optarg);
            break;
        case 263:
            this->ParseCompressionOptions(optarg);
            break;
        case 264:
            this->ParseGeneratorPixeltype(optarg);
            break;
        default:
            break;
        }
    }

    return this->CheckArgumentConsistency();
}

bool CCmdLineOptions::CheckArgumentConsistency() const
{
    stringstream ss;
    static const char* ERRORPREFIX = "Argument error: ";
    Command cmd = this->GetCommand();
    if (cmd == Command::Invalid)
    {
        ss << ERRORPREFIX << "no command specified";
        this->GetLog()->WriteLineStdErr(ss.str());
        return false;
    }

    // in all other cases we need the "source" argument
    if ((cmd != Command::CreateCZI && cmd != Command::ReadWriteCZI) && this->GetCZIFilename().empty())
    {
        ss << ERRORPREFIX << "no source file specified";
        this->GetLog()->WriteLineStdErr(ss.str());
        return false;
    }

    if (cmd != Command::PrintInformation)
    {
        auto str = this->MakeOutputFilename(nullptr, nullptr);
        if (str.empty())
        {
            ss << ERRORPREFIX << "no output file specified";
            this->GetLog()->WriteLineStdErr(ss.str());
            return false;
        }
    }

    switch (cmd)
    {
    case Command::ScalingChannelComposite:
    case Command::SingleChannelScalingTileAccessor:
        if (this->GetZoom() <= 0)
        {
            ss << ERRORPREFIX << "no valid zoom specified";
            this->GetLog()->WriteLineStdErr(ss.str());
            return false;
        }

        break;
    default:
        break;
    }

    switch (cmd)
    {
    case Command::SingleChannelTileAccessor:
    case Command::ChannelComposite:
    case Command::SingleChannelPyramidTileAccessor:
    case Command::SingleChannelScalingTileAccessor:
    case Command::ScalingChannelComposite:
        if (this->GetRectW() <= 0 || this->GetRectH() <= 0)
        {
            ss << ERRORPREFIX << "no valid ROI specified";
            this->GetLog()->WriteLineStdErr(ss.str());
            return false;
        }

        break;
    default:
        break;
    }

    switch (cmd)
    {
    case Command::SingleChannelPyramidTileAccessor:
        if (this->GetPyramidInfoMinificationFactor() <= 0 || this->GetPyramidInfoLayerNo() < 0)
        {
            ss << ERRORPREFIX << "no valid PYRAMIDINFO specified";
            this->GetLog()->WriteLineStdErr(ss.str());
            return false;
        }

        break;
    default:
        break;
    }

    // TODO: there is probably more to be checked

    return true;
}

void CCmdLineOptions::Clear()
{
    this->command = Command::Invalid;
    this->useDisplaySettingsFromDocument = true;
    this->calcHashOfResult = false;
    this->drawTileBoundaries = false;
    this->enabledOutputLevels = 0;
    this->useWicJxrDecoder = false;
    this->backGroundColor.r = this->backGroundColor.g = this->backGroundColor.b = numeric_limits<float>::quiet_NaN();
    this->infoLevel = InfoLevel::Statistics;
    this->channelCompositePixelType = libCZI::PixelType::Bgr24;
    this->channelCompositeAlphaValue = 0xff;
    this->createSize = make_tuple(1200, 1000);
    this->fontnameOrFile.clear();
    this->fontHeight = 36;
    this->newCziFileGuidValid = false;
    this->sbBlkMetadataKeyValueValid = false;
    this->sbBlkMetadataKeyValue.clear();
    this->rectX = this->rectY = 0;
    this->rectW = this->rectH = -1;;
    this->zoom = -1;
    this->pyramidLayerNo = -1;
    this->pyramidMinificationFactor = -1;
    this->createTileInfo.rows = this->createTileInfo.columns = 1;
    this->createTileInfo.overlap = 0;
    this->compressionMode = libCZI::CompressionMode::Invalid;
    this->compressionParameters = nullptr;
    this->pixelTypeForBitmapGenerator = libCZI::PixelType::Bgr24;
}

void CCmdLineOptions::PrintUsage(int switchesCnt, const std::function<std::tuple<int, std::wstring>(int idx)>& getSwitch)
{
    static const char* Synopsis[] =
    {
        "usage: CZIcmd -c COMMAND -s SOURCEFILE -o OUTPUTFILE [-p PLANECOORDINATE]",
        "                 [-r ROI] [-d DISPLAYSETTINGS] [-h] [-b] [-t] [-j DECODERNAME] ",
        "                 [-v VERBOSITYLEVEL] [-y PYRAMIDINFO] [-z ZOOM] [-i INFOLEVEL]",
        "                 [-e SELECTION] [-f FILTER] [-p CHANNELCOMPOSITIONFORMAT]",
        "                 [-b BACKGROUNDCOLOR] [-y PYRAMIDINFO] [-m FORMAT]"
    };

    for (size_t i = 0; i < sizeof(Synopsis) / sizeof(Synopsis[0]); ++i)
    {
        this->GetLog()->WriteLineStdOut(Synopsis[i]);
    }

    stringstream ss;
    int majorVersion, minorVersion;
    libCZI::GetLibCZIVersion(&majorVersion, &minorVersion);
    ss << "  using libCZI version " << majorVersion << "." << minorVersion;
    this->GetLog()->WriteLineStdOut(ss.str());
    this->GetLog()->WriteLineStdOut("");

    static const struct
    {
        int shortOption;
        const wchar_t* argument;
        const wchar_t* explanation;
    } OptionAndExplanation[] =
    {
        {
            '?',
            L"",
            LR"(Show this help message and exit.)"
        },
            {
                L'c',
                L"COMMAND",
                LR"(COMMAND can be any of 'PrintInformation', 'ExtractSubBlock', 'SingleChannelTileAccessor', 'ChannelComposite',
					'SingleChannelPyramidTileAccessor', 'SingleChannelScalingTileAccessor', 'ScalingChannelComposite', 'ExtractAttachment' and 'CreateCZI'.
					\N'PrintInformation' will print information about the CZI-file to the console. The argument 'info-level' can be used
					to specify which information is to be printed.
					\N'ExtractSubBlock' will write the bitmap contained in the specified sub-block to the OUTPUTFILE.
					\N'ChannelComposite' will create a
					channel-composite of the specified region and plane and apply display-settings to it. The resulting bitmap will be written
					to the specified OUTPUTFILE.
					\N'SingleChannelTileAccessor' will create a tile-composite (only from sub-blocks on pyramid-layer 0) of the specified region and plane.
					The resulting bitmap will be written to the specified OUTPUTFILE.
					\N'SingleChannelPyramidTileAccessor' adds to the previous command the ability to explictely address a specific pyramid-layer (which must
					exist in the CZI-document).
					\N'SingleChannelScalingTileAccessor' gets the specified region with an arbitrary zoom factor. It uses the pyramid-layers in the CZI-document
					and scales the bitmap if neccessary. The resulting bitmap will be written to the specified OUTPUTFILE.
					\N'ScalingChannelComposite' operates like the previous command, but in addition gets all channels and creates a multi-channel-composite from them
					using display-settings.
					\N'ExtractAttachment' allows to extract (and save to a file) the contents of attachments.)
					\N'CreateCZI' is used to demonstrate the CZI-creation capabilities of libCZI.)"
            },
            {
                L's',
                L"SOURCEFILE",
                LR"(SOURCEFILE specifies the source CZI-file.)"
            },
            {
                L'p',
                L"PLANE-COORDINATES",
                LR"(Uniquely select a 2D-plane from the document. It is given in the form [DimChar][number], where 'DimChar' specifies a dimension and
					can be any of 'Z', 'C', 'T', 'R', 'I', 'H', 'V' or 'B'. 'number' is an integer. \nExamples: C1T3, C0T-2, C1T44Z15H1.
				)"
            },
            {
                L'r',
                L"ROI",
                LR"(Select a paraxial rectangular region as the region-of-interest. The coordinates may be given either absolute or relative. If using relative
					coordinates, they are relative to what is determined as the upper-left point in the document. \nRelative coordinates are specified with
					the syntax 'rel([x],[y],[width],[height])', absolute coordinates are specified 'abs([x],[y],[width],[height])'.
					\nExamples: rel(0,0,1024,1024), rel(-100,-100,500,500), abs(-230,100,800,800).
				)"
            },
            {
                L'o',
                L"OUTPUTFILE",
                LR"(OUTPUTFILE specifies the output-filename. A suffix will be appended to the name given here depending on the type of the file.)"
            },
            {
                L'd',
                L"DISPLAYSETTINGS",
                LR"(Specifies the display-settings used for creating a channel-composite. The data is given in JSON-notation.)"
            },
            {
                L'h',
                L"",
                LR"(Calculate a hash for the output-picture. The MD5Sum-algorithm is used for this.)"
            },
            {
                L't',
                L"",
                LR"(Draw a one-pixel black line around each tile.)"
            },
            {
                L'j',
                L"DECODERNAME",
                LR"(Choose which decoder implementation is used. Specifying "WIC" will request the Windows-provided decoder - which
				is only available on Windows. By default the internal JPG-XR-decoder is used.)"
            },
            {
                L'v',
                L"VERBOSITYLEVEL",
                LR"(Set the verbosity of this program. The argument is a comma- or semicolon-separated list of the
					following strings: 'All', 'Errors', 'Warnings', 'Infos', 'Errors1', 'Warnings1', 'Infos1',
					'Errors2', 'Warnings2', 'Infos2'.)"
            },
            {
                L'z',
                L"ZOOM",
                LR"(The zoom-factor (which is used for the commands 'SingleChannelScalingTileAccessor' and 'ScalingChannelComposite').
				It is a float between 0 and 1.)"
            },
            {
                L'i',
                L"INFO-LEVEL",
                LR"(When using the command 'PrintInformation' the INFO-LEVEL can be used to specify which information is printed. Possible
				values are "Statistics", "RawXML", "DisplaySettings", "DisplaySettingsJson", "AllSubBlocks", "Attachments", "AllAttachments",
				"PyramidStatistics", "GeneralInfo", "ScalingInfo" and "All".
				The values are given as a list separated by comma or semicolon.)"
            },
            {
                L'b',
                L"BACKGROUND",
                LR"(Specify the background color. BACKGROUND is either a single float or three floats, separated by a comma or semicolon. In case of
				a single float, it gives a grayscale value, in case of three floats it gives a RGB-value. The floats are given normalized to a range
				from 0 to 1.)"
            },
            {
                L'y',
                L"PYRAMIDINFO",
                LR"(For the command 'SingleChannelPyramidTileAccessor' the argument PYRAMIDINFO specifies the pyramid layer. It consists of two
				integers (separated by a comma, semicolon or pipe-symbol), where the first specifies the minification-factor (between pyramid-layers) and
				the second the pyramid-layer (starting with 0 for the layer with the highest resolution).)"
            },
            {
                L'e',
                L"SELECTION",
                LR"(For the command 'ExtractAttachment' this allows to specify a subset which is to be extracted (and saved to a file).
				It is possible to specify the name and the index - only attachments for which the name/index is equal to those values
				specified are processed. The arguments are given in JSON-notation, e.g. {"name":"Thumbnail"} or {"index":3.0}.)"
            },
            {
                L'f',
                L"FILTER",
                LR"(Specify to filter subblocks according to the scene-index. A comma seperated list of either an interval or a single
				integer may be given here, e.g. "2,3" or "2-4,6" or "0-3,5-8".)"
            },
            {
                L'm',
                L"CHANNELCOMPOSITIONFORMAT",
                LR"_(In case of a channel-composition, specifies the pixeltype of the output. Possible values are "bgr24" (the default) and "bgra32".
				If specifying "bgra32" it is possible to give the value of the alpha-pixels in the form "bgra32(128)" - for an alpha-value of 128.)_"
            },
            {
                256,
                L"BOUNDS",
                LR"(Only used for 'CreateCZI': specify the range of coordinates used to create a CZI. Format is e.g. 'T0:3Z0:3C0:2'.)"
            },
            {
                257,
                L"SIZE",
                LR"(Only used for 'CreateCZI': specify the size of the subblocks created in pixels. Format is e.g. '1600x1200'.)"
            },
            {
                258,
                L"TILEINFO",
                LR"(Only used for 'CreateCZI': specify the number of tiles on each plane. Format is e.g. '3x3;10%' for a 3 by 3 tiles arrangement with 10% overlap.)"
            },
            {
                259,
                L"NAME/FILENAME",
                LR"(Only used for 'CreateCZI': (on Linux) specify the filename of a TrueTrype-font (.ttf) to be used for generating text in the subblocks; (on Windows) name of the font.)"
            },
            {
                260,
                L"HEIGHT",
                LR"(Only used for 'CreateCZI': specifies the height of the font in pixels (default: 36).)"
            },
            {
                L'g',
                L"CZI-File-GUID",
                LR"(Only used for 'CreateCZI': specify the GUID of the file (which is useful for bit-exact reproducable results); the GUID must be 
					given in the form  "cfc4a2fe-f968-4ef8-b685-e73d1b77271a" or "{cfc4a2fe-f968-4ef8-b685-e73d1b77271a}".)"
            },
            {
                261,
                L"BITMAPGENERATORCLASSNAME",
                LR"(Only used for 'CreateCZI': specifies the bitmap-generator to use. Possibly values are "gdi", "freetype", "null" or "default". 
				Run with argument '--help=bitmapgen' to get a list of available bitmap-generators.)"
            },
            {
                262,
                L"KEY_VALUE_SUBBLOCKMETADATA",
                LR"(Only used for 'CreateCZI': a key-value list in JSON-notation which will be written as subblock-metadata. For example: 
				{\"StageXPosition\":-8906.346,\"StageYPosition\":-648.51} )"
            },
            {
                263,
                L"COMPRESSIONDESCRIPTION",
                LR"(Only used for 'CreateCZI': a string in a defined format which states the compression-method and (compression-method specific)
				parameters. The format is \"compression_method: key=value; ...\". It starts with the name of the compression-method, followed by a colon,
				then followed by a list of key-value pairs which are separated by a semicolon. Examples: \"zstd0:ExplicitLevel=3\", \"zstd1:ExplicitLevel=2;PreProcess=HiLoByteUnpack\")"
            },
            {
                264,
                L"PIXELTYPE",
                LR"(Only used for 'CreateCZI': a string defining the pixeltype used by the bitmap-generator. Possible valules are 'Gray8', 'Gray16', 
                'Bgr24' or 'Bgr48'. Default is 'Bgr24'.)"
            }
    };

    this->PrintSynopsis(switchesCnt, getSwitch,
        [&](int shortOption)->std::tuple<std::wstring, std::wstring>
        {
            for (size_t i = 0; i < sizeof(OptionAndExplanation) / sizeof(OptionAndExplanation[0]); ++i)
            {
                if (OptionAndExplanation[i].shortOption == shortOption)
                {
                    return make_tuple(wstring(OptionAndExplanation[i].argument), wstring(OptionAndExplanation[i].explanation));
                }
            }

    return make_tuple(wstring(), wstring());
        });
}

void CCmdLineOptions::PrintSynopsis(int switchesCnt, std::function<std::tuple<int, std::wstring>(int idx)> getSwitch, std::function<std::tuple<std::wstring, std::wstring>(int shortOption)> getExplanation)
{
    const int COLUMN_FOR_EXPLANATION = 22;// 24;

    wchar_t arg[2];
    arg[1] = L'\0';
    for (int idx = 0; idx < switchesCnt; ++idx)
    {
        wstringstream ss;
        auto argswitch = getSwitch(idx);	// 1st is short, 2nd is long switch

        auto expl = getExplanation(get<0>(argswitch));
        if (get<0>(expl).empty())
        {
            if (get<0>(argswitch) < 256)
            {
                char shortOpt = (char)get<0>(argswitch);
                ss << L"  " << L'-' << shortOpt << L", --" << get<1>(argswitch);
            }
            else
            {
                ss << L" --" << get<1>(argswitch);
            }
        }
        else
        {
            if (get<0>(argswitch) < 256)
            {
                char shortOpt = (char)get<0>(argswitch);
                ss << L"  " << L'-' << shortOpt << L" " << get<0>(expl) << L", --" << get<1>(argswitch) << L" " << get<0>(expl);
            }
            else
            {
                ss << L" --" << get<1>(argswitch) << L" " << get<0>(expl);
            }
        }

        if (!get<1>(expl).empty())
        {
            wstring prefix;
            if (ss.str().size() < COLUMN_FOR_EXPLANATION - 3)
            {
                prefix = ss.str() + wstring(COLUMN_FOR_EXPLANATION - ss.str().size(), L' ');
            }
            else
            {
                this->GetLog()->WriteLineStdOut(ss.str());
                prefix = wstring(COLUMN_FOR_EXPLANATION, L' ');
            }

            // subtract 1 in order not to run into trouble if outputting a complete line (80 chars normally), where in the end we get two linefeeds...
            auto lines = wrap(get<1>(expl).c_str(), 80 - COLUMN_FOR_EXPLANATION - 1);
            bool isFirstLine = true;
            for (const auto& l : lines)
            {
                auto line = prefix + l;
                this->GetLog()->WriteLineStdOut(line);
                if (isFirstLine == true)
                {
                    prefix = wstring(COLUMN_FOR_EXPLANATION, L' ');
                    isFirstLine = false;
                }
            }
        }
        else
        {
            this->GetLog()->WriteLineStdOut(ss.str());
        }
    }
}

bool CCmdLineOptions::IsLogLevelEnabled(int level) const
{
    if (level < 0)
    {
        level = 0;
    }
    else if (level > 31)
    {
        level = 31;
    }

    return (this->enabledOutputLevels & (1 << level)) ? true : false;;
}

libCZI::CDimCoordinate CCmdLineOptions::ParseDimCoordinate(const std::wstring& str)
{
    return libCZI::CDimCoordinate::Parse(convertToUtf8(str).c_str());
}

libCZI::CDimCoordinate CCmdLineOptions::ParseDimCoordinate(const std::string& s)
{
    return libCZI::CDimCoordinate::Parse(s.c_str());
}

/*static*/Command CCmdLineOptions::ParseCommand(const wchar_t* s)
{
    static const struct
    {
        const wchar_t* cmdName;
        Command		   command;
    } CmdNamesAndCmd[] =
    {
        { L"PrintInformation",					Command::PrintInformation },
        { L"ExtractSubBlock",					Command::ExtractSubBlock },
        { L"SingleChannelTileAccessor",			Command::SingleChannelTileAccessor },
        { L"ChannelComposite",					Command::ChannelComposite },
        { L"SingleChannelPyramidTileAccessor",	Command::SingleChannelPyramidTileAccessor },
        { L"SingleChannelScalingTileAccessor",  Command::SingleChannelScalingTileAccessor },
        { L"ScalingChannelComposite",			Command::ScalingChannelComposite },
        { L"ExtractAttachment",                 Command::ExtractAttachment},
        { L"CreateCZI",							Command::CreateCZI },
        /*{ L"ReadWriteCZI",						Command::ReadWriteCZI}*/
    };

    for (size_t i = 0; i < sizeof(CmdNamesAndCmd) / sizeof(CmdNamesAndCmd[0]); ++i)
    {
        if (__wcasecmp(s, CmdNamesAndCmd[i].cmdName))
        {
            return CmdNamesAndCmd[i].command;
        }
    }

    throw std::invalid_argument("Invalid command.");
}

void CCmdLineOptions::ParseRect(const std::wstring& s)
{
    int x, y, w, h;
    bool absOrRel;

    std::wregex rect_regex(LR"(((abs|rel)\(([\+|-]?[[:digit:]]+),([\+|-]?[[:digit:]]+)),([\+]?[[:digit:]]+),([\+]?[[:digit:]]+)\))");
    std::wsmatch pieces_match;

    if (std::regex_match(s, pieces_match, rect_regex))
    {
        if (pieces_match.size() == 7)
        {
            std::wssub_match sub_match = pieces_match[2];
            if (sub_match.compare(L"abs") == 0)
            {
                absOrRel = true;
            }
            else
            {
                absOrRel = false;
            }

            x = std::stoi(pieces_match[3]);
            y = std::stoi(pieces_match[4]);
            w = std::stoi(pieces_match[5]);
            h = std::stoi(pieces_match[6]);

            this->rectModeAbsoluteOrRelative = absOrRel;
            this->rectX = x;
            this->rectY = y;
            this->rectW = w;
            this->rectH = h;
            return;
        }
    }

    throw std::invalid_argument("Invalid rect");
}

void CCmdLineOptions::SetOutputFilename(const std::wstring& s)
{
#if defined(WIN32ENV)
    wchar_t drive[_MAX_DRIVE];
    wchar_t dir[_MAX_DIR];
    wchar_t fname[_MAX_FNAME];
    wchar_t ext[_MAX_EXT];

    _wsplitpath_s(s.c_str(), drive, dir, fname, ext);

    wchar_t path[_MAX_PATH];
    _wmakepath_s(path, _MAX_PATH, drive, dir, L"", L"");
    this->outputPath = path;
    this->outputFilename = fname;
    this->outputFilename += ext;
#endif
#if defined(LINUXENV)
    std::string sutf8 = convertToUtf8(s);
    char* dirName = strdup(sutf8.c_str());
    char* fname = strdup(sutf8.c_str());

    // 'dirname' might modify the string passed in, it might also return a pointer to internally allocated memory
    char* dirNameResult = dirname(dirName);
    char* filename = basename(fname);
    this->outputPath = convertUtf8ToUCS2(dirNameResult);
    this->outputPath += L'/';
    this->outputFilename = convertUtf8ToUCS2(filename);
    free(dirName);
    free(fname);
#endif
}

std::wstring CCmdLineOptions::MakeOutputFilename(const wchar_t* suffix, const wchar_t* extension) const
{
    std::wstring out;
    out += this->outputPath;
    out += this->outputFilename;
    if (suffix != nullptr)
    {
        out += suffix;
    }

    if (extension != nullptr)
    {
        out += L'.';
        out += extension;
    }

    return out;
}

void CCmdLineOptions::ParseDisplaySettings(const std::wstring& s)
{
    auto str = convertToUtf8(s);
    this->ParseDisplaySettings(str);
}

static std::vector<std::tuple<double, double>> ParseSplintPoints(const rapidjson::Value& v)
{
    if (!v.IsArray())
    {
        throw std::logic_error("Invalid JSON");
    }

    std::vector<std::tuple<double, double>> result;

    for (rapidjson::Value::ConstValueIterator it = v.Begin(); it != v.End(); ++it)
    {
        double d1 = it->GetDouble();
        ++it;
        if (it == v.End())
            break;
        double d2 = it->GetDouble();
        result.push_back(make_tuple(d1, d2));
    }

    return result;
}

static std::tuple<int, ChannelDisplaySettings> GetChannelInfo(const rapidjson::Value& v)
{
    if (v.HasMember("ch") == false)
    {
        throw std::logic_error("Invalid JSON");
    }

    int chNo = v["ch"].GetInt();
    ChannelDisplaySettings chInfo; chInfo.Clear();
    if (v.HasMember("black-point"))
    {
        chInfo.blackPoint = (float)v["black-point"].GetDouble();
    }
    else
    {
        chInfo.blackPoint = 0;
    }

    if (v.HasMember("white-point"))
    {
        chInfo.whitePoint = (float)v["white-point"].GetDouble();
    }
    else
    {
        chInfo.whitePoint = 1;
    }

    if (v.HasMember("weight"))
    {
        chInfo.weight = (float)v["weight"].GetDouble();
    }
    else
    {
        chInfo.weight = 1;
    }

    if (v.HasMember("tinting"))
    {
        if (v["tinting"].IsString())
        {
            const auto& str = trim(v["tinting"].GetString());
            if (icasecmp(str, "none"))
            {
                chInfo.enableTinting = false;
            }
            else if (str.size() > 1 && str[0] == '#')
            {
                std::uint8_t r, g, b;
                r = g = b = 0;
                for (size_t i = 1; i < (std::max)((size_t)7, (size_t)str.size()); ++i)
                {
                    if (!isxdigit(str[i]))
                    {
                        throw std::logic_error("Invalid JSON");
                    }

                    switch (i)
                    {
                    case 1: r = HexCharToInt(str[i]); break;
                    case 2: r = (r << 4) + HexCharToInt(str[i]); break;
                    case 3: g = HexCharToInt(str[i]); break;
                    case 4: g = (g << 4) + HexCharToInt(str[i]); break;
                    case 5: b = HexCharToInt(str[i]); break;
                    case 6: b = (b << 4) + HexCharToInt(str[i]); break;
                    }
                }

                chInfo.tinting.color.r = r;
                chInfo.tinting.color.g = g;
                chInfo.tinting.color.b = b;
                chInfo.enableTinting = true;
            }
        }
    }

    if (v.HasMember("gamma"))
    {
        chInfo.gamma = (float)v["gamma"].GetDouble();
    }

    if (!chInfo.IsGammaValid())
    {
        if (v.HasMember("splinelut"))
        {
            chInfo.splinePoints = ParseSplintPoints(v["splinelut"]);
        }
    }

    return std::make_tuple(chNo, chInfo);
}

bool CCmdLineOptions::TryParseDisplaySettings(const std::string& s, std::map<int, ChannelDisplaySettings>* multiChannelCompositeChannelInfos)
{
    vector<std::tuple<int, ChannelDisplaySettings>> vecChNoAndChannelInfo;
    rapidjson::Document document;
    document.Parse(s.c_str());
    if (document.HasParseError())
    {
        return false;
    }

    const bool isObject = document.IsObject();
    if (!isObject)
    {
        return false;
    }

    const bool hasChannels = document.HasMember("channels");
    if (!hasChannels)
    {
        return false;
    }

    const bool isChannelsArray = document["channels"].IsArray();
    if (!isChannelsArray)
    {
        return false;
    }

    const auto& channels = document["channels"];
    for (decltype(channels.Size()) i = 0; i < channels.Size(); ++i)
    {
        try
        {
            vecChNoAndChannelInfo.emplace_back(GetChannelInfo(channels[i]));
        }
        catch (exception&)
        {
            return false;
        }
    }

    if (multiChannelCompositeChannelInfos != nullptr)
    {
        for (const auto& it : vecChNoAndChannelInfo)
        {
            multiChannelCompositeChannelInfos->at(get<0>(it)) = get<1>(it);
        }
    }

    return true;
}

void CCmdLineOptions::ParseDisplaySettings(const std::string& s)
{
    // TODO: provide a reasonable error handling
    vector<std::tuple<int, ChannelDisplaySettings>> vecChNoAndChannelInfo;
    rapidjson::Document document;
    document.Parse(s.c_str());
    if (document.HasParseError())
    {
        throw std::logic_error("Invalid JSON");
    }

    bool isObj = document.IsObject();
    bool hasChannels = document.HasMember("channels");
    bool isChannelsArray = document["channels"].IsArray();
    const auto& channels = document["channels"];
    for (decltype(channels.Size()) i = 0; i < channels.Size(); ++i)
    {
        vecChNoAndChannelInfo.emplace_back(GetChannelInfo(channels[i]));
    }

    for (const auto& it : vecChNoAndChannelInfo)
    {
        this->multiChannelCompositeChannelInfos[get<0>(it)] = get<1>(it);
    }
}

/*static*/bool CCmdLineOptions::TryParseVerbosityLevel(const std::string& s, std::uint32_t* levels)
{
    static constexpr struct
    {
        const char* name;
        std::uint32_t flags;
    } verbosities[] =
    {
        { "All",0xffffffff} ,
        { "Errors",(1 << 0) | (1 << 1)},
        { "Errors1",(1 << 0) },
        { "Errors2",(1 << 1) },
        { "Warnings",(1 << 2) | (1 << 3) },
        { "Warnings1",(1 << 2)  },
        { "Warnings2",(1 << 3) },
        { "Infos",(1 << 4) | (1 << 5) },
        { "Infos1",(1 << 4)  },
        { "Infos2",(1 << 5) }
    };

    std::uint32_t verbosity_levels = 0;

    size_t offset = 0;
    for (;;)
    {
        const size_t length = strcspn(offset + s.c_str(), ",;");
        if (length == 0)
        {
            break;
        }

        string tk(s.c_str() + offset, length);
        string tktr = trim(tk);
        if (tktr.length() > 0)
        {
            bool token_found = false;
            for (size_t i = 0; i < sizeof(verbosities) / sizeof(verbosities[0]); ++i)
            {
                if (icasecmp(verbosities[i].name, tktr))
                {
                    verbosity_levels |= verbosities[i].flags;
                    token_found = true;
                    break;
                }
            }

            if (!token_found)
            {
                return false;
            }
        }

        if (s[length + offset] == '\0')
        {
            break;
        }

        offset += (length + 1);
    }

    if (levels != nullptr)
    {
        *levels = verbosity_levels;
    }

    return true;
}

std::uint32_t CCmdLineOptions::ParseVerbosityLevel(const wchar_t* s)
{
    static const struct
    {
        const wchar_t* name;
        std::uint32_t flags;
    } Verbosities[] =
    {
        { L"All",0xffffffff} ,
        { L"Errors",(1 << 0) | (1 << 1)},
        { L"Errors1",(1 << 0) },
        { L"Errors2",(1 << 1) },
        { L"Warnings",(1 << 2) | (1 << 3) },
        { L"Warnings1",(1 << 2)  },
        { L"Warnings2",(1 << 3) },
        { L"Infos",(1 << 4) | (1 << 5) },
        { L"Infos1",(1 << 4)  },
        { L"Infos2",(1 << 5) }
    };

    std::uint32_t levels = 0;
    static const wchar_t* Delimiters = L",;";

    for (;;)
    {
        size_t length = wcscspn(s, L",;");
        if (length == 0)
            break;

        std::wstring tk(s, length);
        std::wstring tktr = trim(tk);
        if (tktr.length() > 0)
        {
            for (size_t i = 0; i < sizeof(Verbosities) / sizeof(Verbosities[0]); ++i)
            {
                if (__wcasecmp(Verbosities[i].name, tktr.c_str()))
                {
                    levels |= Verbosities[i].flags;
                    break;
                }
            }
        }

        if (*(s + length) == L'\0')
            break;

        s += (length + 1);
    }

    return levels;
}

/*static*/bool CCmdLineOptions::TryParseInfoLevel(const std::string& s, InfoLevel* info_level)
{
    static constexpr struct
    {
        const char* name;
        InfoLevel flag;
    } info_levels[] =
    {
        { "Statistics", InfoLevel::Statistics },
        { "RawXML", InfoLevel::RawXML },
        { "DisplaySettings", InfoLevel::DisplaySettings },
        { "DisplaySettingsJson", InfoLevel::DisplaySettingsJson },
        { "AllSubBlocks", InfoLevel::AllSubBlocks },
        { "Attachments", InfoLevel::AttachmentInfo },
        { "AllAttachments", InfoLevel::AllAttachments },
        { "PyramidStatistics", InfoLevel::PyramidStatistics },
        { "GeneralInfo", InfoLevel::GeneralInfo },
        { "ScalingInfo", InfoLevel::ScalingInfo },
        { "All", InfoLevel::All }
    };

    std::underlying_type<InfoLevel>::type levels = (std::underlying_type<InfoLevel>::type)InfoLevel::None;

    size_t offset = 0;
    for (;;)
    {
        const size_t length = strcspn(offset + s.c_str(), ",;");
        if (length == 0)
        {
            break;
        }

        string tk(s.c_str() + offset, length);
        string tktr = trim(tk);
        if (tktr.length() > 0)
        {
            bool token_found = false;
            for (size_t i = 0; i < sizeof(info_levels) / sizeof(info_levels[0]); ++i)
            {
                if (icasecmp(info_levels[i].name, tktr))
                {
                    levels |= static_cast<std::underlying_type<InfoLevel>::type>(info_levels[i].flag);
                    token_found = true;
                    break;
                }
            }

            if (!token_found)
            {
                return false;
            }
        }

        if (s[length + offset] == '\0')
        {
            break;
        }

        offset += (length + 1);
    }

    if (info_level != nullptr)
    {
        *info_level = static_cast<InfoLevel>(levels);
    }

    return true;
}

void CCmdLineOptions::ParseInfoLevel(const wchar_t* s)
{
    static const struct
    {
        const wchar_t* name;
        InfoLevel flag;
    } Verbosities[] =
    {
        { L"Statistics", InfoLevel::Statistics },
        { L"RawXML", InfoLevel::RawXML },
        { L"DisplaySettings", InfoLevel::DisplaySettings },
        { L"DisplaySettingsJson", InfoLevel::DisplaySettingsJson },
        { L"AllSubBlocks", InfoLevel::AllSubBlocks },
        { L"Attachments", InfoLevel::AttachmentInfo },
        { L"AllAttachments", InfoLevel::AllAttachments },
        { L"PyramidStatistics", InfoLevel::PyramidStatistics },
        { L"GeneralInfo", InfoLevel::GeneralInfo },
        { L"ScalingInfo", InfoLevel::ScalingInfo },
        { L"All", InfoLevel::All }
    };

    std::underlying_type<InfoLevel>::type  levels = (std::underlying_type<InfoLevel>::type)InfoLevel::None;
    static const wchar_t* Delimiters = L",;";

    for (;;)
    {
        size_t length = wcscspn(s, L",;");
        if (length == 0)
            break;

        std::wstring tk(s, length);
        std::wstring tktr = trim(tk);
        if (tktr.length() > 0)
        {
            for (size_t i = 0; i < sizeof(Verbosities) / sizeof(Verbosities[0]); ++i)
            {
                if (__wcasecmp(Verbosities[i].name, tktr.c_str()))
                {
                    levels |= (std::underlying_type<InfoLevel>::type)Verbosities[i].flag;
                    break;
                }
            }
        }

        if (*(s + length) == L'\0')
            break;

        s += (length + 1);
    }

    this->infoLevel = (InfoLevel)levels;
}

/*static*/bool CCmdLineOptions::TryParseJxrCodecUseWicCodec(const std::string& s, bool* use_wic_codec)
{
    // for the time being, we just decide whether to use the WIC-codec or not    
    if (icasecmp(s, "WIC") || icasecmp(s, "WICDecoder"))
    {
        if (use_wic_codec != nullptr)
        {
            *use_wic_codec = true;
        }

        return true;
    }

    return false;
}

bool CCmdLineOptions::ParseJxrCodec(const wchar_t* s)
{
    wstring str = trim(wstring(s));
    if (__wcasecmp(str.c_str(), L"WIC") || __wcasecmp(str.c_str(), L"WICDecoder"))
    {
        return true;
    }

    return false;
}

/*static*/bool CCmdLineOptions::TryParseParseBackgroundColor(const std::string& s, libCZI::RgbFloatColor* color)
{
    float f[3];
    f[0] = f[1] = f[2] = std::numeric_limits<float>::quiet_NaN();

    const char* pointer = s.c_str();
    for (int i = 0; i < 3; ++i)
    {
        char* endPtr;
        f[i] = strtof(pointer, &endPtr);

        const char* endPtrSkipped = skipWhiteSpaceAndOneOfThese(endPtr, ";,|");
        if (*endPtrSkipped == L'\0')
        {
            if (i == 1)
            {
                // we expect to have exactly one float or three, anything else is invalid
                return false;
            }

            break;
        }

        if (i == 2)
        {
            return false;
        }

        pointer = endPtrSkipped;
    }

    if (isnan(f[1]) && isnan(f[2]))
    {
        if (color != nullptr)
        {
            *color = libCZI::RgbFloatColor{ f[0],f[0],f[0] };
        }
    }

    if (color != nullptr)
    {
        *color = libCZI::RgbFloatColor{ f[0],f[1],f[2] };
    }

    return true;
}

libCZI::RgbFloatColor CCmdLineOptions::ParseBackgroundColor(const wchar_t* s)
{
    // TODO: somewhat stricter parsing...
    float f[3];
    f[0] = f[1] = f[2] = std::numeric_limits<float>::quiet_NaN();
    for (int i = 0; i < 3; ++i)
    {
        wchar_t* endPtr;
        f[i] = wcstof(s, &endPtr);

        const wchar_t* endPtrSkipped = skipWhiteSpaceAndOneOfThese(endPtr, L";,|");
        if (*endPtrSkipped == L'\0')
            break;
    }

    if (isnan(f[1]) && isnan(f[2]))
    {
        return libCZI::RgbFloatColor{ f[0],f[0],f[0] };
    }

    return libCZI::RgbFloatColor{ f[0],f[1],f[2] };
}

/*static*/bool CCmdLineOptions::TryParsePyramidInfo(const std::string& s, int* pyramidMinificationFactor, int* pyramidLayerNo)
{
    size_t position_of_delimiter = s.find_first_of(";,|");
    if (position_of_delimiter == string::npos)
    {
        return false;
    }

    string minification_factor_string = s.substr(0, position_of_delimiter);
    string pyramid_layer_no_string = s.substr(1 + position_of_delimiter);

    int minificationFactor, layerNo;
    if (!TryParseInt32(minification_factor_string, &minificationFactor) ||
        !TryParseInt32(pyramid_layer_no_string, &layerNo))
    {
        return false;
    }

    if (pyramidLayerNo != nullptr)
    {
        *pyramidLayerNo = layerNo;
    }

    if (pyramidMinificationFactor != nullptr)
    {
        *pyramidMinificationFactor = minificationFactor;
    }

    return true;
}

void CCmdLineOptions::ParsePyramidInfo(const wchar_t* sz)
{
    int minificationFactor, layerNo;
    const wchar_t* endPtr;
    minificationFactor = wcstol(sz, (wchar_t**)&endPtr, 10);
    sz = skipWhiteSpaceAndOneOfThese(endPtr, L";,|");
    if (*sz == L'\0')
    {
        throw std::logic_error("Invalid pyramidinfo argument");
    }

    layerNo = wcstol(sz, (wchar_t**)&endPtr, 10);

    // TODO: check arguments...
    this->pyramidLayerNo = layerNo;
    this->pyramidMinificationFactor = minificationFactor;
}

void CCmdLineOptions::ParseZoom(const wchar_t* sz)
{
    // TODO: error handling
    float zoom = stof(sz);
    this->zoom = zoom;
}

void CCmdLineOptions::ParseSelection(const std::wstring& s)
{
    auto str = convertToUtf8(s);
    this->ParseSelection(str);
}

/*static*/bool CCmdLineOptions::TryParseSelection(const std::string& s, std::map<std::string, ItemValue>* key_value)
{
    std::map<string, ItemValue> map;
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
        ItemValue iv;
        if (itr->value.IsString())
        {
            iv = ItemValue(itr->value.GetString());
        }
        else if (itr->value.IsDouble())
        {
            iv = ItemValue(itr->value.GetDouble());
        }
        else if (itr->value.IsBool())
        {
            iv = ItemValue(itr->value.GetBool());
        }
        else
        {
            return false;
        }

        map[name] = iv;
    }

    if (key_value != nullptr)
    {
        key_value->swap(map);
    }

    return true;
}

void CCmdLineOptions::ParseSelection(const std::string& s)
{
    std::map<string, ItemValue> map;
    rapidjson::Document document;
    document.Parse(s.c_str());
    if (document.HasParseError() || !document.IsObject())
    {
        throw std::logic_error("Invalid JSON");
    }

    for (rapidjson::Value::ConstMemberIterator itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr)
    {
        if (!itr->name.IsString())
        {
            throw std::logic_error("Invalid JSON");
        }

        string name = itr->name.GetString();
        ItemValue iv;
        if (itr->value.IsString())
        {
            iv = ItemValue(itr->value.GetString());
        }
        else if (itr->value.IsDouble())
        {
            iv = ItemValue(itr->value.GetDouble());
        }
        else if (itr->value.IsBool())
        {
            iv = ItemValue(itr->value.GetBool());
        }
        else
        {
            throw std::logic_error("Invalid JSON");
        }

        map[name] = iv;
    }

    std::swap(this->mapSelection, map);
}

ItemValue CCmdLineOptions::GetSelectionItemValue(const char* sz) const
{
    const auto it = this->mapSelection.find(sz);
    if (it != this->mapSelection.end())
    {
        return it->second;
    }

    return ItemValue();
}

/*static*/bool CCmdLineOptions::TryParseTileFilter(const std::string& s, std::shared_ptr<libCZI::IIndexSet>* scene_index_set)
{
    shared_ptr<libCZI::IIndexSet> index_set;
    try
    {
        index_set = libCZI::Utils::IndexSetFromString(convertUtf8ToUCS2(s));
    }
    catch (exception&)
    {
        return false;
    }

    if (scene_index_set != nullptr)
    {
        scene_index_set->swap(index_set);
    }

    return true;
}

void CCmdLineOptions::ParseTileFilter(const wchar_t* s)
{
    this->sceneIndexSet = libCZI::Utils::IndexSetFromString(s);
}

std::shared_ptr<libCZI::IIndexSet> CCmdLineOptions::GetSceneIndexSet() const
{
    return this->sceneIndexSet;
}

/*static*/bool CCmdLineOptions::TryParseChannelCompositionFormat(const std::string& s, libCZI::PixelType* channel_composition_format, std::uint8_t* channel_composition_alpha_value)
{
    auto arg = trim(s);
    if (icasecmp(arg, "bgr24"))
    {
        if (channel_composition_format != nullptr)
        {
            *channel_composition_format = libCZI::PixelType::Bgr24;
        }

        return true;
    }
    else if (icasecmp(arg, "bgra32"))
    {
        if (channel_composition_format != nullptr)
        {
            *channel_composition_format = libCZI::PixelType::Bgra32;
        }

        if (channel_composition_alpha_value != nullptr)
        {
            *channel_composition_alpha_value = 0xff;
        }

        return true;
    }

    libCZI::PixelType pixel_type;
    uint8_t alpha;

    if (!TryParseChannelCompositionFormatWithAlphaValue(convertUtf8ToUCS2(arg), pixel_type, alpha))
    {
        return false;
    }

    if (channel_composition_format != nullptr)
    {
        *channel_composition_format = pixel_type;
    }

    if (channel_composition_alpha_value != nullptr)
    {
        *channel_composition_alpha_value = alpha;
    }

    return true;
}

void CCmdLineOptions::ParseChannelCompositionFormat(const wchar_t* s)
{
    auto arg = trim(s);
    if (__wcasecmp(arg.c_str(), L"bgr24"))
    {
        this->channelCompositePixelType = libCZI::PixelType::Bgr24;
        return;
    }
    else if (__wcasecmp(arg.c_str(), L"bgra32"))
    {
        this->channelCompositePixelType = libCZI::PixelType::Bgra32;
        this->channelCompositeAlphaValue = 0xff;
        return;
    }
    else if (TryParseChannelCompositionFormatWithAlphaValue(arg, this->channelCompositePixelType, this->channelCompositeAlphaValue))
    {
        return;
    }

    throw std::invalid_argument("Invalid channel-composition-format.");
}

/*static*/bool CCmdLineOptions::TryParseChannelCompositionFormatWithAlphaValue(const std::wstring& s, libCZI::PixelType& channelCompositePixelType, std::uint8_t& channelCompositeAlphaValue)
{
    std::wregex regex(LR"_(bgra32\((\d+|0x[\d|a-f|A-F]+)\))_", regex_constants::ECMAScript | regex_constants::icase);
    std::wsmatch pieces_match;
    if (std::regex_match(s, pieces_match, regex))
    {
        if (pieces_match.size() == 2)
        {
            std::wssub_match sub_match = pieces_match[1];
            if (sub_match.length() > 2)
            {
                if (sub_match.str()[0] == L'0' && (sub_match.str()[1] == L'x' || sub_match.str()[0] == L'X'))
                {
                    auto hexStr = convertToUtf8(sub_match);
                    std::uint32_t value;
                    if (!ConvertHexStringToInteger(hexStr.c_str() + 2, &value) || value > 0xff)
                    {
                        return false;;
                    }

                    channelCompositePixelType = libCZI::PixelType::Bgra32;
                    channelCompositeAlphaValue = static_cast<std::uint8_t>(value);
                    return true;
                }
            }

            int i = std::stoi(sub_match);
            if (i < 0 || i > 255)
            {
                return false;
            }

            channelCompositePixelType = libCZI::PixelType::Bgra32;
            channelCompositeAlphaValue = static_cast<std::uint8_t>(i);
            return true;
        }
    }

    return false;
}

/*static*/bool CCmdLineOptions::TryParseCreateBounds(const std::string& s, libCZI::CDimBounds* create_bounds)
{
    try
    {
        const auto bounds = libCZI::CDimBounds::Parse(s.c_str());
        if (create_bounds != nullptr)
        {
            *create_bounds = bounds;
        }

        return true;
    }
    catch (exception&)
    {
        return false;
    }
}

void CCmdLineOptions::ParseCreateBounds(const std::wstring& s)
{
    this->createBounds = libCZI::CDimBounds::Parse(convertToUtf8(s).c_str());
}

/*static*/bool CCmdLineOptions::TryParseCreateSize(const std::string& s, std::tuple<std::uint32_t, std::uint32_t>* size)
{
    // expected format is: 1024x768 or 1024*768
    std::regex regex(R"((\d+)\s*[\*xX]\s*(\d+))");
    std::smatch pieces_match;
    if (std::regex_match(s, pieces_match, regex))
    {
        if (pieces_match.size() == 3 && pieces_match[1].matched && pieces_match[2].matched)
        {
            auto v = std::stoull(pieces_match[1].str());
            if (v == 0 || v > (std::numeric_limits<uint32_t>::max)())
            {
                return false;
            }

            const uint32_t w = static_cast<uint32_t>(v);

            v = std::stoull(pieces_match[2].str());
            if (v == 0 || v > (std::numeric_limits<uint32_t>::max)())
            {
                return false;
            }

            const uint32_t h = static_cast<uint32_t>(v);

            if (size != nullptr)
            {
                *size = make_tuple(w, h);
            }

            return true;
        }
    }

    return false;
}

void CCmdLineOptions::ParseCreateSize(const std::wstring& s)
{
    // expected format is: 1024x768 or 1024*768
    std::wregex regex(LR"((\d+)\s*[\*xX]\s*(\d+))");
    std::wsmatch pieces_match;
    if (std::regex_match(s, pieces_match, regex))
    {
        if (pieces_match.size() == 3 && pieces_match[1].matched && pieces_match[2].matched)
        {
            auto v = std::stoull(pieces_match[1].str());
            if (v == 0 || v > (std::numeric_limits<uint32_t>::max)())
            {
                throw std::invalid_argument("Invalid size specification for sub-block creation.");
            }

            uint32_t w = static_cast<uint32_t>(v);

            v = std::stoull(pieces_match[2].str());
            if (v == 0 || v > (std::numeric_limits<uint32_t>::max)())
            {
                throw std::invalid_argument("Invalid size specification for sub-block creation.");
            }

            uint32_t h = static_cast<uint32_t>(v);

            this->createSize = make_tuple(w, h);
            return;
        }
    }

    throw std::invalid_argument("Invalid size specification for sub-block creation.");
}

/*static*/bool CCmdLineOptions::TryParseCreateTileInfo(const std::string& s, CreateTileInfo* create_tile_info)
{
    // expected format: 4x4  or 4x4;10%
    std::regex regex(R"((\d+)\s*[*xX]\s*(\d+)\s*(?:[,;-]\s*(\d+)\s*%){0,1}\s*)");
    std::smatch pieces_match;
    if (std::regex_match(s, pieces_match, regex))
    {
        if (pieces_match.size() == 4 && pieces_match[0].matched == true && pieces_match[1].matched == true && pieces_match[2].matched == true && pieces_match[3].matched == false)
        {
            if (pieces_match[0].str().size() != s.size())
            {
                return false;
            }

            if (pieces_match[0].str().size() != s.size())
            {
                return false;
            }

            auto v = std::stoull(pieces_match[1].str());
            if (v == 0 || v > (numeric_limits<uint32_t>::max)())
            {
                return false;
            }

            const uint32_t rows = static_cast<uint32_t>(v);
            v = std::stoull(pieces_match[2].str());
            if (v == 0 || v > (numeric_limits<uint32_t>::max)())
            {
                return false;
            }

            const uint32_t cols = static_cast<uint32_t>(v);

            if (create_tile_info != nullptr)
            {
                create_tile_info->rows = rows;
                create_tile_info->columns = cols;
                create_tile_info->overlap = 0;
            }

            return true;
        }
        else if (pieces_match.size() == 4 && pieces_match[0].matched == true && pieces_match[1].matched == true && pieces_match[2].matched == true && pieces_match[3].matched == true)
        {
            if (pieces_match[0].str().size() != s.size())
            {
                return false;
            }

            auto v = std::stoull(pieces_match[1].str());
            if (v == 0 || v > (numeric_limits<uint32_t>::max)())
            {
                return false;
            }

            const uint32_t rows = static_cast<uint32_t>(v);
            v = std::stoull(pieces_match[2].str());
            if (v == 0 || v > (numeric_limits<uint32_t>::max)())
            {
                return false;
            }

            const uint32_t cols = static_cast<uint32_t>(v);
            v = std::stoull(pieces_match[3].str());
            if (v == 0 || v > (numeric_limits<uint32_t>::max)())
            {
                return false;
            }

            const uint32_t overlapPercent = (uint32_t)v;

            if (create_tile_info != nullptr)
            {
                create_tile_info->rows = rows;
                create_tile_info->columns = cols;
                create_tile_info->overlap = overlapPercent / 100.0f;
            }

            return true;
        }
    }

    //throw std::invalid_argument("Invalid tile-info specification for sub-block creation.");
    return false;
}

void CCmdLineOptions::ParseCreateTileInfo(const std::wstring& s)
{
    // expected format: 4x4  or 4x4;10%
    std::wregex regex(LR"((\d+)\s*[*xX]\s*(\d+)\s*(?:[,;-]\s*(\d+)\s*%){0,1}\s*)");
    std::wsmatch pieces_match;
    if (std::regex_match(s, pieces_match, regex))
    {
        if (pieces_match.size() == 4 && pieces_match[0].matched == true && pieces_match[1].matched == true && pieces_match[2].matched == true && pieces_match[3].matched == false)
        {
            if (pieces_match[0].str().size() != s.size())
            {
                throw std::invalid_argument("Invalid tile-info specification for sub-block creation.");
            }

            if (pieces_match[0].str().size() != s.size())
            {
                throw std::invalid_argument("Invalid tile-info specification for sub-block creation.");
            }

            auto v = std::stoull(pieces_match[1].str());
            if (v == 0 || v > (numeric_limits<uint32_t>::max)())
            {
                throw std::invalid_argument("Invalid tile-info specification for sub-block creation.");
            }

            uint32_t rows = static_cast<uint32_t>(v);
            v = std::stoull(pieces_match[2].str());
            if (v == 0 || v > (numeric_limits<uint32_t>::max)())
            {
                throw std::invalid_argument("Invalid tile-info specification for sub-block creation.");
            }

            uint32_t cols = static_cast<uint32_t>(v);

            this->createTileInfo.rows = rows;
            this->createTileInfo.columns = cols;
            this->createTileInfo.overlap = 0;
            return;
        }
        else if (pieces_match.size() == 4 && pieces_match[0].matched == true && pieces_match[1].matched == true && pieces_match[2].matched == true && pieces_match[3].matched == true)
        {
            if (pieces_match[0].str().size() != s.size())
            {
                throw std::invalid_argument("Invalid tile-info specification for sub-block creation.");
            }

            auto v = std::stoull(pieces_match[1].str());
            if (v == 0 || v > (numeric_limits<uint32_t>::max)())
            {
                throw std::invalid_argument("Invalid tile-info specification for sub-block creation.");
            }

            uint32_t rows = static_cast<uint32_t>(v);
            v = std::stoull(pieces_match[2].str());
            if (v == 0 || v > (numeric_limits<uint32_t>::max)())
            {
                throw std::invalid_argument("Invalid tile-info specification for sub-block creation.");
            }

            uint32_t cols = static_cast<uint32_t>(v);
            v = std::stoull(pieces_match[3].str());
            if (v == 0 || v > (numeric_limits<uint32_t>::max)())
            {
                throw std::invalid_argument("Invalid tile-info specification for sub-block creation.");
            }

            uint32_t overlapPercent = (uint32_t)v;

            this->createTileInfo.rows = rows;
            this->createTileInfo.columns = cols;
            this->createTileInfo.overlap = overlapPercent / 100.0f;
            return;
        }
    }

    throw std::invalid_argument("Invalid tile-info specification for sub-block creation.");
}

void CCmdLineOptions::ParseFont(const std::wstring& s)
{
    this->fontnameOrFile = s;
}

void CCmdLineOptions::ParseFontHeight(const std::wstring& s)
{
    this->fontHeight = stoul(s);
}

/*static*/bool CCmdLineOptions::TryParseNewCziFileguid(const std::string& s, GUID* guid)
{
    GUID g;
    bool b = TryParseGuid(convertUtf8ToUCS2(s), &g);
    if (!b)
    {
        return false;
    }

    if (guid != nullptr)
    {
        *guid = g;
    }

    return true;
}

void CCmdLineOptions::ParseNewCziFileguid(const std::wstring& s)
{
    GUID g;
    bool b = TryParseGuid(s, &g);
    if (!b)
    {
        throw invalid_argument("invalid argument for file-GUID");
    }

    this->newCziFileGuid = g;
    this->newCziFileGuidValid = true;
}

/*static*/bool CCmdLineOptions::TryParseBitmapGenerator(const std::string& s, std::string* generator_class_name)
{
    string class_name;
    if (icasecmp("null", s))
    {
        class_name = "null";
    }
    else if (icasecmp("default", s))
    {
        class_name = "default";
    }
    else if (icasecmp("gdi", s))
    {
        class_name = "gdi";
    }
    else if (icasecmp("freetype", s))
    {
        class_name = "freetype";
    }

    if (class_name.empty())
    {
        return false;
    }

    if (generator_class_name != nullptr)
    {
        *generator_class_name = class_name;
    }

    return true;
}

void CCmdLineOptions::ParseBitmapGenerator(const std::wstring& s)
{
    if (!icasecmp(L"null", s) && !icasecmp(L"default", s) && !icasecmp(L"gdi", s) && !icasecmp(L"freetype", s))
    {
        throw invalid_argument("invalid argument for bitmap-generator");
    }

    this->bitmapGeneratorClassName = convertToUtf8(s);
}

void CCmdLineOptions::PrintHelp(const wchar_t* sz, int switchesCnt, const std::function<std::tuple<int, std::wstring>(int idx)>& getSwitch)
{
    if (sz != nullptr)
    {
        if (icasecmp(sz, L"bitmapgen") || icasecmp(sz, L"bitmapgenerator"))
        {
            this->PrintHelpBitmapGenerator();
            return;
        }

        if (icasecmp(sz, L"build") || icasecmp(sz, L"buildinfo"))
        {
            this->PrintHelpBuildInfo();
            return;
        }
    }

    this->PrintHelp(switchesCnt, getSwitch);
}

void CCmdLineOptions::PrintHelpBuildInfo()
{
    int majorVer, minorVer;
    libCZI::BuildInformation buildInfo;
    libCZI::GetLibCZIVersion(&majorVer, &minorVer);
    libCZI::GetLibCZIBuildInformation(buildInfo);

    this->GetLog()->WriteLineStdOut("Build-Information");
    this->GetLog()->WriteLineStdOut("-----------------");
    this->GetLog()->WriteLineStdOut("");
    stringstream ss;
    ss << "version          : " << majorVer << "." << minorVer;
    this->GetLog()->WriteLineStdOut(ss.str());
    ss = stringstream();
    ss << "compiler         : " << buildInfo.compilerIdentification;
    this->GetLog()->WriteLineStdOut(ss.str());
    ss = stringstream();
    ss << "repository-URL   : " << buildInfo.repositoryUrl;
    this->GetLog()->WriteLineStdOut(ss.str());
    ss = stringstream();
    ss << "repository-branch: " << buildInfo.repositoryBranch;
    this->GetLog()->WriteLineStdOut(ss.str());
    ss = stringstream();
    ss << "repository-tag   : " << buildInfo.repositoryTag;
    this->GetLog()->WriteLineStdOut(ss.str());
}

void CCmdLineOptions::PrintHelp(int switchesCnt, const std::function<std::tuple<int, std::wstring>(int idx)>& getSwitch)
{
    this->PrintUsage(switchesCnt, getSwitch);
}

void CCmdLineOptions::PrintHelpBitmapGenerator()
{
    this->GetLog()->WriteLineStdOut("Available Bitmap-Generators:  [default class is denoted with '(*)']");
    this->GetLog()->WriteLineStdOut("");

    size_t maxLengthClassName = 0;
    BitmapGenFactory::EnumBitmapGenerator(
        [&](int no, std::tuple<std::string, std::string, bool> name_explanation_isdefault) -> bool
        {
            maxLengthClassName = (std::max)(get<0>(name_explanation_isdefault).length(), maxLengthClassName);
    return true;
        });

    stringstream ss;
    BitmapGenFactory::EnumBitmapGenerator(
        [&](int no, std::tuple<std::string, std::string, bool> name_explanation_isdefault) -> bool
        {
            ss << no + 1 << ": " << std::setw(maxLengthClassName) << std::left << get<0>(name_explanation_isdefault) << std::setw(0) <<
            (!get<2>(name_explanation_isdefault) ? "     " : " (*) ") << "\"" <<
        get<1>(name_explanation_isdefault) << "\"" << endl;
    return true;
        });

    this->GetLog()->WriteLineStdOut(ss.str());
}

/*static*/bool CCmdLineOptions::TryParseSubBlockMetadataKeyValue(const std::string& s, std::map<std::string, std::string>* subblock_metadata_property_bag)
{
    rapidjson::Document document;
    document.Parse(s.c_str());
    if (document.HasParseError())
    {
        return false;
    }

    if (!document.IsObject())
    {
        return false;
    }

    std::map<std::string, std::string> keyValue;
    for (rapidjson::Value::ConstMemberIterator itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr)
    {
        auto key = itr->name.GetString();
        const auto type = itr->value.GetType();
        string value;
        switch (type)
        {
        case rapidjson::kNumberType:
        {
            stringstream ss;
            ss << std::setprecision(numeric_limits<double>::digits10) << itr->value.GetDouble();
            value = ss.str();
            break;
        }
        case rapidjson::kStringType:
            value = itr->value.GetString();
            break;
        case rapidjson::kTrueType:
            value = "true";
            break;
        case rapidjson::kFalseType:
            value = "false";
            break;
        default:
            return false;
        }

        keyValue.insert(std::make_pair(key, value));
    }

    if (subblock_metadata_property_bag != nullptr)
    {
        subblock_metadata_property_bag->swap(keyValue);
    }

    return true;
}

void CCmdLineOptions::ParseSubBlockMetadataKeyValue(const std::string& s)
{
    rapidjson::Document document;
    document.Parse(s.c_str());
    if (document.HasParseError())
    {
        throw std::logic_error("Invalid JSON");
    }

    if (!document.IsObject())
    {
        throw std::logic_error("Invalid JSON");
    }

    std::map<std::string, std::string> keyValue;
    for (rapidjson::Value::ConstMemberIterator itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr)
    {
        auto key = itr->name.GetString();
        const auto type = itr->value.GetType();
        string value;
        switch (type)
        {
        case rapidjson::kNumberType:
        {
            stringstream ss;
            ss << std::setprecision(numeric_limits<double>::digits10) << itr->value.GetDouble();
            value = ss.str();
            break;
        }
        case rapidjson::kStringType:
            value = itr->value.GetString();
            break;
        case rapidjson::kTrueType:
            value = "true";
            break;
        case rapidjson::kFalseType:
            value = "false";
            break;
        default:
            throw std::logic_error("Invalid JSON");
        }

        keyValue.insert(std::make_pair(key, value));
    }

    this->sbBlkMetadataKeyValue = move(keyValue);
    this->sbBlkMetadataKeyValueValid = true;
}

/*static*/bool CCmdLineOptions::TryParseCompressionOptions(const std::string& s, libCZI::Utils::CompressionOption* compression_option)
{
    try
    {
        const libCZI::Utils::CompressionOption opt = libCZI::Utils::ParseCompressionOptions(s);
        if (compression_option != nullptr)
        {
            *compression_option = opt;
        }

        return true;
    }
    catch (exception&)
    {
        return false;
    }
}

void CCmdLineOptions::ParseCompressionOptions(const std::string& s)
{
    const libCZI::Utils::CompressionOption opt = libCZI::Utils::ParseCompressionOptions(s);
    this->compressionMode = opt.first;
    this->compressionParameters = opt.second;
}

void CCmdLineOptions::ParseGeneratorPixeltype(const std::string& s)
{
    auto pixeltypeString = trim(s);

    static const libCZI::PixelType possibleGeneratorPixeltypes[] = { libCZI::PixelType::Gray8,libCZI::PixelType::Gray16, libCZI::PixelType::Bgr24, libCZI::PixelType::Bgr48 };

    for (size_t i = 0; i < sizeof(possibleGeneratorPixeltypes) / sizeof(possibleGeneratorPixeltypes[0]); ++i)
    {
        if (icasecmp(pixeltypeString, libCZI::Utils::PixelTypeToInformalString(possibleGeneratorPixeltypes[i])))
        {
            this->pixelTypeForBitmapGenerator = possibleGeneratorPixeltypes[i];
            return;
        }
    }

    stringstream ss;
    ss << "Error parsing the generator-pixeltype - \"" << s << "\" is not valid.";
    throw logic_error(ss.str());
}

/*static*/ bool CCmdLineOptions::TryParseGeneratorPixeltype(const std::string& s, libCZI::PixelType* pixel_type)
{
    auto pixeltypeString = trim(s);

    static constexpr libCZI::PixelType possibleGeneratorPixeltypes[] = 
    {
        libCZI::PixelType::Gray8,
        libCZI::PixelType::Gray16,
        libCZI::PixelType::Bgr24,
        libCZI::PixelType::Bgr48
    };

    for (size_t i = 0; i < sizeof(possibleGeneratorPixeltypes) / sizeof(possibleGeneratorPixeltypes[0]); ++i)
    {
        if (icasecmp(pixeltypeString, libCZI::Utils::PixelTypeToInformalString(possibleGeneratorPixeltypes[i])))
        {
            if (pixel_type != nullptr)
            {
                *pixel_type = possibleGeneratorPixeltypes[i];
            }
            
            return true;
        }
    }

    return false;
}

/*static*/bool CCmdLineOptions::TryParseInt32(const std::string& str, int* value)
{
    const string trimmed_string = trim(str);
    size_t index;
    try
    {
        int v = stoi(trimmed_string, &index);
        if (trimmed_string.length() != index)
        {
            return false;
        }

        if (value != nullptr)
        {
            *value = v;
        }

        return true;
    }
    catch (invalid_argument&)
    {
        return false;
    }
    catch (out_of_range&)
    {
        return false;
    }
}

/*static*/bool CCmdLineOptions::TryParseRect(const std::string& str, bool* absolute_mode, int* x_position, int* y_position, int* width, int* height)
{
    const regex rect_regex(R"(((abs|rel)\(([\+|-]?[[:digit:]]+),([\+|-]?[[:digit:]]+)),([\+]?[[:digit:]]+),([\+]?[[:digit:]]+)\))");
    smatch pieces_match;

    if (regex_match(str, pieces_match, rect_regex))
    {
        if (pieces_match.size() == 7)
        {
            bool absOrRel;
            const ssub_match sub_match = pieces_match[2];
            if (sub_match.compare("abs") == 0)
            {
                absOrRel = true;
            }
            else
            {
                absOrRel = false;
            }

            if (absolute_mode != nullptr)
            {
                *absolute_mode = absOrRel;
            }

            int x;
            if (!TryParseInt32(pieces_match[3], &x))
            {
                return false;
            }

            int y;
            if (!TryParseInt32(pieces_match[4], &y))
            {
                return false;
            }

            int w;
            if (!TryParseInt32(pieces_match[5], &w) || w <= 0)
            {
                return false;
            }

            int h;
            if (!TryParseInt32(pieces_match[6], &h) || h <= 0)
            {
                return false;
            }

            if (x_position != nullptr)
            {
                *x_position = x;
            }

            if (y_position != nullptr)
            {
                *y_position = y;
            }

            if (width != nullptr)
            {
                *width = w;
            }

            if (height != nullptr)
            {
                *height = h;
            }

            return true;
        }
    }

    return false;
}
