// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace imageio {

/// The colour spaces an image is written in, each with its ICC profile embedded.
enum class ColourSpace {
    /// sRGB, with its curve: what every program and screen assumes; saturated colours clip.
    Srgb,
    /// Adobe RGB (1998), gamma 2.2: wider in the greens and cyans, the usual space for prints.
    AdobeRgb,
    /// ProPhoto RGB, gamma 1.8, D50: wider than any printer; best kept in 16 bits.
    ProPhoto,
    /// The working space itself, linear Rec. 2020: for more processing, not for viewing.
    LinearRec2020,
};

/// The name given on the command line, e.g. "adobe-rgb".
std::string nameOf(ColourSpace space);
/// The colour space of a name, or nothing.
std::optional<ColourSpace> colourSpaceNamed(std::string_view name);
/// Every name, in the order of the enumeration.
std::vector<std::string> colourSpaceNames();

} // namespace imageio
