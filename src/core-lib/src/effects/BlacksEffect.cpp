// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/effects/BlacksEffect.h"
#include "core/Color.h"
#include "core/Luminance.h"
#include "core/Parallel.h"

#include <algorithm>

namespace core {

namespace {

/// The black point moves by up to this much linear luminance, 1/9 of middle grey, at +-100.
constexpr float STRENGTH = 0.02f;

} // namespace

void BlacksEffect::apply(Image &image, const Settings &settings, const RenderContext &) const
{
    const float amount = static_cast<float>(settings.value(BLACKS) / 100.0) * STRENGTH;
    parallelFor(0, image.height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            for (int x = 0; x < image.width; ++x) {
                float *p = image.pixel(x, y);
                const float lum = std::max(0.0f, color::luminance(p));
                if (amount > 0) {
                    // Lifted as grey, so black becomes a neutral dark grey, fading out by middle
                    // grey.
                    const float lift = amount
                                       * (1.0f
                                          - luminance::smoothstep(0.0f, color::MIDDLE_GREY, lum));
                    p[0] += lift;
                    p[1] += lift;
                    p[2] += lift;
                } else {
                    // Y^2 / (Y + b): about Y - b in the mid-tones, and a smooth toe to 0.
                    const float ratio = lum / (lum - amount);
                    p[0] *= ratio;
                    p[1] *= ratio;
                    p[2] *= ratio;
                }
            }
        }
    });
}

} // namespace core
