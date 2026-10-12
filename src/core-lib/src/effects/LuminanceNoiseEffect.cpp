// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/effects/LuminanceNoiseEffect.h"
#include "core/Color.h"
#include "core/Filters.h"
#include "core/Parallel.h"

#include <algorithm>
#include <cmath>

namespace core {

namespace {

/// The noise smoothed at 100, in the square root of the luminance: about three times the
/// grain of a mid-grey at ISO 6400 on a full-frame sensor.
constexpr float MAX_NOISE = 0.03f;
/// The area the noise is measured over, in pixels of the full image: from 1 to 4 as the amount
/// grows, as strong noise is coarser.
constexpr double MIN_RADIUS_PIXELS = 1.0;
constexpr double MAX_RADIUS_PIXELS = 4.0;
/// The share of the detail put back at a detail of 100.
constexpr float MAX_DETAIL = 0.4f;
/// Darker than this, a pixel is left alone: its colour cannot be scaled.
constexpr float MIN_LUMINANCE = 1e-7f;

} // namespace

bool LuminanceNoiseEffect::isIdentity(const Settings &settings) const
{
    return settings.value(AMOUNT) <= 0;
}

void LuminanceNoiseEffect::apply(
    Image &image, const Settings &settings, const RenderContext &context) const
{
    const float amount = static_cast<float>(settings.value(AMOUNT) / 100.0);
    if (amount <= 0) {
        return;
    }
    const float detail = static_cast<float>(settings.value(DETAIL) / 100.0) * MAX_DETAIL;
    const float noise = amount * MAX_NOISE;
    const double pixels = MIN_RADIUS_PIXELS + amount * (MAX_RADIUS_PIXELS - MIN_RADIUS_PIXELS);
    const int radius = std::max(1, static_cast<int>(std::lround(pixels * context.scale)));

    Plane roots{image.width, image.height};
    parallelFor(0, image.height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            for (int x = 0; x < image.width; ++x) {
                roots.at(x, y) = std::sqrt(std::max(0.0f, color::luminance(image.pixel(x, y))));
            }
        }
    });
    const Plane smooth = filters::guidedFilter(roots, radius, noise * noise);

    parallelFor(0, image.height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            for (int x = 0; x < image.width; ++x) {
                const float root = roots.at(x, y);
                if (root * root < MIN_LUMINANCE) {
                    continue;
                }
                const float cleaned = smooth.at(x, y) + detail * (root - smooth.at(x, y));
                const float gain = std::max(0.0f, cleaned) * std::max(0.0f, cleaned)
                                   / (root * root);
                float *p = image.pixel(x, y);
                p[0] *= gain;
                p[1] *= gain;
                p[2] *= gain;
            }
        }
    });
}

} // namespace core
