// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Effect.h"

namespace core {

/**
 * The contrast of the fine texture, a few pixels across, e.g. hairs and surface grain: texture,
 * as raw editors also call it. Above 0 it is boosted, below 0 smoothed. Variations no larger than
 * the noise are left alone, so the grain of the sensor is not boosted with the texture.
 */
class DetailEffect : public Effect
{
public:
    static inline const Parameter DETAIL{"Texture", "Detail", -100, 100, 0, 0};

    std::string section() const override { return "Effects"; }
    std::vector<Parameter> parameters() const override { return {DETAIL}; }
    void apply(Image &image, const Settings &settings, const RenderContext &context) const override;
};

} // namespace core
