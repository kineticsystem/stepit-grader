// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/effects/ShadowsEffect.h"
#include "core/Luminance.h"
#include "core/Parallel.h"

namespace core {

namespace {

/// Areas from middle grey (0) down to 4 stops below it are more and more a shadow.
constexpr float FROM_STOPS = 0.0f;
constexpr float TO_STOPS = -4.0f;
/// The stops a full shadow moves at +-100.
constexpr float STRENGTH = 2.0f;

} // namespace

void ShadowsEffect::apply(Image &image, const Settings &settings, const RenderContext &) const
{
    const float amount = static_cast<float>(settings.value(SHADOWS) / 100.0) * STRENGTH;
    Plane stops = luminance::areaLuminance(luminance::logLuminance(image));
    parallelForEach(stops.data.size(), [&](std::size_t i) {
        stops.data[i] = amount * luminance::smoothstep(FROM_STOPS, TO_STOPS, stops.data[i]);
    });
    luminance::applyStops(image, stops);
}

} // namespace core
