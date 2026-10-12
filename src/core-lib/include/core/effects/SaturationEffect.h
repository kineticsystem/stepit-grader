// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Effect.h"

namespace core {

/// The saturation of every colour, in Oklab, so the hues stay: -100 is black and white.
class SaturationEffect : public Effect
{
public:
    static inline const Parameter SATURATION{"Saturation", "Saturation", -100, 100, 0, 0};

    std::string section() const override { return "Color"; }
    std::vector<Parameter> parameters() const override { return {SATURATION}; }
    void apply(Image &image, const Settings &settings, const RenderContext &context) const override;
};

} // namespace core
