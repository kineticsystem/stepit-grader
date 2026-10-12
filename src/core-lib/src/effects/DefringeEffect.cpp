// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/effects/DefringeEffect.h"
#include "core/Color.h"
#include "core/Filters.h"
#include "core/Luminance.h"
#include "core/Parallel.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

namespace core {

namespace {

/// The hue range's 0 and 100, in Oklab hue degrees: from blue, through violet and purple, to
/// magenta. The default range, 30 to 70, is about 276 to 324 degrees.
constexpr float HUE_FROM_DEGREES = 240.0f;
constexpr float HUE_TO_DEGREES = 360.0f;
/// Hues this close outside the range are treated less and less.
constexpr float HUE_FEATHER_DEGREES = 12.0f;
/// Colours below this Oklab chroma are grey already: left alone, with their noise.
constexpr float MIN_CHROMA = 0.01f;
/// The band searched around an edge, in pixels of the full image: from 1 pixel at amount 1 to 7
/// at 20.
constexpr double BAND_BASE_PIXELS = 0.7;
constexpr double BAND_PIXELS_PER_STEP = 0.32;
/// An edge across which the luminance changes by this many stops, from half a stop to a stop and
/// a half, is more and more a fringing edge.
constexpr float EDGE_FROM_STOPS = 0.5f;
constexpr float EDGE_TO_STOPS = 1.5f;
/// The colour removed at the full amount: all of it.
constexpr float MAX_AMOUNT = 20.0f;

float hueDegrees(float a, float b)
{
    const float degrees = std::atan2(b, a) * 180.0f / std::numbers::pi_v<float>;
    return degrees < 0 ? degrees + 360.0f : degrees;
}

/// 1 inside [low, high], fading to 0 over the feather outside.
float hueWeight(float hue, float low, float high)
{
    if (hue >= low && hue <= high) {
        return 1.0f;
    }
    const float distance = hue < low ? low - hue : hue - high;
    // Hues wrap around: 5 degrees is 365, just past magenta.
    const float wrapped = std::min(distance, 360.0f - distance);
    return 1.0f - luminance::smoothstep(0.0f, HUE_FEATHER_DEGREES, wrapped);
}

/// How much the luminance changes at each pixel, in stops per pixel.
Plane edgeStrength(const Plane &lum)
{
    Plane edges{lum.width, lum.height};
    parallelFor(0, lum.height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            const int up = std::max(0, y - 1), down = std::min(lum.height - 1, y + 1);
            for (int x = 0; x < lum.width; ++x) {
                const int left = std::max(0, x - 1), right = std::min(lum.width - 1, x + 1);
                const float dx = (lum.at(right, y) - lum.at(left, y)) * 0.5f;
                const float dy = (lum.at(x, down) - lum.at(x, up)) * 0.5f;
                edges.at(x, y) = std::hypot(dx, dy);
            }
        }
    });
    return edges;
}

} // namespace

bool DefringeEffect::isIdentity(const Settings &settings) const
{
    return settings.value(AMOUNT) <= 0;
}

void DefringeEffect::apply(Image &image, const Settings &settings, const RenderContext &context) const
{
    const float amount = static_cast<float>(settings.value(AMOUNT));
    if (amount <= 0) {
        return;
    }
    float low = static_cast<float>(settings.value(HUE_LOW));
    float high = static_cast<float>(settings.value(HUE_HIGH));
    if (low > high) {
        std::swap(low, high);
    }
    const float span = HUE_TO_DEGREES - HUE_FROM_DEGREES;
    low = HUE_FROM_DEGREES + low / 100.0f * span;
    high = HUE_FROM_DEGREES + high / 100.0f * span;

    // The change of luminance across the band around each pixel, in stops: the gradient,
    // averaged over the band, times its width. A sharp edge gives its full step whatever the
    // band's width, so the preview and the full image agree. The fringe lies beside the edge,
    // not on it, hence the band.
    const Plane lum = luminance::logLuminance(image);
    const double band = (BAND_BASE_PIXELS + BAND_PIXELS_PER_STEP * amount) * context.scale;
    const int radius = std::max(1, static_cast<int>(std::lround(band)));
    const Plane edges = filters::boxBlur(edgeStrength(lum), radius);
    const float width = static_cast<float>(2 * radius + 1);

    const float strength = std::min(1.0f, amount / MAX_AMOUNT);
    parallelFor(0, image.height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            for (int x = 0; x < image.width; ++x) {
                float *p = image.pixel(x, y);
                float lab[3];
                color::rec2020ToOklab(p, lab);
                const float chroma = std::hypot(lab[1], lab[2]);
                if (chroma < MIN_CHROMA) {
                    continue;
                }
                const float edge = edges.at(x, y) * width;
                const float weight = strength * hueWeight(hueDegrees(lab[1], lab[2]), low, high)
                                     * luminance::smoothstep(EDGE_FROM_STOPS, EDGE_TO_STOPS, edge);
                if (weight <= 0) {
                    continue;
                }
                lab[1] *= 1.0f - weight;
                lab[2] *= 1.0f - weight;
                color::oklabToRec2020(lab, p);
            }
        }
    });
}

} // namespace core
