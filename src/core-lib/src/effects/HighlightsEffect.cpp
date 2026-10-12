// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/effects/HighlightsEffect.h"
#include "core/Luminance.h"
#include "core/Parallel.h"

namespace core {

namespace {

/// Areas from middle grey (0) up to 3 stops above it are more and more a highlight.
constexpr float FROM_STOPS = 0.0f;
constexpr float TO_STOPS = 3.0f;
/// The stops a full highlight moves at +-100.
constexpr float STRENGTH = 1.5f;

} // namespace

void HighlightsEffect::apply(Image &image, const Settings &settings, const RenderContext &) const
{
    const float amount = static_cast<float>(settings.value(HIGHLIGHTS) / 100.0) * STRENGTH;
    Plane stops = luminance::areaLuminance(luminance::logLuminance(image));
    parallelForEach(stops.data.size(), [&](std::size_t i) {
        stops.data[i] = amount * luminance::smoothstep(FROM_STOPS, TO_STOPS, stops.data[i]);
    });
    luminance::applyStops(image, stops);
}

} // namespace core
