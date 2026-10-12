// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Effect.h"

namespace core {

/**
 * Spreads the tones away from middle grey (above 0) or toward it (below 0), most in the mid-tones
 * and less and less beyond 3 stops from grey, so the blacks and the whites are not thrown away.
 */
class ContrastEffect : public Effect
{
public:
    static inline const Parameter CONTRAST{"Contrast2012", "Contrast", -100, 100, 0, 0};

    std::string section() const override { return "Tone"; }
    std::vector<Parameter> parameters() const override { return {CONTRAST}; }
    void apply(Image &image, const Settings &settings, const RenderContext &context) const override;
};

} // namespace core
