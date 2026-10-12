// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Effect.h"

namespace core {

/**
 * Removes the coloured blotches a high ISO makes, mostly in the shadows: the colour, Oklab's a and
 * b, smoothed over up to 16 pixels of the full image, guided by the luminance, so the colour keeps
 * to the edges the luminance shows and does not bleed across them. The luminance, and so the
 * detail, is untouched.
 */
class ColorNoiseEffect : public Effect
{
public:
    static inline const Parameter AMOUNT{"ColorNoiseReduction", "Color", 0, 100, 0, 0};

    std::string section() const override { return "Noise Reduction"; }
    std::vector<Parameter> parameters() const override { return {AMOUNT}; }
    void apply(Image &image, const Settings &settings, const RenderContext &context) const override;
};

} // namespace core
