// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "imageio/ColourSpace.h"

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace cli {

/// What the command line asks for.
struct Options
{
    /// The settings applied to every picture; without them, each picture's own sidecar.
    std::optional<std::filesystem::path> settings;
    std::filesystem::path output;
    imageio::ColourSpace colourSpace = imageio::ColourSpace::Srgb;
    bool compress = false;
    bool overwrite = false;
    std::vector<std::filesystem::path> inputs;
    bool help = false;
    bool version = false;
};

/// The options of the arguments, without the program's name; or what is wrong with them.
std::expected<Options, std::string> parseOptions(const std::vector<std::string> &arguments);

/// The help, as `--help` prints it.
std::string usage(const std::string &program);

} // namespace cli
