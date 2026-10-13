// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/effects/ToneCurveEffect.h"
#include "core/Color.h"
#include "core/Parallel.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace core {

namespace {

/// How far a region's middle moves at +100 or -100, as a fraction of the region's width.
constexpr double SHIFT = 0.5;
/// The smallest gap between two splits, and between two knots, in tones: the curve never goes flat.
constexpr double MIN_GAP = 0.02;
/// The samples of the curve per pixel lookup, from black to white.
constexpr int TABLE_SIZE = 4096;

} // namespace

bool ToneCurveEffect::isIdentity(const Settings &settings) const
{
    return settings.value(SHADOWS) == 0 && settings.value(DARKS) == 0 && settings.value(LIGHTS) == 0
           && settings.value(HIGHLIGHTS) == 0;
}

std::array<double, 3> ToneCurveEffect::splits(const Settings &settings)
{
    const double shadowSplit = settings.value(SHADOW_SPLIT) / 100.0;
    const double midtoneSplit
        = std::max(settings.value(MIDTONE_SPLIT) / 100.0, shadowSplit + MIN_GAP);
    const double highlightSplit
        = std::clamp(settings.value(HIGHLIGHT_SPLIT) / 100.0, midtoneSplit + MIN_GAP, 1.0 - MIN_GAP);
    return {shadowSplit, midtoneSplit, highlightSplit};
}

std::array<std::array<double, 2>, 6> ToneCurveEffect::knots(const Settings &settings)
{
    const auto [shadowSplit, midtoneSplit, highlightSplit] = splits(settings);
    const std::array<double, 5> borders{0.0, shadowSplit, midtoneSplit, highlightSplit, 1.0};
    const std::array<double, 4> amounts{
        settings.value(SHADOWS) / 100.0,
        settings.value(DARKS) / 100.0,
        settings.value(LIGHTS) / 100.0,
        settings.value(HIGHLIGHTS) / 100.0};

    std::array<std::array<double, 2>, 6> knots{};
    knots[0] = {0.0, 0.0};
    knots[5] = {1.0, 1.0};
    for (int i = 0; i < 4; ++i) {
        const double width = borders[i + 1] - borders[i];
        const double x = borders[i] + width / 2;
        knots[i + 1] = {x, x + amounts[i] * SHIFT * width};
    }
    // Every knot above the one before and below white, so the curve rises all the way.
    for (int i = 1; i < 5; ++i) {
        knots[i][1] = std::clamp(knots[i][1], knots[i - 1][1] + MIN_GAP, 1.0 - MIN_GAP * (5 - i));
    }
    return knots;
}

double ToneCurveEffect::curve(const std::array<std::array<double, 2>, 6> &knots, double tone)
{
    constexpr int n = 6;
    std::array<double, n - 1> secants{};
    for (int i = 0; i < n - 1; ++i) {
        secants[i] = (knots[i + 1][1] - knots[i][1]) / (knots[i + 1][0] - knots[i][0]);
    }
    // Fritsch and Carlson: the tangents of a cubic that does not overshoot between the knots.
    std::array<double, n> tangents{};
    tangents[0] = secants[0];
    tangents[n - 1] = secants[n - 2];
    for (int i = 1; i < n - 1; ++i) {
        tangents[i] = secants[i - 1] * secants[i] <= 0 ? 0.0 : (secants[i - 1] + secants[i]) / 2;
    }
    for (int i = 0; i < n - 1; ++i) {
        const double a = tangents[i] / secants[i];
        const double b = tangents[i + 1] / secants[i];
        const double length = std::hypot(a, b);
        if (length > 3) {
            tangents[i] = 3 * a / length * secants[i];
            tangents[i + 1] = 3 * b / length * secants[i];
        }
    }

    tone = std::clamp(tone, 0.0, 1.0);
    int i = 0;
    while (i < n - 2 && tone > knots[i + 1][0]) {
        ++i;
    }
    const double h = knots[i + 1][0] - knots[i][0];
    const double t = (tone - knots[i][0]) / h;
    const double t2 = t * t;
    const double t3 = t2 * t;
    return (2 * t3 - 3 * t2 + 1) * knots[i][1] + (t3 - 2 * t2 + t) * h * tangents[i]
           + (-2 * t3 + 3 * t2) * knots[i + 1][1] + (t3 - t2) * h * tangents[i + 1];
}

void ToneCurveEffect::apply(Image &image, const Settings &settings, const RenderContext &) const
{
    // The gain of each luminance below white, sampled evenly in tones.
    const auto points = knots(settings);
    std::vector<float> gains(TABLE_SIZE + 1);
    for (int i = 0; i <= TABLE_SIZE; ++i) {
        const double tone = static_cast<double>(i) / TABLE_SIZE;
        const double before = color::srgbDecode(tone);
        const double after = color::srgbDecode(curve(points, tone));
        gains[i] = before > 0 ? static_cast<float>(after / before) : 1.0f;
    }
    // Black has no colour to keep: its gain is that of the first tone above it.
    gains[0] = gains[1];

    parallelForEach(image.pixelCount(), [&](std::size_t i) {
        float *p = &image.data[i * 3];
        const float luminance = color::luminance(p);
        if (luminance <= 0 || luminance >= 1) {
            return;
        }
        const float position = static_cast<float>(color::srgbEncode(luminance)) * TABLE_SIZE;
        const int index = std::min(static_cast<int>(position), TABLE_SIZE - 1);
        const float fraction = position - index;
        const float gain = gains[index] + (gains[index + 1] - gains[index]) * fraction;
        p[0] *= gain;
        p[1] *= gain;
        p[2] *= gain;
    });
}

} // namespace core
