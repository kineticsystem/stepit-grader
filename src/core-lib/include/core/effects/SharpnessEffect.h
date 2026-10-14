// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Effect.h"

namespace core {

/**
 * Sharpens the edges, an unsharp mask on the luminance, so the edges get no colour fringes. The
 * radius is in pixels of the full image, Lightroom's range: 1 sharpens the finest edges, a larger
 * one wider edges, which stay visible on a preview much smaller than the image, or on a print
 * that shows less detail than the picture has.
 */
class SharpnessEffect : public Effect
{
public:
    static inline const Parameter SHARPNESS{"Sharpness", "Sharpness", 0, 150, 0, 0};
    static inline const Parameter RADIUS{"SharpenRadius", "Radius", 0.5, 3.0, 1.0, 1};

    std::string section() const override { return "Effects"; }
    std::vector<Parameter> parameters() const override { return {SHARPNESS, RADIUS}; }
    /// Nothing to sharpen without an amount, whatever the radius.
    bool isIdentity(const Settings &settings) const override;
    void apply(Image &image, const Settings &settings, const RenderContext &context) const override;
};

} // namespace core
