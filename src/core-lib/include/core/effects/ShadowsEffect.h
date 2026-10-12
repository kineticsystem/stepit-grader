// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Effect.h"

namespace core {

/**
 * Brightens (above 0) or darkens (below 0) the dark areas, up to 2 stops, and leaves the bright
 * ones, deciding what is dark as HighlightsEffect does.
 */
class ShadowsEffect : public Effect
{
public:
    static inline const Parameter SHADOWS{"Shadows2012", "Shadows", -100, 100, 0, 0};

    std::string section() const override { return "Light"; }
    std::vector<Parameter> parameters() const override { return {SHADOWS}; }
    void apply(Image &image, const Settings &settings, const RenderContext &context) const override;
};

} // namespace core
