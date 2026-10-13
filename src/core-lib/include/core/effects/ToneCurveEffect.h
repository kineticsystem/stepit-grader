// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Effect.h"

#include <array>

namespace core {

/**
 * A parametric tone curve, as raw editors have it: the tones from black to white split into four
 * regions, shadows, darks, lights and highlights, each raised or lowered by its slider, the curve
 * staying smooth and never going down. The three splits move the borders of the regions.
 *
 * The curve works on the luminance as the screen shows it, sRGB-encoded, so the regions are even
 * to the eye; a pixel keeps its colour. Black stays black, and what is at white or above it is
 * left alone, for the HDR compression to bring back.
 */
class ToneCurveEffect : public Effect
{
public:
    static inline const Parameter SHADOWS{"ParametricShadows", "Curve Shadows", -100, 100, 0, 0};
    static inline const Parameter DARKS{"ParametricDarks", "Curve Darks", -100, 100, 0, 0};
    static inline const Parameter LIGHTS{"ParametricLights", "Curve Lights", -100, 100, 0, 0};
    static inline const Parameter
        HIGHLIGHTS{"ParametricHighlights", "Curve Highlights", -100, 100, 0, 0};
    static inline const Parameter
        SHADOW_SPLIT{"ParametricShadowSplit", "Shadows Split", 5, 45, 25, 0};
    static inline const Parameter
        MIDTONE_SPLIT{"ParametricMidtoneSplit", "Midtones Split", 25, 75, 50, 0};
    static inline const Parameter
        HIGHLIGHT_SPLIT{"ParametricHighlightSplit", "Highlights Split", 55, 95, 75, 0};

    std::string section() const override { return "Tone Curve"; }
    std::vector<Parameter> parameters() const override
    {
        return {SHADOWS, DARKS, LIGHTS, HIGHLIGHTS, SHADOW_SPLIT, MIDTONE_SPLIT, HIGHLIGHT_SPLIT};
    }
    /// The splits alone move no tone: only the four regions do.
    bool isIdentity(const Settings &settings) const override;
    bool isPointwise() const override { return true; }
    void apply(Image &image, const Settings &settings, const RenderContext &context) const override;

    /**
     * The three borders between the regions, in sRGB-encoded tones from 0 to 1: the splits'
     * sliders put in order, with room between them, whatever their values.
     */
    static std::array<double, 3> splits(const Settings &settings);

    /**
     * The curve's six knots, x and y in sRGB-encoded tones from 0 to 1: black, the middle of each
     * region, white. Strictly increasing, so the curve through them never goes down.
     */
    static std::array<std::array<double, 2>, 6> knots(const Settings &settings);

    /// The curve at `tone`, 0 to 1, through the knots: monotone cubic (Fritsch and Carlson).
    static double curve(const std::array<std::array<double, 2>, 6> &knots, double tone);
};

} // namespace core
