// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/effects/WhiteBalanceEffect.h"
#include "core/Color.h"
#include "core/Parallel.h"

#include <algorithm>
#include <cmath>

namespace core {

namespace {

/// The temperature the image's own white balance is taken to be, D65's.
constexpr double REFERENCE_KELVIN = 6504.0;
/// Mired per unit of the temperature slider.
constexpr double MIRED_PER_STEP = 1.0;
/// Shift in CIE 1960 v, toward green, per unit of the tint slider.
constexpr double V_PER_TINT_STEP = 0.0002;
/// Darker than this, a colour is mostly noise: there is nothing to measure.
constexpr double MIN_LUMINANCE = 1e-5;

} // namespace

Chromaticity WhiteBalanceEffect::sourceWhite(double temperature, double tint)
{
    // A shift from D65 along the black-body locus, and across it for the tint, so that 0 and 0
    // is exactly D65 and changes nothing.
    const double sourceKelvin = 1e6 / (1e6 / REFERENCE_KELVIN - temperature * MIRED_PER_STEP);
    const auto reference = color::toUv(color::planckian(REFERENCE_KELVIN));
    const auto shifted = color::toUv(color::planckian(sourceKelvin));
    const auto white = color::toUv(color::D65);
    return color::fromUv(
        white[0] + shifted[0] - reference[0],
        white[1] + shifted[1] - reference[1] + tint * V_PER_TINT_STEP);
}

std::optional<WhiteBalanceEffect::Neutral> WhiteBalanceEffect::neutralFor(
    const std::array<float, 3> &rgb)
{
    const Matrix3 &m = color::rec2020ToXyz();
    const double x = m[0] * rgb[0] + m[1] * rgb[1] + m[2] * rgb[2];
    const double y = m[3] * rgb[0] + m[4] * rgb[1] + m[5] * rgb[2];
    const double z = m[6] * rgb[0] + m[7] * rgb[1] + m[8] * rgb[2];
    const double sum = x + y + z;
    if (y <= MIN_LUMINANCE || sum <= 0) {
        return std::nullopt;
    }
    const auto target = color::toUv({x / sum, y / sum});

    // The colour is made neutral when it is the source white. Its u depends on the temperature
    // only, and falls as the temperature rises: found by bisection. The tint then gives its v.
    const double lowest = TEMPERATURE.minimum, highest = TEMPERATURE.maximum;
    auto u = [](double temperature) { return color::toUv(sourceWhite(temperature, 0))[0]; };
    Neutral neutral;
    if (target[0] >= u(lowest)) {
        neutral.temperature = lowest;
        neutral.clamped = true;
    } else if (target[0] <= u(highest)) {
        neutral.temperature = highest;
        neutral.clamped = true;
    } else {
        double low = lowest, high = highest;
        for (int i = 0; i < 60; ++i) {
            const double middle = (low + high) / 2;
            (u(middle) > target[0] ? low : high) = middle;
        }
        neutral.temperature = std::round((low + high) / 2);
    }
    const double v = color::toUv(sourceWhite(neutral.temperature, 0))[1];
    const double tint = std::round((target[1] - v) / V_PER_TINT_STEP);
    neutral.tint = std::clamp(tint, TINT.minimum, TINT.maximum);
    neutral.clamped = neutral.clamped || neutral.tint != tint;
    return neutral;
}

void WhiteBalanceEffect::apply(Image &image, const Settings &settings, const RenderContext &) const
{
    const Chromaticity source = sourceWhite(settings.value(TEMPERATURE), settings.value(TINT));

    const Matrix3 m = color::adaptToD65(source);
    float f[9];
    for (int i = 0; i < 9; ++i) {
        f[i] = static_cast<float>(m[i]);
    }
    parallelFor(0, image.height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            for (int x = 0; x < image.width; ++x) {
                float *p = image.pixel(x, y);
                const float r = p[0], g = p[1], b = p[2];
                p[0] = f[0] * r + f[1] * g + f[2] * b;
                p[1] = f[3] * r + f[4] * g + f[5] * b;
                p[2] = f[6] * r + f[7] * g + f[8] * b;
            }
        }
    });
}

} // namespace core
