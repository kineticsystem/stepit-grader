// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/effects/ColorNoiseEffect.h"
#include "core/Color.h"
#include "core/Filters.h"
#include "core/Luminance.h"
#include "core/Parallel.h"

#include <algorithm>
#include <cmath>

namespace core {

namespace {

/// The size of the blotches removed at 100, in pixels of the full image.
constexpr double MAX_RADIUS_PIXELS = 16.0;
/// Edges of the luminance stronger than about this, in Oklab's L, keep the colour from crossing.
constexpr float EDGE_LIGHTNESS = 0.02f;
/// Below this amount, the smoothed colour is blended in, so the first steps are gentle.
constexpr float FULL_BLEND_AMOUNT = 0.25f;

} // namespace

void ColorNoiseEffect::apply(
    Image &image, const Settings &settings, const RenderContext &context) const
{
    const float amount = static_cast<float>(settings.value(AMOUNT) / 100.0);
    const int radius = std::max(
        1, static_cast<int>(std::lround(1.0 + amount * MAX_RADIUS_PIXELS * context.scale)));
    const float blend = luminance::smoothstep(0.0f, FULL_BLEND_AMOUNT, amount);

    Plane lightness{image.width, image.height};
    Plane a{image.width, image.height};
    Plane b{image.width, image.height};
    parallelFor(0, image.height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            for (int x = 0; x < image.width; ++x) {
                float lab[3];
                color::rec2020ToOklab(image.pixel(x, y), lab);
                lightness.at(x, y) = lab[0];
                a.at(x, y) = lab[1];
                b.at(x, y) = lab[2];
            }
        }
    });
    const float epsilon = EDGE_LIGHTNESS * EDGE_LIGHTNESS;
    const Plane smoothA = filters::guidedFilter(lightness, a, radius, epsilon);
    const Plane smoothB = filters::guidedFilter(lightness, b, radius, epsilon);

    parallelFor(0, image.height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            for (int x = 0; x < image.width; ++x) {
                const float lab[3]
                    = {lightness.at(x, y),
                       a.at(x, y) + blend * (smoothA.at(x, y) - a.at(x, y)),
                       b.at(x, y) + blend * (smoothB.at(x, y) - b.at(x, y))};
                color::oklabToRec2020(lab, image.pixel(x, y));
            }
        }
    });
}

} // namespace core
