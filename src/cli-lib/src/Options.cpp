// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "cli/Options.h"

#include <sstream>

namespace cli {

std::expected<Options, std::string> parseOptions(const std::vector<std::string> &arguments)
{
    Options options;
    bool onlyFiles = false;
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        const std::string &argument = arguments[i];
        // The value of an option, the next argument.
        const auto value = [&]() -> std::expected<std::string, std::string> {
            if (i + 1 >= arguments.size()) {
                return std::unexpected{argument + " needs a value"};
            }
            return arguments[++i];
        };
        if (onlyFiles || argument.empty() || argument.front() != '-' || argument == "-") {
            options.inputs.emplace_back(argument);
        } else if (argument == "--") {
            onlyFiles = true;
        } else if (argument == "-h" || argument == "--help") {
            options.help = true;
        } else if (argument == "--version") {
            options.version = true;
        } else if (argument == "-s" || argument == "--settings") {
            const auto path = value();
            if (!path) {
                return std::unexpected{path.error()};
            }
            options.settings = *path;
        } else if (argument == "-o" || argument == "--output") {
            const auto path = value();
            if (!path) {
                return std::unexpected{path.error()};
            }
            options.output = *path;
        } else if (argument == "-c" || argument == "--colour-space" || argument == "--color-space") {
            const auto name = value();
            if (!name) {
                return std::unexpected{name.error()};
            }
            const auto space = imageio::colourSpaceNamed(*name);
            if (!space) {
                return std::unexpected{"unknown colour space " + *name};
            }
            options.colourSpace = *space;
        } else if (argument == "--compress") {
            options.compress = true;
        } else if (argument == "--overwrite") {
            options.overwrite = true;
        } else {
            return std::unexpected{"unknown option " + argument};
        }
    }
    if (options.help || options.version) {
        return options;
    }
    if (options.inputs.empty()) {
        return std::unexpected{std::string{"no picture given"}};
    }
    if (options.output.empty()) {
        return std::unexpected{std::string{"no output folder given: --output <folder>"}};
    }
    return options;
}

std::string usage(const std::string &program)
{
    std::ostringstream text;
    text << "Usage: " << program << " [options] --output <folder> <picture>...\n"
         << "\n"
         << "Grades each picture, a TIFF, a DNG or a camera raw file, with the settings of an XMP\n"
         << "sidecar, and writes it as a 16-bit TIFF into the output folder, with the name of the\n"
         << "picture and the extension .tif.\n"
         << "\n"
         << "Options:\n"
         << "  -s, --settings <file.xmp>    The settings for every picture. Without it, each\n"
         << "                               picture's own sidecar, <picture>.xmp, or no edit.\n"
         << "  -o, --output <folder>        Where the TIFFs go; made if missing.\n"
         << "  -c, --colour-space <name>    The TIFFs' colour space, its ICC profile embedded:\n"
         << "                               srgb (default), adobe-rgb, prophoto, rec2020-linear.\n"
         << "      --compress               Deflate, lossless: about half the size.\n"
         << "      --overwrite              Replace TIFFs already in the output folder.\n"
         << "  -h, --help                   This help.\n"
         << "      --version                The version.\n"
         << "\n"
         << "Exit status: 0 when every picture is graded, 1 when some are not, 2 for a wrong\n"
         << "command line or settings that cannot be read.\n";
    return text.str();
}

} // namespace cli
