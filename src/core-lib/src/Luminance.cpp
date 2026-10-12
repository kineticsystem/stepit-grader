// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/Luminance.h"
#include "core/Color.h"
#include "core/Filters.h"
#include "core/Parallel.h"

#include <algorithm>
#include <cmath>

namespace core::luminance {

namespace {

/// The darkest luminance considered, about 19 stops below middle grey: black stays black.
constexpr float MIN_LUMINANCE = 1e-6f;

} // namespace

Plane logLuminance(const Image &image)
{
    Plane plane{image.width, image.height};
    parallelFor(0, image.height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            for (int x = 0; x < image.width; ++x) {
                const float lum = std::max(MIN_LUMINANCE, color::luminance(image.pixel(x, y)));
                plane.at(x, y) = std::log2(lum / color::MIDDLE_GREY);
            }
        }
    });
    return plane;
}

void applyStops(Image &image, const Plane &stops)
{
    parallelFor(0, image.height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            for (int x = 0; x < image.width; ++x) {
                const float gain = std::exp2(stops.at(x, y));
                float *p = image.pixel(x, y);
                p[0] *= gain;
                p[1] *= gain;
                p[2] *= gain;
            }
        }
    });
}

Plane areaLuminance(const Plane &logLuminance)
{
    constexpr double RADIUS_OF_DIAGONAL = 0.02;
    constexpr float EDGE_STOPS = 0.5f;
    const double diagonal = std::hypot(logLuminance.width, logLuminance.height);
    const int radius = std::max(1, static_cast<int>(diagonal * RADIUS_OF_DIAGONAL));
    return filters::guidedFilter(logLuminance, radius, EDGE_STOPS * EDGE_STOPS);
}

float smoothstep(float edge0, float edge1, float x)
{
    const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

} // namespace core::luminance
