// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Color.h"
#include "core/Effect.h"

#include <array>
#include <optional>

namespace core {

/**
 * Temperature and tint, relative to the white balance the image has: what Lightroom calls them for
 * a TIFF or a JPEG. The image is adapted, with the Bradford transform, as if the light had been
 * bluer (temperature above 0, the picture gets warmer) or greener (tint above 0, the picture gets
 * more magenta) than its white balance assumed. 100 is about 100 mired, e.g. from 6500 K to 18600 K.
 */
class WhiteBalanceEffect : public Effect
{
public:
    static inline const Parameter
        TEMPERATURE{"IncrementalTemperature", "Temperature", -100, 100, 0, 0};
    static inline const Parameter TINT{"IncrementalTint", "Tint", -100, 100, 0, 0};

    /// What makes a colour neutral: the two values, and whether they had to be clamped to the
    /// sliders' range, in which case the colour is only nearer neutral.
    struct Neutral
    {
        double temperature = 0;
        double tint = 0;
        bool clamped = false;
    };

    /**
     * The temperature and tint, in whole steps as the sliders set them, that make `rgb` neutral:
     * a colour in linear Rec. 2020 as the image has it before this effect, e.g. the mean of an
     * area that should be white or grey. Nothing when it has no colour to measure, e.g. black.
     */
    static std::optional<Neutral> neutralFor(const std::array<float, 3> &rgb);

    /// The light the picture is taken to have been shot under, for these two values.
    static Chromaticity sourceWhite(double temperature, double tint);

    std::string section() const override { return "White Balance"; }
    std::vector<Parameter> parameters() const override { return {TEMPERATURE, TINT}; }
    void apply(Image &image, const Settings &settings, const RenderContext &context) const override;
};

} // namespace core
