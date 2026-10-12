// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Effect.h"

namespace core {

/**
 * Darkens (below 0) or brightens (above 0) the bright areas, up to 1.5 stops, and leaves the dark
 * ones. Bright is decided on the luminance smoothed by an edge-preserving filter, so an area
 * changes as a whole, its texture stays, and a strong edge draws no halo.
 */
class HighlightsEffect : public Effect
{
public:
    static inline const Parameter HIGHLIGHTS{"Highlights2012", "Highlights", -100, 100, 0, 0};

    std::string section() const override { return "Light"; }
    std::vector<Parameter> parameters() const override { return {HIGHLIGHTS}; }
    void apply(Image &image, const Settings &settings, const RenderContext &context) const override;
};

} // namespace core
