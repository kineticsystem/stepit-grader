// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Effect.h"

namespace core {

/**
 * Moves the black point: below 0 the darkest tones go to black, above 0 black is lifted toward a
 * dark grey, as a faded print. Only the tones well below middle grey change.
 */
class BlacksEffect : public Effect
{
public:
    static inline const Parameter BLACKS{"Blacks2012", "Blacks", -100, 100, 0, 0};

    std::string section() const override { return "Light"; }
    std::vector<Parameter> parameters() const override { return {BLACKS}; }
    void apply(Image &image, const Settings &settings, const RenderContext &context) const override;
};

} // namespace core
