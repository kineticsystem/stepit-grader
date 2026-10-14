// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/effects/SharpnessEffect.h"
#include "core/Filters.h"
#include "core/Luminance.h"
#include "core/Parallel.h"

namespace core {

namespace {

/// Variations below about this many stops are taken for noise.
constexpr float NOISE_STOPS = 0.02f;

} // namespace

bool SharpnessEffect::isIdentity(const Settings &settings) const
{
    return settings.value(SHARPNESS) <= 0;
}

void SharpnessEffect::apply(Image &image, const Settings &settings, const RenderContext &context) const
{
    const float amount = static_cast<float>(settings.value(SHARPNESS) / 100.0);
    const Plane lum = luminance::logLuminance(image);
    // The radius of the edges sharpened, in pixels of the full image.
    const Plane blurred = filters::gaussianBlur(lum, settings.value(RADIUS) * context.scale);
    Plane stops{lum.width, lum.height};
    parallelForEach(stops.data.size(), [&](std::size_t i) {
        const float edge = lum.data[i] - blurred.data[i];
        const float squared = edge * edge;
        stops.data[i] = amount * edge * squared / (squared + NOISE_STOPS * NOISE_STOPS);
    });
    luminance::applyStops(image, stops);
}

} // namespace core
