// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Effect.h"

#include <array>

namespace core {

/**
 * The hue, the saturation and the luminance of eight bands of colour, as raw editors have them:
 * red, orange, yellow, green, aqua, blue, purple and magenta. A colour between two bands takes some
 * of each, so the bands blend without a seam, and greys are left alone. Measured in Oklab, as the
 * saturation and the vibrance are, so a hue moves as the eye sees it.
 */
class ColorMixerEffect : public Effect
{
public:
    static constexpr int BANDS = 8;

    /// The bands' names, as they end the parameters' ids, e.g. "HueAdjustmentRed".
    static constexpr std::array<const char *, BANDS>
        BAND_NAMES{"Red", "Orange", "Yellow", "Green", "Aqua", "Blue", "Purple", "Magenta"};

    /// The parameters of each kind, band by band.
    static const std::array<Parameter, BANDS> &hues();
    static const std::array<Parameter, BANDS> &saturations();
    static const std::array<Parameter, BANDS> &luminances();

    std::string section() const override { return "Color Mixer"; }
    std::vector<Parameter> parameters() const override;
    void apply(Image &image, const Settings &settings, const RenderContext &context) const override;

    /// The Oklab hue, in degrees, at the middle of each band: that of the pure sRGB colour.
    static const std::array<double, BANDS> &bandHues();
};

} // namespace core
