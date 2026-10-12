// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Effect.h"

namespace core {

/**
 * Removes purple fringing: the purple or violet edge a lens draws along a strong contrast, e.g. a
 * dark branch against a bright sky. A pixel loses its colour when its hue is in the range set
 * and it lies within a few pixels of a strong edge; a purple area away from edges keeps its
 * colour. The amount, 0 to 20, also
 * widens the band around the edges searched; the hue range, 0 to 100, goes from blue to magenta.
 */
class DefringeEffect : public Effect
{
public:
    static inline const Parameter AMOUNT{"DefringePurpleAmount", "Purple Amount", 0, 20, 0, 0};
    static inline const Parameter HUE_LOW{"DefringePurpleHueLo", "Purple Hue Low", 0, 100, 30, 0};
    static inline const Parameter HUE_HIGH{"DefringePurpleHueHi", "Purple Hue High", 0, 100, 70, 0};

    std::string section() const override { return "Chromatic Aberration"; }
    std::vector<Parameter> parameters() const override { return {AMOUNT, HUE_LOW, HUE_HIGH}; }
    /// Nothing to do without an amount, whatever the hue range.
    bool isIdentity(const Settings &settings) const override;
    void apply(Image &image, const Settings &settings, const RenderContext &context) const override;
};

} // namespace core
