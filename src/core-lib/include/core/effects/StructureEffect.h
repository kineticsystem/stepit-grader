// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Effect.h"

namespace core {

/**
 * The contrast of the medium-sized shapes, about a hundredth of the picture across, e.g. a leg
 * against a body: clarity, as raw editors also call it. The local contrast is measured against an
 * edge-preserving smoothing, so strong edges draw no halo.
 */
class StructureEffect : public Effect
{
public:
    static inline const Parameter STRUCTURE{"Clarity2012", "Structure", -100, 100, 0, 0};

    std::string section() const override { return "Effects"; }
    std::vector<Parameter> parameters() const override { return {STRUCTURE}; }
    void apply(Image &image, const Settings &settings, const RenderContext &context) const override;
};

} // namespace core
