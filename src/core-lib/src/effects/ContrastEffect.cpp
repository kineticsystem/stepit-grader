// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/effects/ContrastEffect.h"
#include "core/Luminance.h"
#include "core/Parallel.h"

#include <cmath>

namespace core {

namespace {

/// How far from middle grey, in stops, the contrast keeps growing before it levels off.
constexpr float RANGE_STOPS = 3.0f;
/// The stops a tone moves per stop from grey, at +100.
constexpr float STRENGTH = 0.5f;

} // namespace

void ContrastEffect::apply(Image &image, const Settings &settings, const RenderContext &) const
{
    const float amount = static_cast<float>(settings.value(CONTRAST) / 100.0) * STRENGTH;
    Plane stops = luminance::logLuminance(image);
    parallelForEach(stops.data.size(), [&](std::size_t i) {
        stops.data[i] = amount * RANGE_STOPS * std::tanh(stops.data[i] / RANGE_STOPS);
    });
    luminance::applyStops(image, stops);
}

} // namespace core
