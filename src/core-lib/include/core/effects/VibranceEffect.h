// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Effect.h"

namespace core {

/**
 * Saturation that changes the dull colours more than the vivid ones, so already vivid colours are
 * not pushed out of gamut. Measured in Oklab, so the hues stay.
 */
class VibranceEffect : public Effect
{
public:
    static inline const Parameter VIBRANCE{"Vibrance", "Vibrance", -100, 100, 0, 0};

    std::string section() const override { return "Presence"; }
    std::vector<Parameter> parameters() const override { return {VIBRANCE}; }
    void apply(Image &image, const Settings &settings, const RenderContext &context) const override;
};

} // namespace core
