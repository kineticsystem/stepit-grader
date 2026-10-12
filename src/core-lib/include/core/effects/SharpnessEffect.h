// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Effect.h"

namespace core {

/**
 * Sharpens the finest edges, an unsharp mask of 1 pixel of the full image on the luminance, so
 * the edges get no colour fringes. On a preview much smaller than the image it does little: look
 * at it at 100%.
 */
class SharpnessEffect : public Effect
{
public:
    static inline const Parameter SHARPNESS{"Sharpness", "Sharpness", 0, 150, 0, 0};

    std::string section() const override { return "Effects"; }
    std::vector<Parameter> parameters() const override { return {SHARPNESS}; }
    void apply(Image &image, const Settings &settings, const RenderContext &context) const override;
};

} // namespace core
