// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/effects/StructureEffect.h"
#include "core/Filters.h"
#include "core/Luminance.h"
#include "core/Parallel.h"

#include <algorithm>
#include <cmath>

namespace core {

namespace {

/// The size of the shapes, as a fraction of the image's diagonal.
constexpr double RADIUS_OF_DIAGONAL = 0.01;
/// Edges stronger than this, in stops, are kept out of the smoothing.
constexpr float EDGE_STOPS = 0.2f;
/// How much of the local contrast is added at +100, or removed at -100.
constexpr float STRENGTH = 0.8f;

} // namespace

void StructureEffect::apply(Image &image, const Settings &settings, const RenderContext &) const
{
    const float amount = static_cast<float>(settings.value(STRUCTURE) / 100.0) * STRENGTH;
    const Plane lum = luminance::logLuminance(image);
    const int radius
        = std::max(1, static_cast<int>(std::hypot(lum.width, lum.height) * RADIUS_OF_DIAGONAL));
    const Plane base = filters::guidedFilter(lum, radius, EDGE_STOPS * EDGE_STOPS);
    Plane stops{lum.width, lum.height};
    parallelForEach(stops.data.size(), [&](std::size_t i) {
        stops.data[i] = amount * (lum.data[i] - base.data[i]);
    });
    luminance::applyStops(image, stops);
}

} // namespace core
