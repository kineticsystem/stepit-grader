// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Effect.h"

namespace core {

/**
 * Smooths the grain of the luminance, as a high ISO makes, and keeps the edges: an
 * edge-preserving filter on the square root of the luminance, where the sensor's noise is about
 * the same in the shadows and the highlights, so one setting cleans both. The detail puts back
 * part of what the smoothing took.
 */
class LuminanceNoiseEffect : public Effect
{
public:
    static inline const Parameter AMOUNT{"LuminanceSmoothing", "Luminance", 0, 100, 0, 0};
    static inline const Parameter
        DETAIL{"LuminanceNoiseReductionDetail", "Luminance Detail", 0, 100, 50, 0};

    std::string section() const override { return "Noise Reduction"; }
    std::vector<Parameter> parameters() const override { return {AMOUNT, DETAIL}; }
    /// Nothing to do without an amount, whatever the detail.
    bool isIdentity(const Settings &settings) const override;
    void apply(Image &image, const Settings &settings, const RenderContext &context) const override;
};

} // namespace core
