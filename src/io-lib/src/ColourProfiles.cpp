// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "ColourProfiles.h"

#include <array>

namespace imageio {

namespace {

struct Named
{
    ColourSpace space;
    const char *name;
};

constexpr std::array NAMES{
    Named{ColourSpace::Srgb, "srgb"},
    Named{ColourSpace::AdobeRgb, "adobe-rgb"},
    Named{ColourSpace::ProPhoto, "prophoto"},
    Named{ColourSpace::LinearRec2020, "rec2020-linear"},
};

profiles::Profile rgbProfile(
    const cmsCIExyY &white, const cmsCIExyYTRIPLE &primaries, double gamma, const char *description)
{
    cmsToneCurve *curve = cmsBuildGamma(nullptr, gamma);
    cmsToneCurve *curves[3] = {curve, curve, curve};
    profiles::Profile profile{cmsCreateRGBProfile(&white, &primaries, curves)};
    cmsFreeToneCurve(curve);
    // The name programs show for the profile.
    cmsMLU *text = cmsMLUalloc(nullptr, 1);
    cmsMLUsetASCII(text, "en", "US", description);
    cmsWriteTag(profile.get(), cmsSigProfileDescriptionTag, text);
    cmsMLUfree(text);
    return profile;
}

} // namespace

std::string nameOf(ColourSpace space)
{
    for (const Named &named : NAMES) {
        if (named.space == space) {
            return named.name;
        }
    }
    return {};
}

std::optional<ColourSpace> colourSpaceNamed(std::string_view name)
{
    for (const Named &named : NAMES) {
        if (name == named.name) {
            return named.space;
        }
    }
    return std::nullopt;
}

std::vector<std::string> colourSpaceNames()
{
    std::vector<std::string> names;
    for (const Named &named : NAMES) {
        names.emplace_back(named.name);
    }
    return names;
}

namespace profiles {

namespace {

constexpr cmsCIExyY D65{0.3127, 0.3290, 1.0};
constexpr cmsCIExyY D50{0.3457, 0.3585, 1.0};

} // namespace

Profile workingSpace()
{
    return rgbProfile(
        D65,
        {{0.708, 0.292, 1.0}, {0.170, 0.797, 1.0}, {0.131, 0.046, 1.0}},
        1.0,
        "Linear Rec. 2020");
}

Profile of(ColourSpace space)
{
    switch (space) {
    case ColourSpace::Srgb: {
        Profile profile{cmsCreate_sRGBProfile()};
        return profile;
    }
    case ColourSpace::AdobeRgb:
        // Adobe RGB (1998): its primaries, D65, and a gamma of 563/256.
        return rgbProfile(
            D65,
            {{0.64, 0.33, 1.0}, {0.21, 0.71, 1.0}, {0.15, 0.06, 1.0}},
            563.0 / 256.0,
            "Adobe RGB (1998) compatible");
    case ColourSpace::ProPhoto:
        return rgbProfile(
            D50,
            {{0.7347, 0.2653, 1.0}, {0.1596, 0.8404, 1.0}, {0.0366, 0.0001, 1.0}},
            1.8,
            "ProPhoto RGB compatible");
    case ColourSpace::LinearRec2020:
        return workingSpace();
    }
    return workingSpace();
}

} // namespace profiles
} // namespace imageio
