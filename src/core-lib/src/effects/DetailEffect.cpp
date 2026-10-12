// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/effects/DetailEffect.h"
#include "core/Filters.h"
#include "core/Luminance.h"
#include "core/Parallel.h"

namespace core {

namespace {

/// The texture is what lies between these two blurs, in pixels of the full image.
constexpr double FINE_SIGMA = 1.0;
constexpr double COARSE_SIGMA = 4.0;
/// Variations below about this many stops are taken for noise.
constexpr float NOISE_STOPS = 0.05f;
/// How much of the texture is added at +100; at -100 it is smoothed away.
constexpr float STRENGTH = 2.0f;

} // namespace

void DetailEffect::apply(Image &image, const Settings &settings, const RenderContext &context) const
{
    const float value = static_cast<float>(settings.value(DETAIL) / 100.0);
    // Smoothing more than the whole band would invert it.
    const float amount = value > 0 ? value * STRENGTH : value;
    const Plane lum = luminance::logLuminance(image);
    const Plane fine = filters::gaussianBlur(lum, FINE_SIGMA * context.scale);
    const Plane coarse = filters::gaussianBlur(lum, COARSE_SIGMA * context.scale);
    Plane stops{lum.width, lum.height};
    parallelForEach(stops.data.size(), [&](std::size_t i) {
        const float band = fine.data[i] - coarse.data[i];
        const float squared = band * band;
        stops.data[i] = amount * band * squared / (squared + NOISE_STOPS * NOISE_STOPS);
    });
    luminance::applyStops(image, stops);
}

} // namespace core
