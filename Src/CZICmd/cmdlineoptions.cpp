// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "stdafx.h"
#include "cmdlineoptions.h"
#include "inc_libCZI.h"
#include <clocale>
#include <locale>
#include <regex>
#include <iostream>
#include <utility>
#include <cstring>
#include <cmath>
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

/// CLI11-validator for the option "--cachesize".
struct CachesizeValidator : public CLI::Validator
{
    CachesizeValidator()
    {
        this->name_ = "CachesizeValidator";
        this->func_ = [](const std::string& str) -> string
            {
                const bool parsed_ok = CCmdLineOptions::TryParseSubBlockCacheSize(str, nullptr);
                if (!parsed_ok)
                {
                    ostringstream string_stream;
                    string_stream << "Invalid subblock-cache-size given \"" << str << "\"";
                    throw CLI::ValidationError(string_stream.str());
                }

                return {};
            };
    }
};

struct TileSizeForPlaneScanValidator : public CLI::Validator
{
    TileSizeForPlaneScanValidator()
    {
        this->name_ = "TileSizeForPlaneScanValidator";
        this->func_ = [](const std::string& str) -> string
            {
                const bool parsed_ok = CCmdLineOptions::TryParseCreateSize(str, nullptr);
                if (!parsed_ok)
                {
                    ostringstream string_stream;
                    string_stream << "Invalid tile-size-plane-scan given \"" << str << "\"";
                    throw CLI::ValidationError(string_stream.str());
                }

                return {};
            };
    }
};

/// A custom formatter for CLI11 - used to have nicely formatted descriptions.
class CustomFormatter : public CLI::Formatter
{
public:
    CustomFormatter() : Formatter()
    {
        this->column_width(20);
    }

    std::string make_usage(const CLI::App* app, std::string name) const override
    {
        // 'name' is the full path of the executable, we only take the path "after the last slash or backslash"
        size_t offset = name.rfind('/');
        if (offset == string::npos)
        {
            offset = name.rfind('\\');
        }

        if (offset != string::npos && offset < name.length())
        {
            name = name.substr(1 + offset);
        }

        const auto result_from_stock_implementation = this->CLI::Formatter::make_usage(app, name);
        ostringstream ss(result_from_stock_implementation);
        int majorVersion, minorVersion, patchVersion;
        libCZI::GetLibCZIVersion(&majorVersion, &minorVersion, &patchVersion);
        ss << result_from_stock_implementation << endl << "  using libCZI version " << majorVersion << "." << minorVersion << "." << patchVersion << endl;
        return ss.str();
    }

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

CCmdLineOptions::CCmdLineOptions(std::shared_ptr<ILog> log)
    : log(std::move(log))
{
    this->Clear();
}

CCmdLineOptions::ParseResult CCmdLineOptions::Parse(int argc, char** argv)
{
    CLI::App cli_app{ };

    // specify the string-to-enum-mapping for "command"
    std::map<string, Command> map_string_to_command
    {
        { "PrintInformation",                   Command::PrintInformation },
        { "ExtractSubBlock",                    Command::ExtractSubBlock },
        { "SingleChannelTileAccessor",          Command::SingleChannelTileAccessor },
        { "ChannelComposite",                   Command::ChannelComposite },
        { "SingleChannelPyramidTileAccessor",   Command::SingleChannelPyramidTileAccessor },
        { "SingleChannelScalingTileAccessor",   Command::SingleChannelScalingTileAccessor },
        { "ScalingChannelComposite",            Command::ScalingChannelComposite },
        { "ExtractAttachment",                  Command::ExtractAttachment},
        { "CreateCZI",                          Command::CreateCZI },
        { "PlaneScan",                          Command::PlaneScan },
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
    const static CachesizeValidator cachesize_validator;
    const static TileSizeForPlaneScanValidator tile_size_for_plane_scan_validator;

    Command argument_command;
    string argument_source_filename;
    string argument_output_filename;
    string argument_plane_coordinate;
    string argument_rect;
    string argument_display_settings;
    bool argument_calc_hash = false;
    bool argument_drawtileboundaries = false;
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
    string argument_subblock_cachesize;
    string argument_tilesize_for_scan;
    bool argument_versionflag = false;
    string argument_source_stream_class;
    string argument_source_stream_creation_propbag;
    bool argument_use_visibility_check_optimization = false;

    // editorconfig-checker-disable
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
           \N'SingleChannelPyramidTileAccessor' adds to the previous command the ability to explicitly address a specific pyramid-layer (which must
           exist in the CZI-document).
           \N'SingleChannelScalingTileAccessor' gets the specified region with an arbitrary zoom factor. It uses the pyramid-layers in the CZI-document
           and scales the bitmap if necessary. The resulting bitmap will be written to the specified OUTPUTFILE.
           \N'ScalingChannelComposite' operates like the previous command, but in addition gets all channels and creates a multi-channel-composite from them
           using display-settings.
           \N'ExtractAttachment' allows to extract (and save to a file) the contents of attachments.)
           \N'CreateCZI' is used to demonstrate the CZI-creation capabilities of libCZI.)
           \N'PlaneScan' does the following: over a ROI given with the --rect option a rectangle of size given with 
           the --tilesize-for-plane-scan option is moved, and the image content of this rectangle is written out to
           files. The operation takes place on a plane which is given with the --plane-coordinate option. The filenames of the
           tile-bitmaps are generated from the filename given with the --output option, where a string _X[x-position]_Y[y-position]_W[width]_H[height]
           is added.)")
        ->default_val(Command::Invalid)
        ->option_text("COMMAND")
        ->transform(CLI::CheckedTransformer(map_string_to_command, CLI::ignore_case));
    // editorconfig-checker-enable
    cli_app.add_option("-s,--source", argument_source_filename,
        "Specifies the source CZI-file.")
        ->option_text("SOURCEFILE");
    cli_app.add_option("--source-stream-class", argument_source_stream_class,
        "Specifies the stream-class used for reading the source CZI-file. If not specified, the default file-reader stream-class is used."
        " Run with argument '--version' to get a list of available stream-classes.")
        ->option_text("STREAMCLASS");
    cli_app.add_option("--propbag-source-stream-creation", argument_source_stream_creation_propbag,
        "Specifies the property-bag used for creating the stream used for reading the source CZI-file. The data is given in JSON-notation.")
        ->option_text("PROPBAG");
    cli_app.add_option("-o,--output", argument_output_filename,
        "specifies the output-filename. A suffix will be appended to the name given here depending on the type of the file.")
        ->option_text("OUTPUTFILE");
    // editorconfig-checker-disable
    cli_app.add_option("-p,--plane-coordinate", argument_plane_coordinate,
        R"(Uniquely select a 2D-plane from the document. It is given in the form [DimChar][number], where 'DimChar' specifies a dimension and 
           can be any of 'Z', 'C', 'T', 'R', 'I', 'H', 'V' or 'B'. 'number' is an integer. \nExamples: C1T3, C0T-2, C1T44Z15H1.)")
        ->option_text("PLANE-COORDINATE")
        ->check(plane_coordinate_validator);
    // editorconfig-checker-enable
    // editorconfig-checker-disable
    cli_app.add_option("-r,--rect", argument_rect,
        R"(Select a paraxial rectangular region as the region-of-interest. The coordinates may be given either absolute or relative. If using relative
            coordinates, they are relative to what is determined as the upper-left point in the document.\nRelative coordinates are specified with
            the syntax 'rel([x],[y],[width],[height])', absolute coordinates are specified 'abs([x],[y],[width],[height])'.
            \nExamples: rel(0, 0, 1024, 1024), rel(-100, -100, 500, 500), abs(-230, 100, 800, 800).)")
        ->option_text("ROI")
        ->check(region_of_interest_validator);
    // editorconfig-checker-enable
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
        "Run with argument '--version' to get a list of available bitmap-generators.")
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
        "then followed by a list of key-value pairs which are separated by a semicolon. Examples: \"zstd0:ExplicitLevel=3\", \"zstd1:ExplicitLevel=2;PreProcess=HiLoByteUnpack\".")
        ->option_text("COMPRESSIONDESCRIPTION")
        ->check(compressionoptions_validator);
    cli_app.add_option("--generatorpixeltype", argument_generatorpixeltype,
        "Only used for 'CreateCZI': a string defining the pixeltype used by the bitmap - generator. Possible values are 'Gray8', 'Gray16', "
        "'Bgr24' or 'Bgr48'. Default is 'Bgr24'.")
        ->option_text("PIXELTYPE")
        ->check(generatorpixeltype_validator);
    cli_app.add_option("--cachesize", argument_subblock_cachesize,
        "Only used for 'PlaneScan' - specify the size of the subblock-cache in bytes. The argument is to "
        "be given with a suffix k, M, G, ...")
        ->option_text("CACHESIZE")
        ->check(cachesize_validator);
    cli_app.add_option("--tilesize-for-plane-scan", argument_tilesize_for_scan,
        "Only used for 'PlaneScan' - specify the size of ROI which is used for scanning the plane in "
        "units of pixels. Format is e.g. '1600x1200' and default is 512x512.")
        ->option_text("TILESIZE")
        ->check(tile_size_for_plane_scan_validator);
    cli_app.add_flag("--use-visibility-check-optimization", argument_use_visibility_check_optimization,
        "Whether to enable the experimental \"visibility check optimization\" for the accessors.");
    cli_app.add_flag("--version", argument_versionflag,
        "Print extended version-info and supported operations, then exit.");

    auto formatter = make_shared<CustomFormatter>();
    cli_app.formatter(formatter);

    try
    {
        cli_app.parse(argc, argv);
    }
    catch (const CLI::CallForHelp& e)
    {
        cli_app.exit(e);
        return ParseResult::Exit;
    }
    catch (const CLI::ParseError& e)
    {
        cli_app.exit(e);
        return ParseResult::Error;
    }

    if (argument_versionflag)
    {
        this->PrintHelpBuildInfo();
        this->GetLog()->WriteLineStdOut("");
        this->GetLog()->WriteLineStdOut("");
        this->PrintHelpBitmapGenerator();
        this->PrintHelpStreamsObjects();
        return ParseResult::Exit;
    }

    this->calcHashOfResult = argument_calc_hash;
    this->drawTileBoundaries = argument_drawtileboundaries;
    this->command = argument_command;
    this->useVisibilityCheckOptimization = argument_use_visibility_check_optimization;

    try
    {
        if (!argument_source_filename.empty())
        {
            this->cziFilename = convertUtf8ToUCS2(argument_source_filename);
        }

        if (!argument_source_stream_class.empty())
        {
            this->source_stream_class = argument_source_stream_class;
        }

        if (!argument_source_stream_creation_propbag.empty())
        {
            const bool b = TryParseInputStreamCreationPropertyBag(argument_source_stream_creation_propbag, &this->property_bag_for_stream_class);
            ThrowIfFalse(b, "--propbag-source-stream-creation", argument_source_stream_creation_propbag);
        }

        if (!argument_output_filename.empty())
        {
            this->SetOutputFilename(convertUtf8ToUCS2(argument_output_filename));
        }

        if (!argument_plane_coordinate.empty())
        {
            this->planeCoordinate = libCZI::CDimCoordinate::Parse(argument_plane_coordinate.c_str());
        }

        if (!argument_rect.empty())
        {
            const bool b = TryParseRect(argument_rect, &this->rectModeAbsoluteOrRelative, &this->rectX, &this->rectY, &this->rectW, &this->rectH);
            ThrowIfFalse(b, "-r,--rect", argument_rect);
        }

        if (!argument_display_settings.empty())
        {
            const bool b = TryParseDisplaySettings(argument_display_settings, &this->multiChannelCompositeChannelInfos);
            ThrowIfFalse(b, "-d,--display-settings", argument_display_settings);
            this->useDisplaySettingsFromDocument = false;
        }

        if (!argument_jpgxrcodec.empty())
        {
            const bool b = TryParseJxrCodecUseWicCodec(argument_jpgxrcodec, &this->useWicJxrDecoder);
            ThrowIfFalse(b, "-j,--jpgxrcodec", argument_jpgxrcodec);
        }

        if (!argument_verbosity.empty())
        {
            const bool b = TryParseVerbosityLevel(argument_verbosity, &this->enabledOutputLevels);
            ThrowIfFalse(b, "-v,--verbosity", argument_verbosity);
        }

        if (!argument_backgroundcolor.empty())
        {
            const bool b = TryParseParseBackgroundColor(argument_backgroundcolor, &this->backGroundColor);
            ThrowIfFalse(b, "-b,--background", argument_backgroundcolor);
        }

        if (!argument_pyramidinfo.empty())
        {
            const bool b = TryParsePyramidInfo(argument_pyramidinfo, &this->pyramidMinificationFactor, &this->pyramidLayerNo);
            ThrowIfFalse(b, "-y,--pyramidinfo", argument_pyramidinfo);
        }

        if (!argument_zoom.empty())
        {
            const bool b = TryParseZoom(argument_zoom, &this->zoom);
            ThrowIfFalse(b, "-z,--zoom", argument_pyramidinfo);
        }

        if (!argument_info_level.empty())
        {
            const bool b = TryParseInfoLevel(argument_info_level, &this->infoLevel);
            ThrowIfFalse(b, "-i,--info-level", argument_info_level);
        }

        if (!argument_selection.empty())
        {
            const bool b = TryParseSelection(argument_selection, &this->mapSelection);
            ThrowIfFalse(b, "-e,--selection", argument_selection);
        }

        if (!argument_tile_filter.empty())
        {
            const bool b = TryParseTileFilter(argument_tile_filter, &this->sceneIndexSet);
            ThrowIfFalse(b, "-f,--tile-filter", argument_tile_filter);
        }

        if (!argument_channelcompositionformat.empty())
        {
            const bool b = TryParseChannelCompositionFormat(argument_channelcompositionformat, &this->channelCompositePixelType, &this->channelCompositeAlphaValue);
            ThrowIfFalse(b, "-m,--channelcompositionformat", argument_channelcompositionformat);
        }

        if (!arguments_createbounds.empty())
        {
            const bool b = TryParseCreateBounds(arguments_createbounds, &this->createBounds);
            ThrowIfFalse(b, "--createbounds", arguments_createbounds);
        }

        if (!arguments_createsubblocksize.empty())
        {
            const bool b = TryParseCreateSize(arguments_createsubblocksize, &this->createSize);
            ThrowIfFalse(b, "--createsubblocksize", arguments_createsubblocksize);
        }

        if (!argument_createtileinfo.empty())
        {
            const bool b = TryParseCreateTileInfo(argument_createtileinfo, &this->createTileInfo);
            ThrowIfFalse(b, "--createtileinfo", argument_createtileinfo);
        }

        if (!argument_truetypefontname.empty())
        {
            this->fontnameOrFile = convertUtf8ToUCS2(argument_truetypefontname);
        }

        if (!argument_fontheight.empty())
        {
            const bool b = TryParseFontHeight(argument_fontheight, &this->fontHeight);
            ThrowIfFalse(b, "--fontheight", argument_fontheight);
        }

        if (!argument_guidofczi.empty())
        {
            const bool b = TryParseNewCziFileguid(argument_guidofczi, &this->newCziFileGuid);
            ThrowIfFalse(b, "--guidofczi", argument_guidofczi);
            this->newCziFileGuidValid = true;
        }

        if (!argument_bitmapgenerator.empty())
        {
            const bool b = TryParseBitmapGenerator(argument_bitmapgenerator, &this->bitmapGeneratorClassName);
            ThrowIfFalse(b, "--bitmapgenerator", argument_bitmapgenerator);
        }

        if (!argument_createczisubblockmetadata.empty())
        {
            const bool b = TryParseSubBlockMetadataKeyValue(argument_createczisubblockmetadata, &this->sbBlkMetadataKeyValue);
            ThrowIfFalse(b, "--createczisbblkmetadata", argument_createczisubblockmetadata);
            this->sbBlkMetadataKeyValueValid = true;
        }

        if (!argument_compressionoptions.empty())
        {
            libCZI::Utils::CompressionOption compression_options;
            const bool b = TryParseCompressionOptions(argument_compressionoptions, &compression_options);
            ThrowIfFalse(b, "--compressionopts", argument_compressionoptions);
            this->compressionMode = compression_options.first;
            this->compressionParameters = compression_options.second;
        }

        if (!argument_generatorpixeltype.empty())
        {
            const bool b = TryParseGeneratorPixeltype(argument_generatorpixeltype, &this->pixelTypeForBitmapGenerator);
            ThrowIfFalse(b, "--generatorpixeltype", argument_generatorpixeltype);
        }

        if (!argument_subblock_cachesize.empty())
        {
            const bool b = TryParseSubBlockCacheSize(argument_subblock_cachesize, &this->subBlockCacheSize);
            ThrowIfFalse(b, "--cachesize", argument_subblock_cachesize);
        }

        if (!argument_tilesize_for_scan.empty())
        {
            const bool b = TryParseCreateSize(argument_tilesize_for_scan, &this->tilesSizeForPlaneScan);
            ThrowIfFalse(b, "--tilesize-for-plane-scan", argument_tilesize_for_scan);
        }
    }
    catch (runtime_error& exception)
    {
        this->GetLog()->WriteLineStdErr(exception.what());
        return ParseResult::Error;
    }

    return this->CheckArgumentConsistency() ? ParseResult::OK : ParseResult::Error;
}

/*static*/void CCmdLineOptions::ThrowIfFalse(bool b, const std::string& argument_switch, const std::string& argument)
{
    if (!b)
    {
        ostringstream string_stream;
        string_stream << "Error parsing argument for '" << argument_switch << "' -> \"" << argument << "\".";
        throw runtime_error(string_stream.str());
    }
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
    this->zoom = 1;
    this->pyramidLayerNo = -1;
    this->pyramidMinificationFactor = -1;
    this->createTileInfo.rows = this->createTileInfo.columns = 1;
    this->createTileInfo.overlap = 0;
    this->compressionMode = libCZI::CompressionMode::Invalid;
    this->compressionParameters = nullptr;
    this->pixelTypeForBitmapGenerator = libCZI::PixelType::Bgr24;
    this->subBlockCacheSize = 0;
    this->tilesSizeForPlaneScan = make_tuple(512, 512);
    this->useVisibilityCheckOptimization = false;
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
        {
            break;
        }

        double d2 = it->GetDouble();
        result.emplace_back(d1, d2);
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
        chInfo.blackPoint = static_cast<float>(v["black-point"].GetDouble());
    }
    else
    {
        chInfo.blackPoint = 0;
    }

    if (v.HasMember("white-point"))
    {
        chInfo.whitePoint = static_cast<float>(v["white-point"].GetDouble());
    }
    else
    {
        chInfo.whitePoint = 1;
    }

    if (v.HasMember("weight"))
    {
        chInfo.weight = static_cast<float>(v["weight"].GetDouble());
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
        chInfo.gamma = static_cast<float>(v["gamma"].GetDouble());
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
            multiChannelCompositeChannelInfos->operator[](get<0>(it)) = get<1>(it);
        }
    }

    return true;
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
        if (*endPtrSkipped == '\0')
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
            *color = libCZI::RgbFloatColor{ f[0], f[0], f[0] };
        }
    }
    else
    {
        if (color != nullptr)
        {
            *color = libCZI::RgbFloatColor{ f[0], f[1], f[2] };
        }
    }

    return true;
}

/*static*/bool CCmdLineOptions::TryParsePyramidInfo(const std::string& s, int* pyramidMinificationFactor, int* pyramidLayerNo)
{
    const size_t position_of_delimiter = s.find_first_of(";,|");
    if (position_of_delimiter == string::npos)
    {
        return false;
    }

    const string minification_factor_string = s.substr(0, position_of_delimiter);
    const string pyramid_layer_no_string = s.substr(1 + position_of_delimiter);

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

/*static*/bool CCmdLineOptions::TryParseZoom(const std::string& s, float* zoom)
{
    try
    {
        float zoom_value = stof(s);
        if (zoom != nullptr)
        {
            *zoom = zoom_value;
        }

        return true;
    }
    catch (exception&)
    {
        return false;
    }
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

std::shared_ptr<libCZI::IIndexSet> CCmdLineOptions::GetSceneIndexSet() const
{
    return this->sceneIndexSet;
}

/*static*/bool CCmdLineOptions::TryParseChannelCompositionFormat(const std::string& s, libCZI::PixelType* channel_composition_format, std::uint8_t* channel_composition_alpha_value)
{
    const auto arg = trim(s);
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

    if (!TryParseChannelCompositionFormatWithAlphaValue(arg, pixel_type, alpha))
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

/*static*/bool CCmdLineOptions::TryParseChannelCompositionFormatWithAlphaValue(const std::string& s, libCZI::PixelType& channelCompositePixelType, std::uint8_t& channelCompositeAlphaValue)
{
    std::regex regex(R"_(bgra32\((\d+|0x[\d|a-f|A-F]+)\))_", regex_constants::ECMAScript | regex_constants::icase);
    std::smatch pieces_match;
    if (std::regex_match(s, pieces_match, regex))
    {
        if (pieces_match.size() == 2)
        {
            std::ssub_match sub_match = pieces_match[1];
            if (sub_match.length() > 2)
            {
                if (sub_match.str()[0] == '0' && (sub_match.str()[1] == 'x' || sub_match.str()[0] == 'X'))
                {
                    auto hexStr = string{ sub_match };
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

            const uint32_t overlapPercent = static_cast<uint32_t>(v);

            if (create_tile_info != nullptr)
            {
                create_tile_info->rows = rows;
                create_tile_info->columns = cols;
                create_tile_info->overlap = overlapPercent / 100.0f;
            }

            return true;
        }
    }

    return false;
}


/*static*/bool CCmdLineOptions::TryParseFontHeight(const std::string& s, int* font_height)
{
    return CCmdLineOptions::TryParseInt32(s, font_height);
}

/*static*/bool CCmdLineOptions::TryParseNewCziFileguid(const std::string& s, libCZI::GUID* guid)
{
    libCZI::GUID g;
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

void CCmdLineOptions::PrintHelpBuildInfo()
{
    int majorVer, minorVer, patchVer;
    libCZI::BuildInformation buildInfo;
    libCZI::GetLibCZIVersion(&majorVer, &minorVer, &patchVer);
    libCZI::GetLibCZIBuildInformation(buildInfo);

    this->GetLog()->WriteLineStdOut("Build-Information");
    this->GetLog()->WriteLineStdOut("-----------------");
    this->GetLog()->WriteLineStdOut("");
    stringstream ss;
    ss << "version          : " << majorVer << "." << minorVer << "." << patchVer;
    this->GetLog()->WriteLineStdOut(ss.str());
    ss.clear();
    ss.str("");
    ss << "compiler         : " << buildInfo.compilerIdentification;
    this->GetLog()->WriteLineStdOut(ss.str());
    ss.clear();
    ss.str("");
    ss << "repository-URL   : " << buildInfo.repositoryUrl;
    this->GetLog()->WriteLineStdOut(ss.str());
    ss.clear();
    ss.str("");
    ss << "repository-branch: " << buildInfo.repositoryBranch;
    this->GetLog()->WriteLineStdOut(ss.str());
    ss.clear();
    ss.str("");
    ss << "repository-tag   : " << buildInfo.repositoryTag;
    this->GetLog()->WriteLineStdOut(ss.str());
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

    ostringstream string_stream;
    BitmapGenFactory::EnumBitmapGenerator(
        [&](int no, std::tuple<std::string, std::string, bool> name_explanation_isdefault) -> bool
        {
            string_stream << no + 1 << ": " << std::setw(maxLengthClassName) << std::left << get<0>(name_explanation_isdefault) << std::setw(0) <<
                (!get<2>(name_explanation_isdefault) ? "     " : " (*) ") << "\"" <<
                get<1>(name_explanation_isdefault) << "\"" << endl;
            return true;
        });

    this->GetLog()->WriteLineStdOut(string_stream.str());
}

void CCmdLineOptions::PrintHelpStreamsObjects()
{
    this->GetLog()->WriteLineStdOut("Available Input-Stream objects:");
    this->GetLog()->WriteLineStdOut("");

    const int stream_object_count = libCZI::StreamsFactory::GetStreamClassesCount();
    ostringstream string_stream;
    for (int i = 0; i < stream_object_count; ++i)
    {
        libCZI::StreamsFactory::StreamClassInfo stream_class_info;
        libCZI::StreamsFactory::GetStreamInfoForClass(i, stream_class_info);

        string_stream << i + 1 << ": " << stream_class_info.class_name << endl;
        string_stream << "    " << stream_class_info.short_description << endl;

        if (stream_class_info.get_build_info)
        {
            string build_info = stream_class_info.get_build_info();
            if (!build_info.empty())
            {
                string_stream << "    " << "Build: " << build_info << endl;
            }
        }
    }

    this->GetLog()->WriteLineStdOut(string_stream.str());
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

/*static*/ bool CCmdLineOptions::TryParseGeneratorPixeltype(const std::string& s, libCZI::PixelType* pixel_type)
{
    auto pixeltype_string = trim(s);

    static constexpr libCZI::PixelType possible_generator_pixeltypes[] =
    {
        libCZI::PixelType::Gray8,
        libCZI::PixelType::Gray16,
        libCZI::PixelType::Bgr24,
        libCZI::PixelType::Bgr48
    };

    for (size_t i = 0; i < sizeof(possible_generator_pixeltypes) / sizeof(possible_generator_pixeltypes[0]); ++i)
    {
        if (icasecmp(pixeltype_string, libCZI::Utils::PixelTypeToInformalString(possible_generator_pixeltypes[i])))
        {
            if (pixel_type != nullptr)
            {
                *pixel_type = possible_generator_pixeltypes[i];
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

/*static*/bool CCmdLineOptions::TryParseInputStreamCreationPropertyBag(const std::string& s, std::map<int, libCZI::StreamsFactory::Property>* property_bag)
{
    // Here we parse the JSON-formatted string that contains the property bag for the input stream and
    //  construct a map<int, libCZI::StreamsFactory::Property> from it.

    static constexpr struct
    {
        const char* name;
        int stream_property_id;
        libCZI::StreamsFactory::Property::Type property_type;
    }
    kKeyStringToId[] =
    {
        {"CurlHttp_Proxy", libCZI::StreamsFactory::StreamProperties::kCurlHttp_Proxy, libCZI::StreamsFactory::Property::Type::String},
        {"CurlHttp_UserAgent", libCZI::StreamsFactory::StreamProperties::kCurlHttp_UserAgent, libCZI::StreamsFactory::Property::Type::String},
        {"CurlHttp_Timeout", libCZI::StreamsFactory::StreamProperties::kCurlHttp_Timeout, libCZI::StreamsFactory::Property::Type::Int32},
        {"CurlHttp_ConnectTimeout", libCZI::StreamsFactory::StreamProperties::kCurlHttp_ConnectTimeout, libCZI::StreamsFactory::Property::Type::Int32},
        {"CurlHttp_Xoauth2Bearer", libCZI::StreamsFactory::StreamProperties::kCurlHttp_Xoauth2Bearer, libCZI::StreamsFactory::Property::Type::String},
        {"CurlHttp_Cookie", libCZI::StreamsFactory::StreamProperties::kCurlHttp_Cookie, libCZI::StreamsFactory::Property::Type::String},
        {"CurlHttp_SslVerifyPeer", libCZI::StreamsFactory::StreamProperties::kCurlHttp_SslVerifyPeer, libCZI::StreamsFactory::Property::Type::Boolean},
        {"CurlHttp_SslVerifyHost", libCZI::StreamsFactory::StreamProperties::kCurlHttp_SslVerifyHost, libCZI::StreamsFactory::Property::Type::Boolean},
        {"CurlHttp_FollowLocation", libCZI::StreamsFactory::StreamProperties::kCurlHttp_FollowLocation, libCZI::StreamsFactory::Property::Type::Boolean},
        {"CurlHttp_MaxRedirs", libCZI::StreamsFactory::StreamProperties::kCurlHttp_MaxRedirs, libCZI::StreamsFactory::Property::Type::Int32},
    };

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
        for (size_t i = 0; i < sizeof(kKeyStringToId) / sizeof(kKeyStringToId[0]); ++i)
        {
            if (name == kKeyStringToId[i].name)
            {
                index_of_key = i;
                break;
            }
        }

        if (index_of_key == numeric_limits<size_t>::max())
        {
            return false;
        }

        switch (kKeyStringToId[index_of_key].property_type)
        {
        case libCZI::StreamsFactory::Property::Type::String:
            if (!itr->value.IsString())
            {
                return false;
            }

            if (property_bag != nullptr)
            {
                property_bag->insert(std::make_pair(kKeyStringToId[index_of_key].stream_property_id, libCZI::StreamsFactory::Property(itr->value.GetString())));
            }

            break;
        case libCZI::StreamsFactory::Property::Type::Boolean:
            if (!itr->value.IsBool())
            {
                return false;
            }

            if (property_bag != nullptr)
            {
                property_bag->insert(std::make_pair(kKeyStringToId[index_of_key].stream_property_id, libCZI::StreamsFactory::Property(itr->value.GetBool())));
            }

            break;
        case libCZI::StreamsFactory::Property::Type::Int32:
            if (!itr->value.IsInt())
            {
                return false;
            }

            if (property_bag != nullptr)
            {
                property_bag->insert(std::make_pair(kKeyStringToId[index_of_key].stream_property_id, libCZI::StreamsFactory::Property(itr->value.GetInt())));
            }

            break;
        default:
            // this actually indicates an internal error - the table kKeyStringToId contains a not yet implemented property type
            return false;
        }
    }

    return true;
}

/*static*/bool CCmdLineOptions::TryParseSubBlockCacheSize(const std::string& text, std::uint64_t* size)
{
    // This regular expression is used to match strings that represent sizes in bytes, kilobytes, megabytes, gigabytes, terabytes, kibibytes, mebibytes, gibibytes, and tebibytes. 
    //
    // Here is a breakdown of the regular expression:
    //
    // - ^\s*: Matches the start of the string, followed by any amount of whitespace.
    // - ([+]?(?:[0-9]+(?:[.][0-9]*)?|[.][0-9]+)): Matches a positive number, which can be an integer or a decimal. The number may optionally be preceded by a plus sign.
    // - \s*: Matches any amount of whitespace following the number.
    // - (k|m|g|t|ki|mi|gi|ti): Matches one of the following units of size: k (kilobytes), m (megabytes), g (gigabytes), t (terabytes), ki (kibibytes), mi (mebibytes), gi (gibibytes), ti (tebibytes).
    // - (?:b?): Optionally matches a 'b', which can be used to explicitly specify that the size is in bytes.
    // - \s*$: Matches any amount of whitespace at the end of the string, followed by the end of the string.
    //
    // This regular expression is case-insensitive.
    regex regex(R"(^\s*([+]?(?:[0-9]+(?:[.][0-9]*)?|[.][0-9]+))\s*(k|m|g|t|ki|mi|gi|ti)(?:b?)\s*$)", regex_constants::icase);
    smatch match;
    regex_search(text, match, regex);
    if (match.size() != 3)
    {
        return false;
    }

    double number;

    try
    {
        number = stod(match[1].str());
    }
    catch (invalid_argument&)
    {
        return false;
    }
    catch (out_of_range&)
    {
        return false;
    }

    uint64_t factor;
    string suffix_string = match[2].str();
    if (icasecmp(suffix_string, "k"))
    {
        factor = 1000;
    }
    else if (icasecmp(suffix_string, "ki"))
    {
        factor = 1024;
    }
    else if (icasecmp(suffix_string, "m"))
    {
        factor = 1000 * 1000;
    }
    else if (icasecmp(suffix_string, "mi"))
    {
        factor = 1024 * 1024;
    }
    else if (icasecmp(suffix_string, "g"))
    {
        factor = 1000 * 1000 * 1000;
    }
    else if (icasecmp(suffix_string, "gi"))
    {
        factor = 1024 * 1024 * 1024;
    }
    else if (icasecmp(suffix_string, "t"))
    {
        factor = 1000ULL * 1000 * 1000 * 1000;
    }
    else if (icasecmp(suffix_string, "ti"))
    {
        factor = 1024ULL * 1024 * 1024 * 1024;
    }
    else
    {
        return false;
    }

    const uint64_t memory_size = llround(number * static_cast<double>(factor));
    if (size != nullptr)
    {
        *size = memory_size;
    }

    return true;
}
