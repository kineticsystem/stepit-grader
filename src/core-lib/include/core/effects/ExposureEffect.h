// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Effect.h"

namespace core {

/// The exposure in stops: every value is multiplied by 2^exposure, as more light would.
class ExposureEffect : public Effect
{
public:
    static inline const Parameter EXPOSURE{"Exposure2012", "Exposure", -5, 5, 0, 2};

    std::string section() const override { return "Tone"; }
    std::vector<Parameter> parameters() const override { return {EXPOSURE}; }
    void apply(Image &image, const Settings &settings, const RenderContext &context) const override;
};

} // namespace core
